// Processor checks: parameters to MIDI, no send on load, recall, manual mode, programs, coalescing.
// Runs the real processor offline through the DAW MIDI output route (no hardware, no display).
#include "../src/plugin/PluginProcessor.h"
#include <cstdio>

static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

struct Ev { int ch, d1, d2; bool prog; };

static std::vector<Ev> drain (Mx400Processor& p, int waitMs = 120)
{
    juce::Thread::sleep (waitMs);
    std::vector<Ev> out;
    for (int round = 0; round < 6; ++round)
    {
        juce::AudioBuffer<float> buf (2, 256); buf.clear();
        juce::MidiBuffer mb;
        p.processBlock (buf, mb);
        for (auto m : mb)
        {
            const auto msg = m.getMessage();
            if (msg.isController()) out.push_back ({ msg.getChannel(), msg.getControllerNumber(), msg.getControllerValue(), false });
            else if (msg.isProgramChange()) out.push_back ({ msg.getChannel(), msg.getProgramChangeNumber(), 0, true });
        }
        juce::Thread::sleep (20);
    }
    return out;
}

static void setP (Mx400Processor& p, const char* id, int v)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) v));
}

static std::unique_ptr<Mx400Processor> make (int config = 1, int route = 1)
{
    auto p = std::make_unique<Mx400Processor>();
    p->setPlayConfigDetails (2, 2, 44100.0, 256);
    p->setSetting ("config", config);
    p->setSetting ("route", route);           // 1 = DAW MIDI output
    p->setSetting ("rate", 0);                // fast
    drain (*p, 30);                           // let the sender settle
    return p;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    {   // nothing is sent at start
        auto p = make();
        CHECK (drain (*p).empty());
    }
    {   // Dual Stereo, routing view 2 (Dual Stereo map): channel A and B, CC numbers from the map
        auto p = make (1);
        setP (*p, "e1_p1", 100);  auto e = drain (*p);
        CHECK (e.size() == 1 && e[0].ch == 1 && e[0].d1 == 14 && e[0].d2 == 100);
        setP (*p, "e2_mix", 33);  e = drain (*p);
        CHECK (e.size() == 1 && e[0].ch == 1 && e[0].d1 == 27 && e[0].d2 == 33);
        setP (*p, "e3_mix", 44);  e = drain (*p);
        CHECK (e.size() == 1 && e[0].ch == 2 && e[0].d1 == 26 && e[0].d2 == 44);      // program B on channel B
        setP (*p, "e4_p2", 5);    e = drain (*p);
        CHECK (e.size() == 1 && e[0].ch == 2 && e[0].d1 == 22 && e[0].d2 == 5);
        setP (*p, "e1_bypass", 1); e = drain (*p);
        CHECK (e.size() == 1 && e[0].d1 == 13 && e[0].d2 == 127);
        setP (*p, "e1_bypass", 0); e = drain (*p);
        CHECK (e.size() == 1 && e[0].d1 == 13 && e[0].d2 == 0);
        setP (*p, "a_gbypass", 1); e = drain (*p);
        CHECK (e.size() == 1 && e[0].ch == 1 && e[0].d1 == 120 && e[0].d2 == 127);   // program bypass, program A
        setP (*p, "b_gbypass", 1); e = drain (*p);
        CHECK (e.size() == 1 && e[0].ch == 2 && e[0].d1 == 120 && e[0].d2 == 127);   // program B on channel B
        setP (*p, "b_gbypass", 0); e = drain (*p);
        CHECK (e.size() == 1 && e[0].ch == 2 && e[0].d1 == 120 && e[0].d2 == 0);
        setP (*p, "a_gbypass", 0); e = drain (*p);
        setP (*p, "a_routing", 3); e = drain (*p);
        CHECK (e.size() == 1 && e[0].ch == 1 && e[0].d1 == 28 && e[0].d2 == 3);
        setP (*p, "e1_x3", 9);    e = drain (*p);
        CHECK (e.empty());                                                              // Dual Stereo map has no extra 3
    }
    {   // Dual Stereo with routing 5 uses the stereo map
        auto p = make (1);
        p->setSetting ("routingA", 5);
        setP (*p, "e1_x4", 70); auto e = drain (*p);
        CHECK (e.size() == 1 && e[0].d1 == 21 && e[0].d2 == 70);
    }
    {   // Stereo configuration: map p47/48, no program B
        auto p = make (0);
        setP (*p, "e1_mix", 10); setP (*p, "e2_mix", 20); setP (*p, "e3_mix", 30);
        auto e = drain (*p);
        CHECK (e.size() == 2);
        if (e.size() == 2) { CHECK (e[0].d1 == 27 && e[0].d2 == 10); CHECK (e[1].d1 == 54 && e[1].d2 == 20); }
        setP (*p, "e2_x8", 99); e = drain (*p);
        CHECK (e.size() == 1 && e[0].d1 == 52 && e[0].d2 == 99);
        setP (*p, "b_prog", 5); e = drain (*p);
        CHECK (e.empty());                                                              // program B only in Dual Stereo
    }
    {   // Surround
        auto p = make (2);
        setP (*p, "e1_mix", 11); setP (*p, "e1_mixRear", 22); setP (*p, "e2_mix", 33);
        auto e = drain (*p);
        CHECK (e.size() == 2);
        if (e.size() == 2) { CHECK (e[0].d1 == 26 && e[0].d2 == 11); CHECK (e[1].d1 == 27 && e[1].d2 == 22); }
        setP (*p, "e1_assignB", 7); e = drain (*p);
        CHECK (e.size() == 1 && e[0].d1 == 30);
    }
    {   // programs: bank select then program change, on the right channel
        auto p = make (1);
        setP (*p, "a_prog", 7); auto e = drain (*p);
        CHECK (e.size() == 2 && ! e[0].prog && e[0].d1 == 32 && e[0].d2 == 0 && e[1].prog && e[1].d1 == 6 && e[1].ch == 1);
        setP (*p, "b_bank", 1); setP (*p, "b_prog", 99); e = drain (*p);
        CHECK (e.size() == 2 && e[0].d1 == 32 && e[0].d2 == 1 && e[0].ch == 2 && e[1].prog && e[1].d1 == 98 && e[1].ch == 2);
        p->resendPrograms(); e = drain (*p);
        CHECK (e.size() == 4);
    }
    {   // a fast sweep is coalesced and ends on the last value
        auto p = make (1);
        for (int v = 0; v <= 127; ++v) setP (*p, "e1_p2", v);
        auto e = drain (*p, 200);
        CHECK (! e.empty() && e.size() < 20 && e.back().d1 == 15 && e.back().d2 == 127);
    }
    {   // project load is not a move; recall on load sends only touched controls
        juce::MemoryBlock state;
        {
            auto p = make (1);
            setP (*p, "e1_p1", 50); setP (*p, "e2_p3", 60);
            drain (*p);
            p->getStateInformation (state);
        }
        {
            auto p = make (1);
            p->setStateInformation (state.getData(), (int) state.getSize());
            CHECK (drain (*p).empty());                                                  // nothing on load
            CHECK (p->isTouched (mxp::indexOf ("e1_p1")) && p->isTouched (mxp::indexOf ("e2_p3")) && ! p->isTouched (mxp::indexOf ("e1_p2")));
            p->sendAllTouched();
            auto e = drain (*p);
            CHECK (e.size() == 2);
        }
        {
            auto p = make (1);
            p->setSetting ("recallOnLoad", true);
            p->setStateInformation (state.getData(), (int) state.getSize());
            auto e = drain (*p);                                                         // saved state has recallOnLoad false again: nothing
            CHECK (e.empty());
            juce::MemoryBlock st2; p->setSetting ("recallOnLoad", true); p->getStateInformation (st2);
            auto q = make (1);
            q->setStateInformation (st2.getData(), (int) st2.getSize());
            e = drain (*q);
            CHECK (e.size() == 2);                                                       // total recall when switched on
        }
    }
    {   // manual mode: nothing until Send all
        auto p = make (1);
        p->setSetting ("manual", true);
        setP (*p, "e1_p1", 77);
        CHECK (drain (*p).empty());
        p->sendAllTouched();
        auto e = drain (*p);
        CHECK (e.size() == 1 && e[0].d1 == 14 && e[0].d2 == 77);
        p->clearTouched(); p->sendAllTouched();
        CHECK (drain (*p).empty());
    }
    {   // host MIDI pass-through setting
        auto p = make (1);
        juce::AudioBuffer<float> buf (2, 64); buf.clear();
        juce::MidiBuffer in; in.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        p->processBlock (buf, in);
        CHECK (! in.isEmpty());                                                          // passes through by default
        p->setSetting ("thru", false);
        juce::MidiBuffer in2; in2.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        p->processBlock (buf, in2);
        CHECK (in2.isEmpty());
    }

    std::printf ("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
