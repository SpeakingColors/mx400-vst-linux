#pragma once
// Parameter table of the MX400 controller. Every value is the raw MIDI value 0..127 that goes
// on the wire: the manual does not print value ranges or enum encodings, so nothing is scaled.
#include <string>
#include <vector>

namespace mxp
{
enum class Kind { Cc, Toggle, Program };

struct Info
{
    std::string id;     // APVTS id, stable for saved projects: never rename
    std::string name;   // name the DAW shows for automation
    std::string key;    // column key in the CC map ("mix", "p1", "x3", "assignA", "type", "routing", ...)
    int effect = 0;     // 1..4 for effect parameters, 0 for program wide ones
    int program = 0;    // 0 = program A, 1 = program B (for routing and program parameters)
    Kind kind = Kind::Cc;
    int def = 0;
};

inline const std::vector<Info>& table()
{
    static const std::vector<Info> t = []
    {
        std::vector<Info> v;
        const char* cc[] = { "mix", "mixRear", "tempo", "p1", "p2", "p3", "p4",
                             "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8",
                             "assignA", "assignB", "assignC", "type" };
        const char* nm[] = { "Mix", "Mix Rear", "Tempo", "Param 1", "Param 2", "Param 3", "Param 4",
                             "Extra 1", "Extra 2", "Extra 3", "Extra 4", "Extra 5", "Extra 6", "Extra 7", "Extra 8",
                             "Assign A", "Assign B", "Assign C", "Effect Type (raw)" };
        for (int e = 1; e <= 4; ++e)
        {
            for (size_t i = 0; i < sizeof cc / sizeof *cc; ++i)
            {
                Info p; p.id = "e" + std::to_string(e) + "_" + cc[i];
                p.name = "Effect " + std::to_string(e) + " " + nm[i];
                p.key = cc[i]; p.effect = e;
                v.push_back(p);
            }
            Info b; b.id = "e" + std::to_string(e) + "_bypass"; b.name = "Effect " + std::to_string(e) + " Bypass";
            b.key = "bypass"; b.effect = e; b.kind = Kind::Toggle; v.push_back(b);
        }
        for (int pr = 0; pr < 2; ++pr)
        {
            const std::string L = pr == 0 ? "a" : "b", N = pr == 0 ? "Program A " : "Program B ";
            Info r; r.id = L + "_routing"; r.name = N + "Routing (raw)"; r.key = "routing"; r.program = pr; v.push_back(r);
            Info gb; gb.id = L + "_gbypass"; gb.name = N + "Bypass (whole program)"; gb.key = "globalBypass"; gb.program = pr; gb.kind = Kind::Toggle; v.push_back(gb);
            Info bk; bk.id = L + "_bank"; bk.name = N + "Bank (0 Factory, 1 User)"; bk.key = "bank"; bk.program = pr; bk.kind = Kind::Program; v.push_back(bk);
            Info pg; pg.id = L + "_prog"; pg.name = N + "Program"; pg.key = "prog"; pg.program = pr; pg.kind = Kind::Program; pg.def = 1; v.push_back(pg);
        }
        return v;
    }();
    return t;
}

inline int indexOf(const std::string& id)
{
    const auto& t = table();
    for (size_t i = 0; i < t.size(); ++i) if (t[i].id == id) return (int) i;
    return -1;
}
}
