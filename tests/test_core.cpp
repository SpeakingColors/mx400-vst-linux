// Core checks: no JUCE, no hardware. Run with: sh tests/run.sh
#include "../src/core/Mx400Map.h"
#include "../src/core/Mx400Midi.h"
#include <cstdio>
#include <cstdlib>
#include <set>

static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

using namespace mx400;

static const char* cell(const Map& m, const char* fx, const char* col)
{
    const int e = effectIndex(m, fx), c = columnIndex(m, col);
    if (e < 0 || c < 0) return "<missing>";
    const char* s = m.effects[(size_t) e].cells[(size_t) c];
    return s ? s : "";
}

int main()
{
    // ---- map: spot checks against the printed pages ----
    const auto& ds = dualStereoMap();
    CHECK(ds.effects.size() == 31);
    CHECK(std::string(cell(ds, "Small Hall", "p2")) == "Decay / Mid RT");
    CHECK(std::string(cell(ds, "Gated", "p3")) == "Diffusion");
    CHECK(std::string(cell(ds, "Spring", "p3")) == "Boing");
    CHECK(std::string(cell(ds, "Tape Delay", "p4")) == "Smear");
    CHECK(std::string(cell(ds, "Pong Delay", "p4")) == "Tap Ratio");
    CHECK(std::string(cell(ds, "Rotary", "p2")) == "Doppler");
    CHECK(std::string(cell(ds, "De Esser", "p3")) == "Bandwidth");
    CHECK(std::string(cell(ds, "Small Hall", "tempo")).empty());      // tempo only on delays
    CHECK(std::string(cell(ds, "Studio Delay", "tempo")) == "Tempo");
    CHECK(ds.columns[(size_t) columnIndex(ds, "mix")].cc[0] == 26 && ds.columns[(size_t) columnIndex(ds, "mix")].cc[1] == 27);
    CHECK(ds.columns[(size_t) columnIndex(ds, "bypass")].cc[0] == 13 && ds.columns[(size_t) columnIndex(ds, "bypass")].cc[1] == 20);
    CHECK(ds.columns[(size_t) columnIndex(ds, "tempo")].cc[0] == 18 && ds.columns[(size_t) columnIndex(ds, "tempo")].cc[1] == 25);

    const auto& st = stereoMap();
    CHECK(st.effects.size() == 22);
    CHECK(std::string(cell(st, "Hall Reverbs", "x4")) == "Bass Boost Freq");
    CHECK(std::string(cell(st, "Plate Reverbs", "x7")) == "Feedback Delay");
    CHECK(std::string(cell(st, "Room Reverb", "x4")) == "ER Time");
    CHECK(std::string(cell(st, "2-Tap Delay", "p4")) == "Delay Type");
    CHECK(std::string(cell(st, "2-Tap Delay", "x6")) == "Pan 2");
    CHECK(std::string(cell(st, "Rotary", "p2")) == "Stereo Spread");
    CHECK(std::string(cell(st, "Compressor", "x2")) == "Knee");
    CHECK(std::string(cell(st, "Spring Reverb", "x1")).empty());
    CHECK(st.columns[(size_t) columnIndex(st, "mix")].cc[1] == 54);
    CHECK(st.columns[(size_t) columnIndex(st, "x8")].cc[0] == 25 && st.columns[(size_t) columnIndex(st, "x8")].cc[1] == 52);

    const auto& su = surroundMap();
    CHECK(su.effects.size() == 1);
    CHECK(su.columns[(size_t) columnIndex(su, "mixRear")].cc[0] == 27);
    CHECK(su.fixed.size() == 3 && su.fixed[0].cc[0] == 29 && su.fixed[2].cc[0] == 31);

    // map selection
    CHECK(&mapFor(Config::Stereo) == &stereoMap());
    CHECK(&mapFor(Config::DualStereo, 1) == &dualStereoMap());
    CHECK(&mapFor(Config::DualStereo, 5) == &stereoMap());
    CHECK(&mapFor(Config::Surround) == &surroundMap());

    // CC 120 (program bypass) must not collide with any mapped CC
    CHECK(kProgramBypassCc == 120);
    for (const Map* m : { &ds, &st, &su })
    {
        for (const auto& c : m->columns) for (int v : c.cc) CHECK(v != kProgramBypassCc);
        for (const auto& f : m->fixed)   for (int v : f.cc) CHECK(v != kProgramBypassCc);
    }

    // every CC is a legal 7 bit number, every row has one cell per column
    for (const Map* m : { &ds, &st, &su })
    {
        for (const auto& c : m->columns) for (int v : c.cc) CHECK(v == kNone || (v >= 0 && v <= 127));
        for (const auto& f : m->fixed)   for (int v : f.cc) CHECK(v == kNone || (v >= 0 && v <= 127));
        for (const auto& e : m->effects) CHECK(e.cells.size() == m->columns.size());
    }

    // printed overlaps: Dual Stereo prints 26, 27 and 28 twice; the other maps have none
    {
        std::set<int> ccs; for (const auto& o : printedOverlaps(ds)) ccs.insert(o.cc);
        CHECK(ccs == (std::set<int>{ 26, 27, 28 }));
        CHECK(printedOverlaps(st).empty());
        CHECK(printedOverlaps(su).empty());
    }

    // controlsFor: slot picks the CC of the second effect
    {
        const int hall = effectIndex(st, "Hall Reverbs");
        auto s0 = controlsFor(st, hall, 0), s1 = controlsFor(st, hall, 1);
        auto find = [](const std::vector<Control>& v, const char* k) { for (auto& c : v) if (c.key == k) return c.cc; return -2; };
        CHECK(find(s0, "mix") == 27 && find(s1, "mix") == 54);
        CHECK(find(s0, "p1") == 14 && find(s1, "p1") == 41);
        CHECK(find(s0, "effectType") == 12 && find(s1, "effectType") == 39);
        CHECK(find(s0, "routing") == 28 && find(s1, "routing") == -2);   // routing is one program wide CC, listed on slot 0 only
        CHECK(find(s0, "tempo") == -2);                                   // hall has no tempo
        const int sh = effectIndex(ds, "Small Hall");
        CHECK(find(controlsFor(ds, sh, 1), "assignB") == 30);
        CHECK(controlsFor(st, 999, 0).empty());
        const int surr = effectIndex(su, "Surround Hall");
        CHECK(find(controlsFor(su, surr, 1), "mixFront") == 26);          // slot ignored in Surround
    }

    // ---- message builders ----
    {
        auto m = makeCC(0, 14, 100);
        CHECK(m.n == 3 && m.b[0] == 0xB0 && m.b[1] == 14 && m.b[2] == 100 && ! m.isProgram);
        m = makeCC(15, 200, 300);                       // clamped, channel 16
        CHECK(m.b[0] == 0xBF && m.b[1] == 127 && m.b[2] == 127);
        auto p = makeProgramChange(0, 1);               // program 1 = value 0
        CHECK(p.n == 2 && p.b[0] == 0xC0 && p.b[1] == 0 && p.isProgram);
        p = makeProgramChange(1, 99);                   // program 99 = value 98
        CHECK(p.b[0] == 0xC1 && p.b[1] == 98);
        p = makeProgramChange(0, 150);                  // clamped to 99
        CHECK(p.b[1] == 98);
        p = makeProgramChange(0, 0);                    // clamped to 1
        CHECK(p.b[1] == 0);
        auto b = makeBankSelect(0, false);
        CHECK(b.b[0] == 0xB0 && b.b[1] == 32 && b.b[2] == 0 && b.isProgram);
        b = makeBankSelect(2, true);
        CHECK(b.b[0] == 0xB2 && b.b[1] == 32 && b.b[2] == 1);
        Channels ch; ch.a = 1; ch.b = 9;
        CHECK(channelForEffect(ch, 1) == 0 && channelForEffect(ch, 2) == 0);
        CHECK(channelForEffect(ch, 3) == 8 && channelForEffect(ch, 4) == 8);
    }

    // ---- queue ----
    {
        SendQueue q({ 1000, 50000 });
        MidiMsg out;
        for (int v = 0; v < 100; ++v) q.push(makeCC(0, 14, v));      // a knob sweep
        q.push(makeCC(0, 15, 5));
        CHECK(q.size() == 2);                                         // coalesced to the latest per cc
        CHECK(q.pop(0, out) && out.b[1] == 14 && out.b[2] == 99);
        CHECK(! q.pop(500, out));                                     // too soon
        CHECK(q.pop(1000, out) && out.b[1] == 15);
        CHECK(q.empty());

        // different channels do not coalesce
        q.push(makeCC(0, 14, 1)); q.push(makeCC(1, 14, 2));
        CHECK(q.size() == 2);
        q.clear();

        // program change keeps order, is never coalesced, and holds the queue afterwards
        SendQueue r({ 1000, 50000 });
        r.push(makeBankSelect(0, true)); r.push(makeProgramChange(0, 7)); r.push(makeProgramChange(0, 8));
        r.push(makeCC(0, 14, 64));
        CHECK(r.pop(0, out) && out.b[0] == 0xB0 && out.b[1] == 32);
        CHECK(r.pop(1000, out) && out.b[0] == 0xC0 && out.b[1] == 6);
        CHECK(! r.pop(30000, out));                                   // quiet time after program change
        CHECK(r.pop(51000, out) && out.b[0] == 0xC0 && out.b[1] == 7);
        CHECK(r.pop(101000, out) && out.b[0] == 0xB0 && out.b[1] == 14);
        CHECK(r.empty());
        CHECK(rateFast().minGapUs < rateNormal().minGapUs && rateNormal().minGapUs < rateSlow().minGapUs);
        CHECK(rateFast().minGapUs >= 1000);                           // never faster than DIN wire speed
    }

    // ---- shadow ----
    {
        Shadow s;
        CHECK(s.changed(0, 14, 0));
        s.remember(0, 14, 0);
        CHECK(! s.changed(0, 14, 0) && s.changed(0, 14, 1) && s.changed(1, 14, 0));
        s.forget();
        CHECK(s.changed(0, 14, 0) && s.count() == 0);
    }

    // ---- sysex ----
    {
        Bytes a { 0xF0, 0x01, 0x02, 0x03, 0xF7 };
        Bytes b { 0xF0, 0x7D, 0x10, 0xF7 };
        CHECK(isValidSysex(a) && isValidSysex(b));
        CHECK(! isValidSysex({ 0xF0, 0xF7 }));                        // empty body
        CHECK(! isValidSysex({ 0xF0, 0x01, 0x80, 0xF7 }));            // data byte with high bit
        CHECK(! isValidSysex({ 0x01, 0x02, 0x03 }));
        // file with two messages and junk between them
        Bytes file = joinSyx({ a, b });
        file.insert(file.begin() + (long) a.size(), { 0x55, 0x66 });
        auto parts = splitSyx(file);
        CHECK(parts.size() == 2 && parts[0] == a && parts[1] == b);
        // real time byte inside a message is ignored, stream split across feeds
        SysexAssembler asmb; std::vector<Bytes> got;
        const uint8_t s1[] = { 0xF0, 0x01, 0xF8, 0x02 }; const uint8_t s2[] = { 0x03, 0xF7 };
        asmb.feed(s1, sizeof s1, got); CHECK(got.empty() && asmb.inMessage());
        asmb.feed(s2, sizeof s2, got); CHECK(got.size() == 1 && got[0] == a);
        // a new status byte aborts the open message
        const uint8_t s3[] = { 0xF0, 0x01, 0x90, 0x40, 0x40, 0xF0, 0x05, 0xF7 };
        got.clear(); SysexAssembler a2; a2.feed(s3, sizeof s3, got);
        CHECK(got.size() == 1 && got[0] == (Bytes { 0xF0, 0x05, 0xF7 }) && a2.droppedCount() == 1);
        CHECK(toHex(a) == "F0 01 02 03 F7");
        CHECK(toHex(a, 2) == "F0 01 ...");
        CHECK(manufacturerBytes(a) == "01");
        CHECK(manufacturerBytes({ 0xF0, 0x00, 0x20, 0x33, 0x01, 0xF7 }) == "00 20 33");
        CHECK(diffBytes({ 0xF0, 1, 2, 3, 0xF7 }, { 0xF0, 1, 9, 3, 0xF7 }) == std::vector<size_t>({ 2 }));
        CHECK(diffBytes({ 0xF0, 1, 0xF7 }, { 0xF0, 1, 2, 0xF7 }).empty());
    }

    std::printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
