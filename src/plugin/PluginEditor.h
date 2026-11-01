#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <catui/CatLookAndFeel.h>
#include <catui/RenderGate.h>
#include "PluginProcessor.h"

namespace mxui { class Content; }

class Mx400Editor : public juce::AudioProcessorEditor
{
public:
    explicit Mx400Editor (Mx400Processor&);
    ~Mx400Editor() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void resetTo100();
    void showPage (int page);          // 0 Panel, 1 Library, 2 Surround, 3 Settings (used by the screenshot tool)
    void rebuildAll();

    static constexpr int kW = 1500, kH = 940;
private:
    Mx400Processor& proc;
    std::unique_ptr<mxui::Content> content;
    juce::ComponentBoundsConstrainer constrainer;
    std::unique_ptr<juce::ResizableBorderComponent> border;
    juce::TooltipWindow tooltips { this, 900 };
    std::unique_ptr<juce::LookAndFeel> laf;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mx400Editor)
};
