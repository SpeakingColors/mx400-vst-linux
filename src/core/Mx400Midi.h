#pragma once
// MX400 MIDI messages, send queue and SysEx helpers. Pure C++17, no JUCE.
//
// Rules this file follows (hardware-protocol-capture skill):
//  - only messages the manual documents are built here (CC, program change, bank select CC 32);
//  - SysEx is only framed, validated, split and stored; the MX-Edit SysEx format is NOT known
//    and nothing here builds or sends an MX400 SysEx message;
//  - nothing is sent on its own: the owner of the queue decides when to push.
#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace mx400
{
struct MidiMsg
{
    std::array<uint8_t, 3> b { 0, 0, 0 };
    uint8_t n = 0;                    // number of valid bytes (2 or 3)
    bool isProgram = false;           // program change or bank select: never coalesced, keeps order
    bool operator==(const MidiMsg& o) const { return n == o.n && b == o.b; }
};

inline uint8_t clamp7(int v) { return (uint8_t) std::min(127, std::max(0, v)); }
inline uint8_t clampCh(int ch) { return (uint8_t) std::min(15, std::max(0, ch)); }   // 0 based

inline MidiMsg makeCC(int ch, int cc, int value)
{
    MidiMsg m; m.n = 3;
    m.b = { (uint8_t) (0xB0 | clampCh(ch)), clamp7(cc), clamp7(value) };
    return m;
}

// Chart p. 45: program change 0 to 98 = programs 1 to 99.
inline MidiMsg makeProgramChange(int ch, int program1to99)
{
    MidiMsg m; m.n = 2; m.isProgram = true;
    m.b = { (uint8_t) (0xC0 | clampCh(ch)), clamp7(std::min(98, std::max(0, program1to99 - 1))), 0 };
    return m;
}

// Chart p. 45: bank select is MIDI CC 32, value 0 = Factory, 1 = User.
inline MidiMsg makeBankSelect(int ch, bool user)
{
    MidiMsg m = makeCC(ch, 32, user ? 1 : 0);
    m.isProgram = true;
    return m;
}

// Channel handling. The unit's "MIDI Channel A / B" can be set to 1 to 16, All, or Off.
// The plugin always sends on a concrete channel chosen by the user; it must match the unit.
struct Channels
{
    int a = 1;   // 1 to 16, MIDI Channel A (Effects 1 and 2, the Stereo and Surround programs)
    int b = 2;   // 1 to 16, MIDI Channel B (Dual Stereo only: Effects 3 and 4, program B)
};

// slot 0..3 = Effect 1..4. In Dual Stereo effects 3 and 4 are program B on channel B.
inline int channelForEffect(const Channels& c, int effect1to4)
{
    return (effect1to4 >= 3 ? c.b : c.a) - 1;
}

// ---- Send queue -----------------------------------------------------------------------

struct RateProfile
{
    uint32_t minGapUs;        // minimum gap between two messages
    uint32_t afterProgramUs;  // quiet time after a program change (the unit is loading)
};
// 5-pin DIN runs at 31250 baud: a 3 byte message needs about 960 us on the wire, so even
// "fast" is above wire speed. The quiet time after a program change is a safe guess, not a
// measured value (verify on hardware).
inline RateProfile rateFast()   { return { 2000, 60000 }; }
inline RateProfile rateNormal() { return { 5000, 120000 }; }
inline RateProfile rateSlow()   { return { 12000, 250000 }; }

// Parameter messages are coalesced: only the latest value per (channel, cc) waits in the
// queue, so a fast knob sweep never floods the unit. Program and bank messages keep their order.
class SendQueue
{
public:
    explicit SendQueue(RateProfile p = rateNormal()) : profile(p) {}

    void setProfile(RateProfile p) { profile = p; }

    void push(const MidiMsg& m)
    {
        if (m.isProgram) { ordered.push_back(m); return; }
        const int key = (m.b[0] & 0x0F) * 128 + m.b[1];
        for (auto& e : pending)
            if (e.first == key) { e.second = m; return; }
        pending.emplace_back(key, m);
    }

    bool empty() const { return ordered.empty() && pending.empty(); }
    size_t size() const { return ordered.size() + pending.size(); }
    void clear() { ordered.clear(); pending.clear(); }

    // Call often with a monotonic clock in microseconds. Returns true and fills `out` when a
    // message may go on the wire now.
    bool pop(uint64_t nowUs, MidiMsg& out)
    {
        if (nowUs < nextAllowedUs) return false;
        if (! ordered.empty())
        {
            out = ordered.front(); ordered.pop_front();
            nextAllowedUs = nowUs + (out.b[0] >= 0xC0 && out.b[0] < 0xD0 ? profile.afterProgramUs : profile.minGapUs);
            return true;
        }
        if (! pending.empty())
        {
            out = pending.front().second; pending.erase(pending.begin());
            nextAllowedUs = nowUs + profile.minGapUs;
            return true;
        }
        return false;
    }

private:
    RateProfile profile;
    std::deque<MidiMsg> ordered;
    std::vector<std::pair<int, MidiMsg>> pending;
    uint64_t nextAllowedUs = 0;
};

// ---- Shadow of what the plugin believes the unit holds ----------------------------------
// The MX400 reports nothing back over CC, so this is an assumption, not a readback.
class Shadow
{
public:
    // Returns true when the value differs from what was last sent on that (channel, cc).
    bool changed(int ch, int cc, int value) const
    {
        auto it = last.find(ch * 128 + cc);
        return it == last.end() || it->second != value;
    }
    void remember(int ch, int cc, int value) { last[ch * 128 + cc] = value; }
    void forget() { last.clear(); }
    size_t count() const { return last.size(); }
private:
    std::map<int, int> last;
};

// ---- SysEx ------------------------------------------------------------------------------

using Bytes = std::vector<uint8_t>;

// F0 ... F7 with every data byte below 0x80 and at least one data byte.
inline bool isValidSysex(const Bytes& m)
{
    if (m.size() < 3 || m.front() != 0xF0 || m.back() != 0xF7) return false;
    for (size_t i = 1; i + 1 < m.size(); ++i)
        if (m[i] & 0x80) return false;
    return true;
}

// Splits a byte stream (a .syx file, or a raw capture) into complete SysEx messages.
// Real-time bytes (0xF8 to 0xFF) are ignored anywhere. A stray status byte aborts the open message.
class SysexAssembler
{
public:
    // Feed bytes; complete, valid messages are appended to `out`.
    void feed(const uint8_t* data, size_t len, std::vector<Bytes>& out)
    {
        for (size_t i = 0; i < len; ++i)
        {
            const uint8_t c = data[i];
            if (c >= 0xF8) continue;                     // real time: may be interleaved
            if (c == 0xF0) { cur.clear(); cur.push_back(c); open = true; continue; }
            if (! open) continue;
            if (c == 0xF7)
            {
                cur.push_back(c); open = false;
                if (isValidSysex(cur)) out.push_back(cur); else ++dropped;
                cur.clear(); continue;
            }
            if (c & 0x80) { open = false; cur.clear(); ++dropped; continue; }  // other status: abort
            cur.push_back(c);
        }
    }
    bool inMessage() const { return open; }
    int droppedCount() const { return dropped; }
    void reset() { cur.clear(); open = false; }
private:
    Bytes cur;
    bool open = false;
    int dropped = 0;
};

inline std::vector<Bytes> splitSyx(const Bytes& file)
{
    SysexAssembler a; std::vector<Bytes> out;
    a.feed(file.data(), file.size(), out);
    return out;
}

inline Bytes joinSyx(const std::vector<Bytes>& msgs)
{
    Bytes out;
    for (const auto& m : msgs) out.insert(out.end(), m.begin(), m.end());
    return out;
}

// "F0 00 01 ..." for logs and the capture list. maxBytes limits long dumps (0 = all).
inline std::string toHex(const Bytes& m, size_t maxBytes = 0)
{
    static const char* d = "0123456789ABCDEF";
    std::string s;
    const size_t n = (maxBytes == 0) ? m.size() : std::min(maxBytes, m.size());
    for (size_t i = 0; i < n; ++i)
    {
        if (i) s += ' ';
        s += d[m[i] >> 4]; s += d[m[i] & 15];
    }
    if (n < m.size()) s += " ...";
    return s;
}

// The first bytes after F0 identify the maker (one byte, or 00 plus two bytes). Shown as raw hex only.
inline std::string manufacturerBytes(const Bytes& m)
{
    if (! isValidSysex(m)) return {};
    const size_t n = (m[1] == 0x00) ? 3 : 1;
    if (m.size() < 1 + n + 1) return {};
    return toHex(Bytes(m.begin() + 1, m.begin() + 1 + (long) n));
}

// Byte-level diff of two equal-length messages: indexes that differ. Used by the capture
// guide (change one parameter on the unit, dump twice, diff) to find where a value lives.
inline std::vector<size_t> diffBytes(const Bytes& a, const Bytes& b)
{
    std::vector<size_t> d;
    if (a.size() != b.size()) return d;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i]) d.push_back(i);
    return d;
}
}
