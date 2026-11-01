// Renders the real editor offscreen to PNG files (docs/screenshot.png and one per page).
// Usage: MX400Screenshots <output folder>
#include "../src/plugin/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::File out = argc > 1 ? juce::File (argv[1]) : juce::File::getCurrentWorkingDirectory();
    out.createDirectory();
    Mx400Processor proc;
    proc.setSetting ("config", 1);
    // a few values so the pictures show the knobs and graphs in use (not sent anywhere: no port is open)
    auto set = [&proc] (const char* id, int v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 ((float) v)); };
    set ("e1_p1", 40); set ("e1_p2", 90); set ("e1_p3", 60); set ("e1_mix", 70); set ("e2_mix", 45);
    set ("e2_p1", 55); set ("e2_p2", 40); set ("e2_p3", 80); set ("a_prog", 25); set ("b_prog", 12);
    proc.setSetting ("effectView", 0);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* e = dynamic_cast<Mx400Editor*> (ed.get());
    e->setVisible (true);
    juce::Thread::sleep (50);
    const char* names[] = { "panel", "library", "surround", "settings" };
    for (int i = 0; i < 4; ++i)
    {
        e->showPage (i);
        juce::Thread::sleep (50);
        auto img = e->createComponentSnapshot (e->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        auto f = out.getChildFile (juce::String ("mx400-") + names[i] + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        if (os.openedOk()) png.writeImageToStream (img, os);
    }
    return 0;
}
