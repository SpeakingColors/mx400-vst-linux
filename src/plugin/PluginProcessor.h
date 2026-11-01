#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <array>
#include <deque>
#include "Params.h"
#include "../core/Mx400Map.h"
#include "../core/Mx400Midi.h"

// MX400 controller. Audio passes through untouched; the plugin sends MIDI (CC, program change,
// bank select) to the MX400, either straight to a MIDI port or through the host's MIDI output.
// Nothing is sent when a project loads unless "Recall on load" is switched on, and then only
// the controls you have touched.
class Mx400Processor : public juce::AudioProcessor,
                       private juce::Thread,
                       private juce::MidiInputCallback
{
public:
    Mx400Processor();
    ~Mx400Processor() override;

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MX400 Controller"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- settings (saved with the project) ----
    enum Route { Direct = 0, HostOut = 1, Both = 2 };
    int  getInt (const char* key, int def) const;
    bool getBool (const char* key, bool def) const;
    juce::String getStr (const char* key) const;
    void setSetting (const char* key, const juce::var& v);      // message thread

    mx400::Config config() const { return (mx400::Config) cfg.load(); }
    int routingView (int program) const { return (program == 0 ? routeViewA : routeViewB).load(); }
    const mx400::Map& mapForProgram (int program) const { return mx400::mapFor (config(), routingView (program)); }

    // ---- sending ----
    // CC for a parameter under the current configuration, or -1 when the unit has no such control here.
    int ccFor (int paramIndex, int& channel0) const;
    void sendAllTouched();                 // user action: push every control you have touched
    void resendPrograms();                 // user action: bank + program again
    bool isTouched (int paramIndex) const { return touched[(size_t) paramIndex].load(); }
    void clearTouched();
    std::vector<std::string> lastSent() const;     // newest last, for the monitor on the Settings page
    int sentCount() const { return sent.load(); }

    // ---- MIDI ports ----
    juce::StringArray outputNames() const;         // names, same order as outputIds
    juce::StringArray outputIds() const;
    juce::StringArray inputNames() const;
    juce::StringArray inputIds() const;
    void chooseOutput (const juce::String& id, const juce::String& name);
    void chooseInput (const juce::String& id, const juce::String& name);
    bool outputOpen() const { return outOpen.load(); }
    bool inputOpen() const { return inOpen.load(); }

    // ---- SysEx capture (receive only; the MX-Edit SysEx format is not known, so nothing is sent) ----
    struct Capture { juce::Time when; mx400::Bytes data; };
    std::vector<Capture> captures() const;
    void clearCaptures();
    static juce::File dataDir();                   // <app data>/Studio 87/MX400
    juce::File saveCapture (int index);            // writes a .syx file, returns it ({} on failure)
    int saveAllCaptures();

    static std::vector<mx400::Bytes> readSyxFile (const juce::File& f, bool& ok);

private:
    void run() override;
    void serviceOnce();
    void enqueueParam (int i, int value);
    void emit (const mx400::MidiMsg&);
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage&) override;
    void refreshAtomics();
    void snapshotValues();
    juce::String describe (const mx400::MidiMsg&) const;

    std::vector<std::atomic<float>*> raw;
    std::vector<int> lastSeen;
    int idxBank[2] { -1, -1 }, idxProg[2] { -1, -1 };
    std::vector<std::atomic<bool>> touched;
    mx400::SendQueue queue;

    std::atomic<int> cfg { 1 }, routeViewA { 2 }, routeViewB { 2 }, chA { 1 }, chB { 2 }, routeMode { 0 }, rate { 1 };
    std::atomic<bool> recallOnLoad { false }, manual { false }, thru { true };
    std::atomic<bool> needResync { true }, needSendAll { false }, needPrograms { false }, suspendPolling { false };
    std::atomic<int> sent { 0 };

    juce::CriticalSection outLock;
    std::unique_ptr<juce::MidiOutput> midiOut;
    std::unique_ptr<juce::MidiInput> midiIn;
    std::atomic<bool> outOpen { false }, inOpen { false };

    static constexpr int kFifo = 2048;
    juce::AbstractFifo fifo { kFifo };
    std::array<mx400::MidiMsg, kFifo> fifoBuf;

    mutable juce::CriticalSection logLock;
    std::deque<std::string> log;

    mutable juce::CriticalSection capLock;
    std::vector<Capture> caps;
    mx400::SysexAssembler assembler;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mx400Processor)
};
