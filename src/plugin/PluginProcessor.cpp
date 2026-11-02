#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace mx400;

static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for (const auto& p : mxp::table())
    {
        const juce::ParameterID pid { p.id, 1 };
        if (p.kind == mxp::Kind::Toggle)
            l.add (std::make_unique<juce::AudioParameterBool> (pid, p.name, false));
        else if (p.key == "bank")
            l.add (std::make_unique<juce::AudioParameterInt> (pid, p.name, 0, 1, 0));
        else if (p.key == "prog")
            l.add (std::make_unique<juce::AudioParameterInt> (pid, p.name, 1, 99, 1));
        else
            l.add (std::make_unique<juce::AudioParameterInt> (pid, p.name, 0, 127, p.def));
    }
    return l;
}

Mx400Processor::Mx400Processor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread ("MX400 sender"),
      apvts (*this, nullptr, "MX400", makeLayout()),
      touched (mxp::table().size())
{
    const auto& t = mxp::table();
    raw.resize (t.size());
    lastSeen.assign (t.size(), 0);
    for (size_t i = 0; i < t.size(); ++i)
    {
        raw[i] = apvts.getRawParameterValue (t[i].id);
        touched[i].store (false);
        lastSeen[i] = (int) (raw[i]->load() + 0.5f);
        if (t[i].key == "bank") idxBank[t[i].program] = (int) i;
        if (t[i].key == "prog") idxProg[t[i].program] = (int) i;
    }
    refreshAtomics();
    startThread();
}

Mx400Processor::~Mx400Processor()
{
    stopThread (2000);
    if (midiIn) midiIn->stop();
    midiIn.reset();
    const juce::ScopedLock sl (outLock);
    midiOut.reset();
}

bool Mx400Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    return (o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono()) && o == l.getMainInputChannelSet();
}

void Mx400Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());      // audio otherwise passes through untouched
    if (! thru.load()) midi.clear();

    int s1, n1, s2, n2;
    fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
    for (int i = 0; i < n1; ++i) { const auto& m = fifoBuf[(size_t) (s1 + i)]; midi.addEvent (juce::MidiMessage (m.b.data(), (int) m.n), 0); }
    for (int i = 0; i < n2; ++i) { const auto& m = fifoBuf[(size_t) (s2 + i)]; midi.addEvent (juce::MidiMessage (m.b.data(), (int) m.n), 0); }
    fifo.finishedRead (n1 + n2);
}

juce::AudioProcessorEditor* Mx400Processor::createEditor() { return new Mx400Editor (*this); }

// ---- settings ----------------------------------------------------------------------------

int Mx400Processor::getInt (const char* k, int def) const { return (int) apvts.state.getProperty (k, def); }
bool Mx400Processor::getBool (const char* k, bool def) const { return (bool) apvts.state.getProperty (k, def); }
juce::String Mx400Processor::getStr (const char* k) const { return apvts.state.getProperty (k).toString(); }

void Mx400Processor::setSetting (const char* k, const juce::var& v)
{
    apvts.state.setProperty (k, v, nullptr);
    refreshAtomics();
}

void Mx400Processor::refreshAtomics()
{
    cfg = juce::jlimit (0, 2, getInt ("config", 1));
    routeViewA = juce::jlimit (1, 5, getInt ("routingA", 2));
    routeViewB = juce::jlimit (1, 5, getInt ("routingB", 2));
    chA = juce::jlimit (1, 16, getInt ("chA", 1));
    chB = juce::jlimit (1, 16, getInt ("chB", 2));
    routeMode = juce::jlimit (0, 2, getInt ("route", 0));
    rate = juce::jlimit (0, 2, getInt ("rate", 1));
    recallOnLoad = getBool ("recallOnLoad", false);
    manual = getBool ("manual", false);
    thru = getBool ("thru", true);
}

// ---- state --------------------------------------------------------------------------------

void Mx400Processor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::StringArray ids;
    for (size_t i = 0; i < touched.size(); ++i)
        if (touched[i].load()) ids.add (juce::String (mxp::table()[i].id));
    apvts.state.setProperty ("touched", ids.joinIntoString (","), nullptr);
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void Mx400Processor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    suspendPolling = true;                    // values that come from the saved project are not user moves
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    refreshAtomics();
    const auto list = juce::StringArray::fromTokens (getStr ("touched"), ",", "");
    for (size_t i = 0; i < touched.size(); ++i)
        touched[i].store (list.contains (juce::String (mxp::table()[i].id)));
    chooseOutput (getStr ("outId"), getStr ("outName"));
    chooseInput (getStr ("inId"), getStr ("inName"));
    needResync = true;
    suspendPolling = false;
}

void Mx400Processor::clearTouched() { for (auto& t : touched) t.store (false); }

// ---- sending ------------------------------------------------------------------------------

int Mx400Processor::ccFor (int pi, int& ch) const
{
    const auto& p = mxp::table()[(size_t) pi];
    const auto c = config();
    ch = chA.load() - 1;
    if (p.kind == mxp::Kind::Program) return -1;
    int program = p.program, effect = p.effect;
    if (effect > 0) program = effect >= 3 ? 1 : 0;
    if (program == 1)
    {
        if (c != Config::DualStereo) return -1;        // program B exists only in Dual Stereo
        ch = chB.load() - 1;
    }
    if (p.key == "globalBypass") return kProgramBypassCc;   // whole program, same CC in every map
    if (c == Config::Surround && effect > 1) return -1;
    if (c == Config::Stereo && effect > 2) return -1;

    const auto& m = mapFor (c, routingView (program));
    const int slot = (c == Config::Surround) ? 0 : (effect == 0 ? 0 : (effect - 1) % 2);
    std::string key = p.key;
    if (key == "type") key = "effectType";
    if (c == Config::Surround && key == "mix") key = "mixFront";
    const int ci = columnIndex (m, key);
    if (ci >= 0) { const int cc = m.columns[(size_t) ci].cc[slot]; return cc == kNone ? -1 : cc; }
    for (const auto& f : m.fixed)
        if (key == f.key) return f.cc[slot] == kNone ? -1 : f.cc[slot];
    return -1;
}

void Mx400Processor::snapshotValues()
{
    for (size_t i = 0; i < raw.size(); ++i) lastSeen[i] = (int) (raw[i]->load() + 0.5f);
}

void Mx400Processor::enqueueParam (int i, int value)
{
    const auto& p = mxp::table()[(size_t) i];
    int ch = 0;
    const int cc = ccFor (i, ch);
    if (cc < 0) return;
    queue.push (makeCC (ch, cc, p.kind == mxp::Kind::Toggle ? (value ? 127 : 0) : value));
}

void Mx400Processor::sendAllTouched() { needSendAll = true; }
void Mx400Processor::resendPrograms() { needPrograms = true; }

void Mx400Processor::run()
{
    while (! threadShouldExit())
    {
        serviceOnce();
        wait (2);
    }
}

void Mx400Processor::serviceOnce()
{
    if (suspendPolling.load()) return;
    const uint64_t now = (uint64_t) ((double) juce::Time::getHighResolutionTicks() * 1.0e6 / (double) juce::Time::getHighResolutionTicksPerSecond());
    queue.setProfile (rate == 0 ? rateFast() : rate == 2 ? rateSlow() : rateNormal());

    auto sendProgram = [this] (int pr)
    {
        if (pr == 1 && config() != Config::DualStereo) return;
        const int ch = (pr == 0 ? chA.load() : chB.load()) - 1;
        queue.push (makeBankSelect (ch, lastSeen[(size_t) idxBank[pr]] != 0));
        queue.push (makeProgramChange (ch, lastSeen[(size_t) idxProg[pr]]));
    };
    auto sendAllNow = [this, &sendProgram]
    {
        const auto& t = mxp::table();
        for (size_t i = 0; i < t.size(); ++i)
            if (t[i].kind != mxp::Kind::Program && touched[i].load()) enqueueParam ((int) i, lastSeen[i]);
        for (int pr = 0; pr < 2; ++pr)
            if (touched[(size_t) idxProg[pr]].load() || touched[(size_t) idxBank[pr]].load()) sendProgram (pr);
    };

    if (needResync.exchange (false))
    {
        snapshotValues();                              // loading a project is not a move
        if (recallOnLoad.load()) sendAllNow();         // unless total recall is switched on
    }
    if (needSendAll.exchange (false)) sendAllNow();
    if (needPrograms.exchange (false)) { sendProgram (0); sendProgram (1); }

    bool progChanged[2] { false, false };
    const auto& t = mxp::table();
    for (size_t i = 0; i < raw.size(); ++i)
    {
        const int v = (int) (raw[i]->load() + 0.5f);
        if (v == lastSeen[i]) continue;
        lastSeen[i] = v;
        touched[i].store (true);
        if (manual.load()) continue;                   // manual mode: only "Send all" sends
        if (t[i].kind == mxp::Kind::Program) progChanged[t[i].program] = true;
        else enqueueParam ((int) i, v);
    }
    for (int pr = 0; pr < 2; ++pr) if (progChanged[pr]) sendProgram (pr);

    MidiMsg m;
    while (queue.pop (now, m)) emit (m);
}

juce::String Mx400Processor::describe (const MidiMsg& m) const
{
    const int ch = (m.b[0] & 0x0F) + 1;
    if ((m.b[0] & 0xF0) == 0xC0) return "ch " + juce::String (ch) + "  program " + juce::String (m.b[1] + 1);
    if (m.b[1] == 32) return "ch " + juce::String (ch) + "  bank " + (m.b[2] ? "User" : "Factory");
    return "ch " + juce::String (ch) + "  CC " + juce::String (m.b[1]) + " = " + juce::String (m.b[2]);
}

void Mx400Processor::emit (const MidiMsg& m)
{
    const int mode = routeMode.load();
    if (mode == Direct || mode == Both)
    {
        const juce::ScopedLock sl (outLock);
        if (midiOut) midiOut->sendMessageNow (juce::MidiMessage (m.b.data(), (int) m.n));
    }
    if (mode == HostOut || mode == Both)
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (1, s1, n1, s2, n2);
        if (n1 > 0) { fifoBuf[(size_t) s1] = m; fifo.finishedWrite (1); }
        else if (n2 > 0) { fifoBuf[(size_t) s2] = m; fifo.finishedWrite (1); }
    }
    ++sent;
    const juce::ScopedLock sl (logLock);
    log.push_back (describe (m).toStdString());
    while (log.size() > 40) log.pop_front();
}

std::vector<std::string> Mx400Processor::lastSent() const
{
    const juce::ScopedLock sl (logLock);
    return { log.begin(), log.end() };
}

// ---- MIDI ports ---------------------------------------------------------------------------

juce::StringArray Mx400Processor::outputNames() const { juce::StringArray a; for (auto& d : juce::MidiOutput::getAvailableDevices()) a.add (d.name); return a; }
juce::StringArray Mx400Processor::outputIds() const   { juce::StringArray a; for (auto& d : juce::MidiOutput::getAvailableDevices()) a.add (d.identifier); return a; }
juce::StringArray Mx400Processor::inputNames() const  { juce::StringArray a; for (auto& d : juce::MidiInput::getAvailableDevices()) a.add (d.name); return a; }
juce::StringArray Mx400Processor::inputIds() const    { juce::StringArray a; for (auto& d : juce::MidiInput::getAvailableDevices()) a.add (d.identifier); return a; }

void Mx400Processor::chooseOutput (const juce::String& id, const juce::String& name)
{
    apvts.state.setProperty ("outId", id, nullptr);
    apvts.state.setProperty ("outName", name, nullptr);
    std::unique_ptr<juce::MidiOutput> opened;
    if (id.isNotEmpty()) opened = juce::MidiOutput::openDevice (id);
    const juce::ScopedLock sl (outLock);
    midiOut = std::move (opened);
    outOpen = (midiOut != nullptr);
}

void Mx400Processor::chooseInput (const juce::String& id, const juce::String& name)
{
    apvts.state.setProperty ("inId", id, nullptr);
    apvts.state.setProperty ("inName", name, nullptr);
    if (midiIn) { midiIn->stop(); midiIn.reset(); }
    inOpen = false;
    if (id.isEmpty()) return;
    midiIn = juce::MidiInput::openDevice (id, this);
    if (midiIn) { midiIn->start(); inOpen = true; }
}

// ---- SysEx capture ------------------------------------------------------------------------

void Mx400Processor::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m)
{
    if (! m.isSysEx()) return;
    std::vector<Bytes> out;
    const juce::ScopedLock sl (capLock);
    assembler.feed (m.getRawData(), (size_t) m.getRawDataSize(), out);
    for (auto& b : out)
    {
        if (caps.size() >= 200) caps.erase (caps.begin());
        caps.push_back ({ juce::Time::getCurrentTime(), std::move (b) });
    }
}

std::vector<Mx400Processor::Capture> Mx400Processor::captures() const { const juce::ScopedLock sl (capLock); return caps; }
void Mx400Processor::clearCaptures() { const juce::ScopedLock sl (capLock); caps.clear(); assembler.reset(); }

juce::File Mx400Processor::dataDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Studio 87").getChildFile ("MX400");
}

juce::File Mx400Processor::saveCapture (int index)
{
    Capture c;
    { const juce::ScopedLock sl (capLock); if (index < 0 || index >= (int) caps.size()) return {}; c = caps[(size_t) index]; }
    auto dir = dataDir().getChildFile ("captures");
    if (! dir.createDirectory().wasOk()) return {};
    auto f = dir.getChildFile ("mx400-" + c.when.formatted ("%Y%m%d-%H%M%S") + "-" + juce::String (index + 1) + ".syx");
    return f.replaceWithData (c.data.data(), c.data.size()) ? f : juce::File();
}

int Mx400Processor::saveAllCaptures()
{
    int n = 0;
    const int total = (int) captures().size();
    for (int i = 0; i < total; ++i) if (saveCapture (i).exists()) ++n;
    return n;
}

std::vector<Bytes> Mx400Processor::readSyxFile (const juce::File& f, bool& ok)
{
    juce::MemoryBlock mb;
    ok = f.existsAsFile() && f.getSize() < 64 * 1024 * 1024 && f.loadFileAsData (mb);
    if (! ok) return {};
    auto parts = splitSyx (Bytes ((const uint8_t*) mb.getData(), (const uint8_t*) mb.getData() + mb.getSize()));
    ok = ! parts.empty();
    return parts;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Mx400Processor(); }
