#pragma once
// MX400 MIDI CC maps, transcribed from the owner's manual (Lexicon part 18-0446-A, 2006),
// MIDI CC maps on printed pages 46 to 49. Pure C++17, no JUCE.
//
// Status of every entry: "official only" (printed in the manual, not yet checked on the
// real unit). Value ranges and enum encodings are NOT printed, so every CC value here is
// the raw 0..127 CC value and nothing more.
#include <cstdint>
#include <string>
#include <vector>

namespace mx400
{
enum class Config { Stereo = 0, DualStereo = 1, Surround = 2 };

// The three printed CC maps. Stereo configuration and Dual Stereo with routing 5 (Stereo)
// use the "routing 5 / stereo effects" map (pp. 47 and 48); Dual Stereo with routing 1 to 4
// uses the Dual Stereo map (p. 46); Surround has its own (p. 49).
enum class MapKind { DualStereo, Stereo, Surround };

constexpr int kNone = -1;
// Printed under every CC map (pp. 46 to 49): "Program (Global) Bypass", MIDI CC 120. Same CC in every map.
constexpr int kProgramBypassCc = 120;

// One column of a map: a parameter slot shared by every effect row.
// cc[0] drives the first effect (Effect 1, or 3 on channel B), cc[1] the second (2, or 4).
struct Column
{
    const char* key;    // stable id, e.g. "mix", "p1", "x3"
    const char* label;  // generic label when no effect is chosen
    int cc[2];          // kNone when the slot has no second CC
    bool isCommon;      // true for mix/bypass/tempo (shown for every effect)
};

struct EffectRow
{
    const char* name;
    const char* group;                 // Reverb, Delay, Modulation, Dynamics
    std::vector<const char*> cells;    // one label per column, nullptr when the cell is empty
};

struct FixedControl
{
    const char* key;    // "assignA", "assignB", "assignC", "effectType", "routing"
    const char* label;
    int cc[2];
};

struct Map
{
    MapKind kind;
    const char* title;
    const char* page;                  // printed page(s)
    std::vector<Column> columns;
    std::vector<EffectRow> effects;
    std::vector<FixedControl> fixed;
};

inline const Map& dualStereoMap()
{
    static const Map m{
        MapKind::DualStereo, "Dual Stereo effects (routing 1 to 4)", "46",
        {
            { "mix",    "Mix",     { 26, 27 }, true },
            { "bypass", "Bypass",  { 13, 20 }, true },
            { "tempo",  "Tempo",   { 18, 25 }, true },
            { "p1",     "Param 1", { 14, 21 }, false },
            { "p2",     "Param 2", { 15, 22 }, false },
            { "p3",     "Param 3", { 16, 23 }, false },
            { "p4",     "Param 4", { 17, 24 }, false },
        },
        {
            // columns:                         mix     bypass  tempo    p1                p2                  p3               p4
            { "Small Hall",     "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Large Hall",     "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Small Plate",    "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Large Plate",    "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Chamber",        "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Room",           "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Studio",         "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Ambience",       "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Vocal Hall",     "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Vocal Plate",    "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Drum Hall",      "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Drum Plate",     "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Arena",          "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay / Mid RT", "Liveliness", nullptr } },
            { "Gated",          "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay", "Diffusion", nullptr } },
            { "Reverse",        "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay", "Diffusion", nullptr } },
            { "Spring",         "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay", "Boing", nullptr } },

            { "Studio Delay",     "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "Ducker Threshold" } },
            { "Mono Delay",       "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "Ducker Threshold" } },
            { "Tape Delay",       "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "Smear" } },
            { "Pong Delay",       "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "Tap Ratio" } },
            { "Modulated Delay",  "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "Modulation Depth" } },
            { "Reverse Delay",    "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "Smear" } },

            { "Chorus",         "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Depth", "Voices", nullptr } },
            { "Flanger",        "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Depth", "Regen", nullptr } },
            { "Phaser",         "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Depth", "Regen", nullptr } },
            { "Tremolo / Pan",  "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Depth", "Phase", nullptr } },
            { "Rotary",         "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Doppler", "Stereo Spread", nullptr } },
            { "Vibrato",        "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Depth", "Phase", nullptr } },
            { "Pitch Shift",    "Modulation", { "Mix", "Bypass", nullptr, "Shift Amount", "Down/Up Shift", "High Pass Filter", nullptr } },
            { "Detune",         "Modulation", { "Mix", "Bypass", nullptr, "Shift Amount", "Shift Amount", "Stereo Spread", nullptr } },
            { "De Esser",       "Dynamics",   { "Mix", "Bypass", nullptr, "Ratio", "Frequency", "Bandwidth", nullptr } },
        },
        {
            { "assignA",    "Assign A",    { 26, 29 } },
            { "assignB",    "Assign B",    { 27, 30 } },
            { "assignC",    "Assign C",    { 28, 31 } },
            { "effectType", "Effect Type", { 12, 19 } },
            { "routing",    "Routing",     { 28, kNone } },
        }
    };
    return m;
}

inline const Map& stereoMap()
{
    static const Map m{
        MapKind::Stereo, "Stereo effects (Stereo configuration, Dual Stereo routing 5)", "47 and 48",
        {
            { "mix",    "Mix",     { 27, 54 }, true },
            { "bypass", "Bypass",  { 13, 40 }, true },
            { "tempo",  "Tempo",   { 26, 53 }, true },
            { "p1",     "Param 1", { 14, 41 }, false },
            { "p2",     "Param 2", { 15, 42 }, false },
            { "p3",     "Param 3", { 16, 43 }, false },
            { "p4",     "Param 4", { 17, 44 }, false },
            { "x1",     "Extra 1", { 18, 45 }, false },
            { "x2",     "Extra 2", { 19, 46 }, false },
            { "x3",     "Extra 3", { 20, 47 }, false },
            { "x4",     "Extra 4", { 21, 48 }, false },
            { "x5",     "Extra 5", { 22, 49 }, false },
            { "x6",     "Extra 6", { 23, 50 }, false },
            { "x7",     "Extra 7", { 24, 51 }, false },
            { "x8",     "Extra 8", { 25, 52 }, false },
        },
        {
            // mix bypass tempo p1 p2 p3 p4 x1..x8
            { "Hall Reverbs", "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Mid RT", "Size", "Shape",
                "Spread", "RT High Cut", "High Cut", "Bass Boost Freq", "Bass Boost", "Diffusion", "ER Time", "ER Level" } },
            { "Plate Reverbs", "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Mid RT", "Size", "Shape",
                "Spread", "RT High Cut", "High Cut", "Bass Boost Freq", "Bass Boost", "Diffusion", "Feedback Delay", "Feedback Level" } },
            { "Chamber Reverb", "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Mid RT", "Size", "Shape",
                "Spread", "RT High Cut", "High Cut", "Bass Boost Freq", "Bass Boost", "Diffusion", "ER Time", "ER Level" } },
            { "Room Reverb", "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Mid RT", "Diffusion", "Shape",
                "Spread", "RT High Cut", "High Cut", "ER Time", "ER Level", nullptr, nullptr, nullptr } },
            { "Ambience Reverb", "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Size", "Diffusion", "Shape",
                "Spread", "RT High Cut", "High Cut", nullptr, nullptr, nullptr, nullptr, nullptr } },
            { "Gated / Reverse Reverb", "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay", "Diffusion", nullptr,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } },
            { "Spring Reverb", "Reverb", { "Mix", "Bypass", nullptr, "Pre Delay", "Decay", "Boing", nullptr,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } },

            { "Stereo Studio Delay", "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "High Pass Filter",
                "Low Pass Filter", "Ducker Level", "Ducker Thresh", nullptr, nullptr, nullptr, nullptr, nullptr } },
            { "Mono Delay", "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "High Pass Filter",
                "Low Pass Filter", "Ducker Level", "Ducker Thresh", "Pan", nullptr, nullptr, nullptr, nullptr } },
            { "Pong Delay", "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "High Pass Filter",
                "Low Pass Filter", "Ducker Level", "Ducker Thresh", "Tap Ratio", "Tap Swap", nullptr, nullptr, nullptr } },
            { "2-Tap Delay", "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "Delay Type",
                "Tap 1", "Level 1", "Pan 1", "Tap 2", "Level 2", "Pan 2", nullptr, nullptr } },
            { "Modulated Delay", "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "High Pass Filter",
                "Low Pass Filter", "Mod Depth", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } },
            { "Tape / Reverse Delay", "Delay", { "Mix", "Bypass", "Tempo", "Tempo", "Delay Time", "Feedback", "Pan",
                "Smear", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } },

            { "Chorus", "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Depth", "Voices", "Pre Delay",
                "Waveform", "Diffusion", "High Pass Filter", nullptr, nullptr, nullptr, nullptr, nullptr } },
            { "Flanger", "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Depth", "Regen", "Waveform",
                "PreDelay", "Phase", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } },
            { "Phaser", "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Depth", "Regen", "Waveform",
                "Phase Stages", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } },
            { "Tremolo / Panner", "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Depth", "Phase", "Waveform",
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } },
            { "Rotary", "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Stereo Spread", "Drive", "Rotor Minimum",
                "Rotor Maximum", "Horn Minimum", "Horn Maximum", "Doppler", nullptr, nullptr, nullptr, nullptr } },
            { "Vibrato", "Modulation", { "Mix", "Bypass", nullptr, "Speed", "Depth", "Phase", "Waveform",
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } },
            { "Pitch Shift / Detune", "Modulation", { "Mix", "Bypass", nullptr, "Shift 1", "Delay 1", "Feedback 1", "Shift 2",
                "Delay 2", "Feedback 2", "Pan 1", "Pan 2", nullptr, nullptr, nullptr, nullptr } },
            { "DeEsser", "Dynamics", { "Mix", "Bypass", nullptr, "Ratio", "Frequency", "Bandwidth", nullptr,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } },
            { "Compressor", "Dynamics", { "Mix", "Bypass", nullptr, "Ratio", "Threshold", "Gain", "Attack",
                "Release", "Knee", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } },
        },
        {
            { "assignA",    "Assign A",    { 29, 55 } },
            { "assignB",    "Assign B",    { 30, 56 } },
            { "assignC",    "Assign C",    { 31, 57 } },
            { "effectType", "Effect Type", { 12, 39 } },
            { "routing",    "Routing",     { 28, kNone } },
        }
    };
    return m;
}

inline const Map& surroundMap()
{
    // One effect (Surround Hall). cc[1] is unused except for the printed "Effect 2" assign row.
    static const Map m{
        MapKind::Surround, "Surround Hall", "49",
        {
            { "bypass",   "Bypass",          { 13, kNone }, true },
            { "mixFront", "Mix Front",       { 26, kNone }, true },
            { "mixRear",  "Mix Rear",        { 27, kNone }, true },
            { "routing",  "Routing",         { 28, kNone }, true },
            { "p1",       "Pre Delay",       { 14, kNone }, false },
            { "p2",       "Mid RT",          { 15, kNone }, false },
            { "p3",       "Size",            { 16, kNone }, false },
            { "x1",       "Spread",          { 18, kNone }, false },
            { "x2",       "RT High Cut",     { 19, kNone }, false },
            { "x3",       "High Cut",        { 20, kNone }, false },
            { "x4",       "Bass Boost Freq", { 21, kNone }, false },
            { "x5",       "Bass Boost",      { 22, kNone }, false },
            { "x6",       "Diffusion",       { 23, kNone }, false },
            { "x7",       "ER Time",         { 24, kNone }, false },
            { "x8",       "ER Level",        { 25, kNone }, false },
        },
        {
            { "Surround Hall", "Reverb", { "Bypass", "Mix Front", "Mix Rear", "Routing", "Pre Delay", "Mid RT", "Size",
                "Spread", "RT High Cut", "High Cut", "Bass Boost Freq", "Bass Boost", "Diffusion", "ER Time", "ER Level" } },
        },
        {
            // Printed under the heading "Effect 2" on p. 49 (Surround has a single reverb effect).
            { "assignA", "Assign A", { 29, kNone } },
            { "assignB", "Assign B", { 30, kNone } },
            { "assignC", "Assign C", { 31, kNone } },
        }
    };
    return m;
}

// Stereo configuration: always the Stereo map. Dual Stereo: routing 5 (value 5) uses the
// Stereo map, routing 1 to 4 the Dual Stereo map. Surround: the Surround map.
inline const Map& mapFor(Config cfg, int routing = 1)
{
    switch (cfg)
    {
        case Config::Surround:   return surroundMap();
        case Config::Stereo:     return stereoMap();
        case Config::DualStereo: return routing == 5 ? stereoMap() : dualStereoMap();
    }
    return stereoMap();
}

// Index of a column by key, or -1.
inline int columnIndex(const Map& m, const std::string& key)
{
    for (size_t i = 0; i < m.columns.size(); ++i)
        if (key == m.columns[i].key) return (int) i;
    return -1;
}

inline int effectIndex(const Map& m, const std::string& name)
{
    for (size_t i = 0; i < m.effects.size(); ++i)
        if (name == m.effects[i].name) return (int) i;
    return -1;
}

// A control the UI can show for one effect slot (0 = Effect 1/3, 1 = Effect 2/4).
struct Control
{
    std::string key;    // column key or fixed key
    std::string label;  // effect specific label
    int cc;             // CC number to send
    bool fixed;         // true for assign / effect type / routing
};

// Controls for an effect row on a slot. Empty cells are skipped. In the Surround map the slot is ignored.
inline std::vector<Control> controlsFor(const Map& m, int effect, int slot)
{
    std::vector<Control> out;
    if (effect < 0 || effect >= (int) m.effects.size()) return out;
    const auto& row = m.effects[(size_t) effect];
    for (size_t i = 0; i < m.columns.size() && i < row.cells.size(); ++i)
    {
        if (row.cells[i] == nullptr) continue;
        const int cc = m.columns[i].cc[m.kind == MapKind::Surround ? 0 : (slot != 0 ? 1 : 0)];
        if (cc == kNone) continue;
        out.push_back({ m.columns[i].key, row.cells[i], cc, false });
    }
    for (const auto& f : m.fixed)
    {
        const int cc = f.cc[m.kind == MapKind::Surround ? 0 : (slot != 0 ? 1 : 0)];
        if (cc == kNone) continue;
        out.push_back({ f.key, f.label, cc, true });
    }
    return out;
}

// CCs the manual prints for two different meanings inside one map (a conflict in the print).
struct Overlap { int cc; std::string a, b; };

inline std::vector<Overlap> printedOverlaps(const Map& m)
{
    struct Use { int cc; std::string what; };
    std::vector<Use> uses;
    for (const auto& c : m.columns)
        for (int s = 0; s < 2; ++s)
            if (c.cc[s] != kNone) uses.push_back({ c.cc[s], std::string(c.key) + (s ? " (2)" : " (1)") });
    for (const auto& f : m.fixed)
        for (int s = 0; s < 2; ++s)
            if (f.cc[s] != kNone) uses.push_back({ f.cc[s], std::string(f.key) + (s ? " (2)" : " (1)") });
    std::vector<Overlap> out;
    for (size_t i = 0; i < uses.size(); ++i)
        for (size_t j = i + 1; j < uses.size(); ++j)
            if (uses[i].cc == uses[j].cc) out.push_back({ uses[i].cc, uses[i].what, uses[j].what });
    return out;
}
}
