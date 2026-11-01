#include "PluginEditor.h"
#include <catui/Logo.h>
#include <catui/SodaPop.h>
#include <cmath>

using namespace juce;
using namespace mx400;

namespace mxui
{
// ---- look: Neon Navy (the FX plugin look), PurrOne wood cheeks --------------------------------
namespace C
{
    static Colour page()     { return Colour (0xff161638); }
    static Colour panel()    { return Colour (0xff2C2C62); }
    static Colour panelDark(){ return Colour (0xff1c1c4a); }
    static Colour graph()    { return Colour (0xff05060d); }
    static Colour line()     { return Colour (0xff33403f); }
    static Colour text()     { return Colour (0xffe8e6e1); }
    static Colour muted()    { return Colour (0xff9a9690); }
    static Colour title()    { return Colour (0xffE1CBF4); }
    static Colour amber()    { return catui::SodaPop::amber(); }
    static Colour mint()     { return catui::SodaPop::mint(); }
    static Colour sky()      { return catui::SodaPop::popSky(); }
    static Colour on()       { return Colour (0xff5b7fe8); }
}

struct Look : catui::CatLookAndFeel
{
    Look()
    {
        setColour (ResizableWindow::backgroundColourId, C::page());
        setColour (Label::textColourId, C::text());
        setColour (TextButton::buttonColourId, C::panelDark());
        setColour (TextButton::buttonOnColourId, C::on());
        setColour (TextButton::textColourOffId, C::text());
        setColour (TextButton::textColourOnId, Colours::white);
        setColour (ComboBox::backgroundColourId, C::panelDark());
        setColour (ComboBox::textColourId, C::text());
        setColour (ComboBox::outlineColourId, Colour (0xff4a4f8a));
        setColour (ComboBox::arrowColourId, C::title());
        setColour (PopupMenu::backgroundColourId, Colour (0xff1c1c4a));
        setColour (PopupMenu::textColourId, C::text());
        setColour (PopupMenu::highlightedBackgroundColourId, C::on());
        setColour (TextEditor::backgroundColourId, C::graph());
        setColour (TextEditor::textColourId, C::text());
        setColour (TextEditor::outlineColourId, Colour (0xff4a4f8a));
        setColour (ListBox::backgroundColourId, C::graph());
        setColour (TooltipWindow::backgroundColourId, Colour (0xff0e0e26));
        setColour (TooltipWindow::textColourId, C::text());
        setColour (TooltipWindow::outlineColourId, C::amber());
    }

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider&) override
    {
        auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
        const float d = jmin (r.getWidth(), r.getHeight());
        r = r.withSizeKeepingCentre (d, d);
        const float angle = a0 + pos * (a1 - a0);
        g.setColour (Colour (0xff0b0b20)); g.fillEllipse (r.expanded (2.0f));
        ColourGradient cg (Colour (0xff3a3a52), r.getX(), r.getY(), Colour (0xff0e0e18), r.getRight(), r.getBottom(), false);
        g.setGradientFill (cg); g.fillEllipse (r);
        Path arc; arc.addCentredArc (r.getCentreX(), r.getCentreY(), d * 0.5f + 1.0f, d * 0.5f + 1.0f, 0.0f, a0, angle, true);
        g.setColour (C::amber()); g.strokePath (arc, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        Path tick; tick.addRoundedRectangle (-1.5f, -d * 0.5f + 3.0f, 3.0f, d * 0.30f, 1.2f);
        g.setColour (Colours::white); g.fillPath (tick, AffineTransform::rotation (angle).translated (r.getCentre()));
    }

    void drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (over ? C::panelDark().brighter (0.15f) : C::panelDark()); g.fillRoundedRectangle (r, 4.0f);
        g.setColour (Colour (0xff4a4f8a)); g.drawRoundedRectangle (r, 4.0f, 1.0f);
        auto led = Rectangle<float> (r.getX() + 8.0f, r.getCentreY() - 5.0f, 14.0f, 10.0f);
        g.setColour (b.getToggleState() ? C::mint() : Colour (0xff2d3060)); g.fillRoundedRectangle (led, 2.0f);
        g.setColour (C::text());
        g.setFont (FontOptions (13.0f));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (28.0f).reduced (2.0f, 0.0f), Justification::centredLeft, true);
    }
};

static void titleText (Graphics& g, const String& s, Rectangle<int> r)
{
    g.setColour (C::title()); g.setFont (FontOptions (14.0f, Font::bold));
    g.drawText (s.toUpperCase(), r, Justification::centredLeft);
}

static void panelBox (Graphics& g, Rectangle<int> r, const String& title)
{
    g.setColour (C::panel()); g.fillRoundedRectangle (r.toFloat(), 6.0f);
    g.setColour (Colour (0xff3d4278)); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 6.0f, 1.0f);
    if (title.isNotEmpty()) { titleText (g, title, Rectangle<int> (r.getX() + 14, r.getY() + 8, r.getWidth() - 28, 22)); g.setColour (Colour (0xff3d4278)); g.drawHorizontalLine (r.getY() + 34, (float) r.getX() + 14, (float) r.getRight() - 14); }
}

// ---- knob with value chip and label -------------------------------------------------------------
class KnobCell : public Component, public SettableTooltipClient
{
public:
    explicit KnobCell (Mx400Processor& p) : proc (p)
    {
        addAndMakeVisible (slider);
        slider.setSliderStyle (Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
        slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
        slider.setMouseDragSensitivity (180);
        slider.setScrollWheelEnabled (true);
        slider.onValueChange = [this] { repaint(); };
        setVisible (false);
    }
    void bind (const String& labelText, const String& paramId, const String& tip, bool isToggleValue = false)
    {
        att.reset();
        label = labelText; toggleValue = isToggleValue;
        if (auto* prm = proc.apvts.getParameter (paramId))
            att = std::make_unique<SliderParameterAttachment> (*prm, slider, nullptr);
        slider.setTooltip (tip); setTooltip (tip);
        setVisible (true); repaint();
    }
    void unbind() { att.reset(); setVisible (false); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds();
        auto chip = r.removeFromTop (16).withSizeKeepingCentre (62, 16);
        g.setColour (C::graph()); g.fillRoundedRectangle (chip.toFloat(), 3.0f);
        g.setColour (C::sky()); g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 12.0f, Font::plain));
        g.drawText (String ((int) std::lround (slider.getValue())), chip, Justification::centred);
        g.setColour (C::text()); g.setFont (FontOptions (12.5f));
        g.drawText (label, getLocalBounds().removeFromBottom (18), Justification::centred, true);
    }
    void resized() override { slider.setBounds (getLocalBounds().withTrimmedTop (17).withTrimmedBottom (18).reduced (14, 0)); }
private:
    Mx400Processor& proc;
    Slider slider;
    std::unique_ptr<SliderParameterAttachment> att;
    String label; bool toggleValue = false;
};

static String paramKey (const String& key)
{
    if (key == "effectType") return "type";
    if (key == "mixFront") return "mix";
    return key;
}

static String tipFor (const Control& c, const Map& m)
{
    String t = String (c.label) + "  (MIDI CC " + String (c.cc) + ", sends the raw value 0 to 127).";
    for (const auto& o : printedOverlaps (m)) if (o.cc == c.cc) { t += "\nThe manual prints CC " + String (c.cc) + " for two controls in this map: verify on the unit."; break; }
    if (c.key == "effectType") t += "\nThe manual does not print which value selects which effect: raw value, use with care.";
    if (c.key == "routing") t += "\nThe manual does not print which value selects which routing: raw value.";
    return t;
}

// ---- small live picture of what the effect does (shape follows the knobs, scale is not calibrated) ----
class GraphView : public Component
{
public:
    GraphView (Mx400Processor& p, int effect) : proc (p), e (effect)
    {
        for (const char* k : { "p1", "p2", "p3", "p4", "x7", "x8" })
            vals[k] = proc.apvts.getRawParameterValue ("e" + String (e) + "_" + k);
    }
    void setFamily (const String& g, const String& n) { group = g; name = n; repaint(); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (C::graph()); g.fillRoundedRectangle (r, 4.0f);
        g.setColour (C::line()); for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (r.getHeight() * i / 4), 0.0f, r.getWidth());
        const auto a = r.reduced (8.0f, 10.0f);
        auto v = [this] (const char* k) { return jlimit (0.0f, 1.0f, vals[k]->load() / 127.0f); };
        Path p;
        if (group == "Reverb")
        {
            const float pre = v ("p1") * 0.25f, tau = 0.12f + v ("p2") * 0.7f;
            p.startNewSubPath (a.getX(), a.getBottom());
            const float x0 = a.getX() + pre * a.getWidth();
            p.lineTo (x0, a.getBottom()); p.lineTo (x0, a.getY());
            for (int i = 0; i <= 80; ++i) { const float t = (float) i / 80.0f; p.lineTo (x0 + t * (a.getRight() - x0), a.getBottom() - (a.getHeight()) * std::exp (-t / tau) * (1.0f - t * 0.1f)); }
            g.setColour (C::sky().withAlpha (0.25f)); { Path f (p); f.lineTo (a.getRight(), a.getBottom()); f.closeSubPath(); g.fillPath (f); }
            g.setColour (C::sky()); g.strokePath (p, PathStrokeType (1.6f));
            const int ticks = 3 + (int) (v ("x7") * 6.0f + v ("p3") * 3.0f);
            g.setColour (C::amber());
            for (int i = 0; i < ticks; ++i) { const float x = x0 + (i + 1) * (a.getWidth() * 0.15f) / (float) ticks; g.fillRect (x, a.getBottom() - a.getHeight() * (0.55f - 0.04f * i), 2.0f, a.getHeight() * (0.55f - 0.04f * i)); }
        }
        else if (group == "Delay")
        {
            const float step = 22.0f + v ("p2") * (a.getWidth() / 5.0f), fb = 0.2f + 0.75f * v ("p3");
            float amp = 1.0f;
            for (float x = a.getX() + 4.0f; x < a.getRight() && amp > 0.03f; x += step, amp *= fb)
            { g.setColour (x == a.getX() + 4.0f ? Colours::white : C::sky()); g.fillRect (x, a.getBottom() - a.getHeight() * amp, 3.0f, a.getHeight() * amp); }
        }
        else if (group == "Modulation")
        {
            const float cyc = 1.0f + v ("p1") * 7.0f, amp = 0.15f + 0.85f * v ("p2");
            for (int i = 0; i <= 160; ++i) { const float t = (float) i / 160.0f; const float y = a.getCentreY() - std::sin (t * cyc * MathConstants<float>::twoPi) * amp * a.getHeight() * 0.5f; if (i == 0) p.startNewSubPath (a.getX(), y); else p.lineTo (a.getX() + t * a.getWidth(), y); }
            g.setColour (C::mint()); g.strokePath (p, PathStrokeType (1.6f));
        }
        else
        {
            const float thr = 0.2f + 0.7f * v ("p2"), ratio = 1.0f + v ("p1") * 9.0f;
            p.startNewSubPath (a.getX(), a.getBottom());
            for (int i = 1; i <= 100; ++i) { const float x = (float) i / 100.0f; const float y = x < thr ? x : thr + (x - thr) / ratio; p.lineTo (a.getX() + x * a.getWidth(), a.getBottom() - y * a.getHeight()); }
            g.setColour (C::amber()); g.strokePath (p, PathStrokeType (1.6f));
        }
        g.setColour (C::muted()); g.setFont (FontOptions (10.0f));
        g.drawText ("illustration: follows the knobs, scale not calibrated", getLocalBounds().reduced (6, 2), Justification::bottomRight);
    }
private:
    Mx400Processor& proc; int e;
    std::map<String, std::atomic<float>*> vals;
    String group, name;
};

// ---- one effect: header, picture, up to 12 knobs, advanced (raw) strip -------------------------
class EffectPanel : public Component
{
public:
    EffectPanel (Mx400Processor& p, int effectNumber, bool surroundOnly, std::function<void()> changed)
        : proc (p), e (effectNumber), forceSurround (surroundOnly), onChanged (std::move (changed)), graph (p, effectNumber)
    {
        addAndMakeVisible (fxBox); addAndMakeVisible (bypass); addAndMakeVisible (graph);
        fxBox.onChange = [this]
        {
            if (updating) return;
            proc.setSetting (viewKey().toRawUTF8(), fxBox.getText());
            rebuild();
        };
        bypass.setButtonText ("Bypass");
        bypass.setTooltip ("Bypass this effect. The manual does not print the on/off values: the plugin sends 127 for on and 0 for off (usual MIDI convention), verify on the unit.");
        for (int i = 0; i < 12; ++i) { main.push_back (std::make_unique<KnobCell> (p)); addAndMakeVisible (*main.back()); }
        for (int i = 0; i < 5; ++i)  { adv.push_back (std::make_unique<KnobCell> (p)); addAndMakeVisible (*adv.back()); }
        fxBox.setTooltip ("Which effect the unit has loaded here. The MX400 cannot report it back, so pick the same one: it only changes the knob names and the CC map used. It does not change the unit.");
    }

    int program() const { return e >= 3 ? 1 : 0; }
    const Map& map() const { return forceSurround ? surroundMap() : proc.mapForProgram (program()); }
    String viewKey() const { return "view_" + String (map().title).substring (0, 6) + "_" + String (e); }

    void rebuild()
    {
        const auto& m = map();
        const auto cfg = forceSurround ? Config::Surround : proc.config();
        const int slot = (e - 1) % 2;
        // slot 2 of a routing 5 program does not exist; effects 3 and 4 only exist in Dual Stereo
        bool exists = true;
        if (cfg == Config::Stereo && slot == 1 && proc.routingView (0) == 5) exists = false;
        if (cfg == Config::DualStereo && slot == 1 && proc.routingView (program()) == 5) exists = false;
        if (cfg != Config::DualStereo && e > 2) exists = false;
        if (cfg == Config::Surround && e > 1) exists = false;
        setVisible (exists);
        if (! exists) return;

        updating = true;
        fxBox.clear (dontSendNotification);
        for (size_t i = 0; i < m.effects.size(); ++i) fxBox.addItem (m.effects[i].name, (int) i + 1);
        String want = proc.getStr (viewKey().toRawUTF8());
        int idx = effectIndex (m, want.toStdString()); if (idx < 0) idx = 0;
        fxBox.setSelectedId (idx + 1, dontSendNotification);
        updating = false;

        const auto& row = m.effects[(size_t) idx];
        graph.setFamily (row.group, row.name);
        bypass.setVisible (true);
        bypassAtt.reset();
        if (auto* prm = proc.apvts.getParameter ("e" + String (e) + "_bypass"))
            bypassAtt = std::make_unique<ButtonParameterAttachment> (*prm, bypass, nullptr);

        for (auto& k : main) k->unbind();
        for (auto& k : adv)  k->unbind();
        size_t mi = 0, ai = 0;
        for (const auto& c : controlsFor (m, idx, slot))
        {
            const String key (c.key);
            if (key == "mix" || key == "mixFront" || key == "mixRear" || key == "routing" || key == "bypass") continue;
            const String id = "e" + String (e) + "_" + paramKey (key);
            const bool isMain = key.startsWithChar ('p') || key.startsWithChar ('x');
            auto& list = isMain ? main : adv; auto& n = isMain ? mi : ai;
            if (n < list.size()) list[n++]->bind (c.label, id, tipFor (c, m));
        }
        resized(); repaint();
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (14, 8);
        auto head = r.removeFromTop (30);
        head.removeFromLeft (150);                                   // title is painted here
        bypass.setBounds (head.removeFromRight (100).reduced (0, 1));
        head.removeFromRight (10);
        fxBox.setBounds (head.removeFromLeft (230).reduced (0, 1));
        r.removeFromTop (14);
        auto top = r.removeFromTop (190);
        graph.setBounds (top.removeFromLeft (350).reduced (0, 4));
        top.removeFromLeft (14);
        const int cw = top.getWidth() / 6;
        for (size_t i = 0; i < main.size(); ++i)
            main[i]->setBounds (top.getX() + (int) (i % 6) * cw, top.getY() + (int) (i / 6) * 95, cw, 93);
        r.removeFromTop (6);
        auto strip = r.removeFromTop (95);
        strip.removeFromLeft (364);
        for (size_t i = 0; i < adv.size(); ++i) adv[i]->setBounds (strip.getX() + (int) i * cw, strip.getY(), cw, 93);
    }

    void paint (Graphics& g) override
    {
        panelBox (g, getLocalBounds(), {});
        titleText (g, "Effect " + String (e), Rectangle<int> (14, 8, 140, 30));
        g.setColour (C::muted()); g.setFont (FontOptions (11.0f));
        g.drawText ("Advanced and raw controls", Rectangle<int> (14, getHeight() - 112, 340, 14), Justification::centredLeft);
    }

    ComboBox fxBox;
    ToggleButton bypass;
    GraphView graph;
    std::vector<std::unique_ptr<KnobCell>> main, adv;
private:
    Mx400Processor& proc;
    int e; bool forceSurround; std::function<void()> onChanged;
    bool updating = false;
    std::unique_ptr<ButtonParameterAttachment> bypassAtt;
};

// ---- left panel: program A/B, routing, mix ------------------------------------------------------
class RoutingPanel : public Component
{
public:
    RoutingPanel (Mx400Processor& p, std::function<void()> changed) : proc (p), onChanged (std::move (changed)), mixA (p), mixB (p), routeRaw (p)
    {
        for (auto* b : { &progA, &progB }) { addAndMakeVisible (*b); b->setRadioGroupId (91); b->setClickingTogglesState (true); }
        progA.setButtonText ("Program A"); progB.setButtonText ("Program B"); progA.setToggleState (true, dontSendNotification);
        progA.onClick = [this] { current = 0; onChanged(); };
        progB.onClick = [this] { current = 1; onChanged(); };
        addAndMakeVisible (routeBox);
        for (auto n : { "Dual Mono", "Cascade", "Parallel", "Mono Split", "Stereo" }) routeBox.addItem (n, routeBox.getNumItems() + 1);
        routeBox.setTooltip ("The routing the unit uses for this program. The MX400 cannot report it, so pick the same one: it chooses which CC map the plugin uses (routing 5, Stereo, uses the stereo effects map). It sends nothing.");
        routeBox.onChange = [this]
        {
            if (updating) return;
            proc.setSetting (current == 0 ? "routingA" : "routingB", routeBox.getSelectedId());
            onChanged();
        };
        addAndMakeVisible (mixA); addAndMakeVisible (mixB); addAndMakeVisible (routeRaw);
    }
    int current = 0;                                   // program being edited (Dual Stereo)
    void rebuild()
    {
        const auto cfg = proc.config();
        const bool dual = cfg == Config::DualStereo;
        if (! dual) current = 0;
        progA.setVisible (dual); progB.setVisible (dual);
        progA.setToggleState (current == 0, dontSendNotification); progB.setToggleState (current == 1, dontSendNotification);
        routeBox.setVisible (cfg != Config::Surround);
        updating = true; routeBox.setSelectedId (proc.routingView (cfg == Config::Stereo ? 0 : current), dontSendNotification); updating = false;
        const int base = current * 2;
        const auto& m = proc.mapForProgram (current);
        if (cfg == Config::Surround)
        {
            mixA.bind ("Mix Front", "e1_mix", "Mix Front (MIDI CC 26, raw 0 to 127)");
            mixB.bind ("Mix Rear", "e1_mixRear", "Mix Rear (MIDI CC 27, raw 0 to 127)");
        }
        else
        {
            const auto& col = m.columns[(size_t) columnIndex (m, "mix")];
            mixA.bind ("Mix 1", "e" + String (base + 1) + "_mix", "Mix of effect " + String (base + 1) + " (MIDI CC " + String (col.cc[0]) + ", raw 0 to 127)");
            const bool two = ! (proc.routingView (cfg == Config::Stereo ? 0 : current) == 5);
            if (two) mixB.bind ("Mix 2", "e" + String (base + 2) + "_mix", "Mix of effect " + String (base + 2) + " (MIDI CC " + String (col.cc[1]) + ", raw 0 to 127)");
            else mixB.unbind();
        }
        routeRaw.bind ("Routing (raw)", current == 0 ? "a_routing" : "b_routing",
                       "Routing, MIDI CC 28, raw value. The manual does not print which value is which routing.");
        resized(); repaint();
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (14, 8); r.removeFromTop (30);
        auto seg = r.removeFromTop (30);
        progA.setBounds (seg.removeFromLeft (seg.getWidth() / 2 - 3)); seg.removeFromLeft (6); progB.setBounds (seg);
        r.removeFromTop (10);
        routeBox.setBounds (r.removeFromTop (30));
        diagram = r.removeFromTop (110).reduced (0, 8);
        auto k = r.removeFromTop (95);
        mixA.setBounds (k.removeFromLeft (k.getWidth() / 2)); mixB.setBounds (k);
        r.removeFromTop (10);
        routeRaw.setBounds (r.removeFromTop (95).withSizeKeepingCentre (110, 95));
    }
    void paint (Graphics& g) override
    {
        const auto cfg = proc.config();
        panelBox (g, getLocalBounds(), cfg == Config::DualStereo ? String ("Input / routing") : cfg == Config::Surround ? String ("Surround") : String ("Input / routing"));
        if (cfg == Config::Surround || ! routeBox.isVisible()) return;
        const int routing = proc.routingView (cfg == Config::Stereo ? 0 : current);
        g.setColour (C::graph()); g.fillRoundedRectangle (diagram.toFloat(), 4.0f);
        auto box = [&] (float x, float y, const char* t, Colour c) { Rectangle<float> b (x, y, 40, 26); g.setColour (c); g.fillRoundedRectangle (b, 4.0f); g.setColour (Colours::white); g.setFont (FontOptions (13.0f, Font::bold)); g.drawText (t, b, Justification::centred); };
        auto ln = [&] (float x1, float y1, float x2, float y2) { g.setColour (C::muted()); g.drawLine (x1, y1, x2, y2, 1.5f); };
        const float cx = (float) diagram.getCentreX(), cy = (float) diagram.getCentreY(), x0 = (float) diagram.getX() + 24.0f, x1 = (float) diagram.getRight() - 24.0f;
        const Colour a = Colour (0xff3d9a52), b = Colour (0xffb44040);
        g.setColour (C::muted()); g.setFont (FontOptions (11.0f));
        g.drawText ("IN", diagram.getX() + 2, (int) cy - 8, 22, 16, Justification::centred); g.drawText ("OUT", diagram.getRight() - 26, (int) cy - 8, 26, 16, Justification::centred);
        switch (routing)
        {
            case 1: ln (x0, cy - 20, x1, cy - 20); ln (x0, cy + 20, x1, cy + 20); box (cx - 20, cy - 33, "1", a); box (cx - 20, cy + 7, "2", b); break;
            case 2: ln (x0, cy, x1, cy); box (cx - 60, cy - 13, "1", a); box (cx + 20, cy - 13, "2", b); break;
            case 3: ln (x0, cy, x1, cy); box (cx - 20, cy - 36, "1", a); box (cx - 20, cy + 10, "2", b); break;
            case 4: ln (x0, cy - 20, x1, cy - 20); ln (x0, cy + 20, x1, cy + 20); box (cx - 20, cy - 33, "1", a); box (cx - 20, cy + 7, "2", b); break;
            default: ln (x0, cy, x1, cy); box (cx - 20, cy - 13, "1", a); break;
        }
    }
    TextButton progA, progB;
    ComboBox routeBox;
private:
    Mx400Processor& proc; std::function<void()> onChanged;
    KnobCell mixA, mixB, routeRaw;
    Rectangle<int> diagram;
    bool updating = false;
};

// ---- program box in the header (LCD look) ------------------------------------------------------
class ProgramBox : public Component, public SettableTooltipClient
{
public:
    ProgramBox (Mx400Processor& p, int program) : proc (p), pr (program)
    {
        setTooltip ("Program " + String (pr == 0 ? "A" : "B") + ". Click the arrows or turn the mouse wheel to change program, click FACTORY or USER to switch bank. Changing it sends bank select and program change to the MX400.");
    }
    int prog() const { return (int) std::lround (proc.apvts.getRawParameterValue (pr == 0 ? "a_prog" : "b_prog")->load()); }
    int bank() const { return (int) std::lround (proc.apvts.getRawParameterValue (pr == 0 ? "a_bank" : "b_bank")->load()); }
    void setProg (int v) { set (pr == 0 ? "a_prog" : "b_prog", jlimit (1, 99, v)); }
    void setBank (int v) { set (pr == 0 ? "a_bank" : "b_bank", v); }
    String nick() const { return proc.getStr (("nick_" + String (proc.getInt ("config", 1)) + "_" + String (bank()) + "_" + String (prog())).toRawUTF8()); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        ColourGradient cg (Colour (0xff9fc0ff), 0, r.getY(), Colour (0xff5d86e6), 0, r.getBottom(), false);
        g.setGradientFill (cg); g.fillRoundedRectangle (r, 5.0f);
        g.setColour (Colour (0xff10163a));
        g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 22.0f, Font::plain));
        const String n = nick();
        g.drawText (String (pr == 0 ? "A" : "B") + "  " + String (prog()).paddedLeft ('0', 2) + (n.isNotEmpty() ? "  " + n : String()), getLocalBounds().withTrimmedLeft (34).withTrimmedRight (116), Justification::centredLeft, true);
        g.setFont (FontOptions (13.0f, Font::bold));
        g.drawText (bank() ? "USER v" : "FACTORY v", getLocalBounds().removeFromRight (112), Justification::centred);
        g.drawText ("<", getLocalBounds().removeFromLeft (30), Justification::centred);
    }
    void mouseDown (const MouseEvent& e) override
    {
        if (e.x < 30) setProg (prog() - 1);
        else if (e.x > getWidth() - 112) setBank (bank() ? 0 : 1);
        else if (e.x > getWidth() - 140) setProg (prog() + 1);
        else
        {
            PopupMenu m;
            for (int g = 0; g < 10; ++g) { PopupMenu sub; for (int i = 1; i <= 10 && g * 10 + i <= 99; ++i) sub.addItem (g * 10 + i, String (g * 10 + i)); m.addSubMenu (String (g * 10 + 1) + " to " + String (jmin (99, g * 10 + 10)), sub); }
            m.showMenuAsync (PopupMenu::Options().withTargetComponent (this), [this] (int id) { if (id > 0) setProg (id); });
        }
        repaint();
    }
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) override { setProg (prog() + (w.deltaY > 0 ? 1 : -1)); repaint(); }
private:
    void set (const String& id, int v)
    {
        if (auto* prm = proc.apvts.getParameter (id)) { prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 ((float) v)); prm->endChangeGesture(); }
    }
    Mx400Processor& proc; int pr;
};

// ---- library: program grid, notes, SysEx capture ----------------------------------------------
class LibraryPage : public Component, private ListBoxModel
{
public:
    explicit LibraryPage (Mx400Processor& p) : proc (p)
    {
        for (auto* b : { &tabFactory, &tabUser, &tabFav }) { addAndMakeVisible (*b); b->setRadioGroupId (92); b->setClickingTogglesState (true); }
        tabFactory.setButtonText ("Factory"); tabUser.setButtonText ("User"); tabFav.setButtonText ("Favorites"); tabFactory.setToggleState (true, dontSendNotification);
        for (auto* b : { &tabFactory, &tabUser, &tabFav }) b->onClick = [this] { repaint(); };
        for (auto* b : { &targetA, &targetB }) { addAndMakeVisible (*b); b->setRadioGroupId (93); b->setClickingTogglesState (true); }
        targetA.setButtonText ("Program A"); targetB.setButtonText ("Program B"); targetA.setToggleState (true, dontSendNotification);
        addAndMakeVisible (note); note.setTextToShowWhenEmpty ("Your note for this program", C::muted());
        note.setTooltip ("Your own name or note for this program. The MX400's program names cannot be read over MIDI, so this is saved in the plugin only.");
        note.onTextChange = [this] { proc.setSetting (nickKey (sel).toRawUTF8(), note.getText()); getParentComponent() ? getParentComponent()->repaint() : repaint(); };
        addAndMakeVisible (sendBtn); sendBtn.setButtonText ("Send to MX400");
        sendBtn.setTooltip ("Sends bank select and program change for the selected program to the MX400.");
        sendBtn.onClick = [this] { applySelection(); };
        addAndMakeVisible (list); list.setModel (this); list.setRowHeight (22);
        addAndMakeVisible (saveSel); saveSel.setButtonText ("Save selected .syx");
        addAndMakeVisible (saveAll); saveAll.setButtonText ("Save all");
        addAndMakeVisible (openDir); openDir.setButtonText ("Open folder");
        addAndMakeVisible (clearBtn); clearBtn.setButtonText ("Clear");
        addAndMakeVisible (importBtn); importBtn.setButtonText ("Import .syx (check only)");
        addAndMakeVisible (fetchBtn); fetchBtn.setButtonText ("Fetch from MX400"); fetchBtn.setEnabled (false);
        fetchBtn.setTooltip ("Needs the MX-Edit SysEx request, which is not documented. Capture it first (see docs/CAPTURE-GUIDE.md). Nothing unverified is ever sent to the unit.");
        importBtn.setTooltip ("Reads a .syx file and tells you how many SysEx messages it holds. Sending a dump to the MX400 is not built until the format is verified.");
        addAndMakeVisible (status); status.setJustificationType (Justification::topLeft); status.setColour (Label::textColourId, C::muted());
        saveSel.onClick = [this]
        {
            const int i = list.getSelectedRow();
            auto f = proc.saveCapture (i);
            status.setText (f.exists() ? "Saved " + f.getFileName() + " in " + Mx400Processor::dataDir().getChildFile ("captures").getFullPathName() : "Select a capture first.", dontSendNotification);
        };
        saveAll.onClick = [this] { status.setText ("Saved " + String (proc.saveAllCaptures()) + " file(s) to " + Mx400Processor::dataDir().getChildFile ("captures").getFullPathName(), dontSendNotification); };
        openDir.onClick = [this] { auto d = Mx400Processor::dataDir().getChildFile ("captures"); d.createDirectory(); d.revealToUser(); };
        clearBtn.onClick = [this] { proc.clearCaptures(); refresh(); };
        importBtn.onClick = [this]
        {
            chooser = std::make_unique<FileChooser> ("Choose a .syx file", File(), "*.syx;*.mid;*.bin");
            chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
            {
                const auto f = fc.getResult(); if (f == File()) return;
                bool ok = false; const auto parts = Mx400Processor::readSyxFile (f, ok);
                size_t bytes = 0; for (auto& m : parts) bytes += m.size();
                status.setText (ok ? f.getFileName() + ": " + String ((int) parts.size()) + " SysEx message(s), " + String ((int) bytes) + " bytes, first: " + String (toHex (parts[0], 12)) + "\nChecked only. Nothing was sent."
                                   : f.getFileName() + ": no valid SysEx found.", dontSendNotification);
            });
        };
    }
    int target() const { return targetB.getToggleState() ? 1 : 0; }
    void setDual (bool d) { targetB.setVisible (d); targetA.setVisible (d); if (! d) targetA.setToggleState (true, dontSendNotification); }
    void refresh() { list.updateContent(); list.repaint(); }
    void visibilityChanged() override { if (isVisible()) refresh(); }

    int bankSel() const { return tabUser.getToggleState() ? 1 : 0; }
    String favKey (int p) const { return "fav_" + String (proc.getInt ("config", 1)) + "_" + String (bankSel()) + "_" + String (p); }
    String nickKey (int p) const { return "nick_" + String (proc.getInt ("config", 1)) + "_" + String (bankSel()) + "_" + String (p); }

    void resized() override
    {
        auto r = getLocalBounds();
        left = r.removeFromLeft (560).reduced (0, 0);
        auto lt = left.reduced (14, 8); lt.removeFromTop (30);
        auto tabs = lt.removeFromTop (28);
        tabFactory.setBounds (tabs.removeFromLeft (110)); tabs.removeFromLeft (6); tabUser.setBounds (tabs.removeFromLeft (110)); tabs.removeFromLeft (6); tabFav.setBounds (tabs.removeFromLeft (110));
        lt.removeFromTop (8); grid = lt.removeFromTop (11 * 40);
        r.removeFromLeft (16);
        auto detail = r.removeFromTop (230); detailBox = detail;
        auto d = detail.reduced (14, 8); d.removeFromTop (30);
        auto tg = d.removeFromTop (28); targetA.setBounds (tg.removeFromLeft (110)); tg.removeFromLeft (6); targetB.setBounds (tg.removeFromLeft (110));
        d.removeFromTop (10); note.setBounds (d.removeFromTop (30));
        d.removeFromTop (10); sendBtn.setBounds (d.removeFromTop (32).removeFromLeft (170));
        r.removeFromTop (16);
        sysexBox = r;
        auto s = r.reduced (14, 8); s.removeFromTop (30);
        auto bt = s.removeFromBottom (30); status.setBounds (s.removeFromBottom (52));
        saveSel.setBounds (bt.removeFromLeft (140)); bt.removeFromLeft (6); saveAll.setBounds (bt.removeFromLeft (80)); bt.removeFromLeft (6); openDir.setBounds (bt.removeFromLeft (100)); bt.removeFromLeft (6); clearBtn.setBounds (bt.removeFromLeft (70)); bt.removeFromLeft (6);
        auto b2 = s.removeFromBottom (0); (void) b2;
        importBtn.setBounds (bt.removeFromLeft (170)); bt.removeFromLeft (6); fetchBtn.setBounds (bt.removeFromLeft (150));
        list.setBounds (s);
    }
    void paint (Graphics& g) override
    {
        panelBox (g, left, "Programs"); panelBox (g, detailBox, "Selected program"); panelBox (g, sysexBox, "SysEx capture (receive only)");
        for (int i = 1; i <= 99; ++i)
        {
            auto c = cell (i);
            const bool fav = (bool) proc.apvts.state.getProperty (favKey (i).toRawUTF8(), false);
            if (tabFav.getToggleState() && ! fav) { g.setColour (C::graph().withAlpha (0.4f)); g.fillRoundedRectangle (c.toFloat(), 4.0f); continue; }
            const bool cur = (i == sel);
            g.setColour (cur ? C::on() : C::panelDark()); g.fillRoundedRectangle (c.toFloat(), 4.0f);
            g.setColour (Colour (0xff4a4f8a)); g.drawRoundedRectangle (c.toFloat().reduced (0.5f), 4.0f, 1.0f);
            g.setColour (C::text()); g.setFont (FontOptions (14.0f, Font::bold));
            g.drawText (String (i).paddedLeft ('0', 2), c.withTrimmedBottom (14), Justification::centred);
            const String n = proc.getStr (nickKey (i).toRawUTF8());
            g.setFont (FontOptions (9.5f)); g.setColour (C::muted()); g.drawText (n, c.removeFromBottom (14), Justification::centred, true);
            if (fav) { g.setColour (C::amber()); g.setFont (FontOptions (12.0f)); g.drawText ("*", c.removeFromTop (14).removeFromRight (12), Justification::centred); }
        }
        g.setColour (C::muted()); g.setFont (FontOptions (11.0f));
        g.drawText ("Click to select, double click to send, right click for favorite.", Rectangle<int> (left.getX() + 14, grid.getBottom() + 4, left.getWidth() - 28, 16), Justification::centredLeft);
        const auto in = proc.getStr ("inName");
        g.drawText (in.isEmpty() ? "No input chosen: pick the MX400's MIDI input in Settings to capture SysEx." : "Listening on: " + in, Rectangle<int> (sysexBox.getX() + 14, sysexBox.getY() + 12, sysexBox.getWidth() - 28, 16), Justification::centredRight);
    }
    void mouseDown (const MouseEvent& e) override
    {
        const int i = hit (e.getPosition()); if (i < 0) return;
        if (e.mods.isPopupMenu()) { const auto k = favKey (i).toRawUTF8(); proc.setSetting (k, ! (bool) proc.apvts.state.getProperty (k, false)); repaint(); return; }
        select (i);
    }
    void mouseDoubleClick (const MouseEvent& e) override { const int i = hit (e.getPosition()); if (i >= 0) { select (i); applySelection(); } }
    void select (int i) { sel = i; note.setText (proc.getStr (nickKey (i).toRawUTF8()), dontSendNotification); repaint(); }
private:
    Rectangle<int> cell (int i) const { const int c = (i - 1) % 9, r = (i - 1) / 9; return Rectangle<int> (grid.getX() + c * 58, grid.getY() + r * 40, 54, 36); }
    int hit (Point<int> p) const { for (int i = 1; i <= 99; ++i) if (cell (i).contains (p)) return i; return -1; }
    void applySelection()
    {
        const String pre = target() == 0 ? "a" : "b";
        for (auto k : { String ("_bank"), String ("_prog") })
            if (auto* prm = proc.apvts.getParameter (pre + k))
            {
                const float v = k == "_bank" ? (float) bankSel() : (float) sel;
                prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); prm->endChangeGesture();
            }
        proc.resendPrograms();                          // sends even when the unit already shows this program
    }
    int getNumRows() override { return (int) proc.captures().size(); }
    void paintListBoxItem (int row, Graphics& g, int w, int h, bool selected) override
    {
        const auto c = proc.captures(); if (row >= (int) c.size()) return;
        if (selected) { g.setColour (C::on()); g.fillRect (0, 0, w, h); }
        g.setColour (C::text()); g.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 11.5f, Font::plain));
        g.drawText (c[(size_t) row].when.formatted ("%H:%M:%S") + "  " + String ((int) c[(size_t) row].data.size()).paddedLeft (' ', 6) + " B  " + String (toHex (c[(size_t) row].data, 10)), 6, 0, w - 8, h, Justification::centredLeft, true);
    }
    Mx400Processor& proc;
    TextButton tabFactory, tabUser, tabFav, targetA, targetB, sendBtn, saveSel, saveAll, openDir, clearBtn, importBtn, fetchBtn;
    TextEditor note; ListBox list; Label status;
    Rectangle<int> left, grid, detailBox, sysexBox;
    int sel = 1;
    std::unique_ptr<FileChooser> chooser;
};

// ---- settings page -----------------------------------------------------------------------------
class SettingsPage : public Component
{
public:
    SettingsPage (Mx400Processor& p, std::function<void()> changed, std::function<void()> reset100) : proc (p), onChanged (std::move (changed))
    {
        auto row = [this] (ComboBox& c, const char* tip) { addAndMakeVisible (c); c.setTooltip (tip); };
        row (outBox, "The MIDI output that goes to the MX400 (its USB MIDI port or a 5-pin DIN interface).");
        row (inBox, "The MIDI input from the MX400. Used only to capture SysEx.");
        row (routeBox, "Direct to port: the plugin opens the MIDI port itself. DAW MIDI output: the messages leave through this plugin's MIDI output, route it in your DAW. Both sends to both.");
        row (chABox, "Must equal MIDI Channel A in the MX400 System menu.");
        row (chBBox, "Must equal MIDI Channel B in the MX400 System menu (Dual Stereo only).");
        row (rateBox, "How fast messages leave. Normal is safe for DIN MIDI.");
        for (int i = 1; i <= 16; ++i) { chABox.addItem (String (i), i); chBBox.addItem (String (i), i); }
        routeBox.addItem ("Direct to port", 1); routeBox.addItem ("DAW MIDI output", 2); routeBox.addItem ("Both", 3);
        rateBox.addItem ("Fast", 1); rateBox.addItem ("Normal", 2); rateBox.addItem ("Slow", 3);
        auto tog = [this] (ToggleButton& t, const char* text, const char* tip) { addAndMakeVisible (t); t.setButtonText (text); t.setTooltip (tip); };
        tog (recallT, "Recall on load: send my touched controls when the project opens", "Off by default: nothing is sent when a project opens. When on, only the controls you have moved are sent.");
        tog (manualT, "Manual mode: only send when I press Send all", "Moving a control changes the plugin only; nothing goes to the unit until you press Send all.");
        tog (thruT, "Pass DAW MIDI through", "Lets MIDI from the DAW pass through this plugin's MIDI output.");
        tog (hintsT, "Show hints (info bubbles)", "Hover a control for about one second to see what it does.");
        outBox.onChange = [this] { const int i = outBox.getSelectedId() - 2; if (i < 0) proc.chooseOutput ({}, {}); else proc.chooseOutput (proc.outputIds()[i], proc.outputNames()[i]); onChanged(); };
        inBox.onChange  = [this] { const int i = inBox.getSelectedId() - 2;  if (i < 0) proc.chooseInput ({}, {}); else proc.chooseInput (proc.inputIds()[i], proc.inputNames()[i]); onChanged(); };
        routeBox.onChange = [this] { proc.setSetting ("route", routeBox.getSelectedId() - 1); };
        chABox.onChange = [this] { proc.setSetting ("chA", chABox.getSelectedId()); };
        chBBox.onChange = [this] { proc.setSetting ("chB", chBBox.getSelectedId()); };
        rateBox.onChange = [this] { proc.setSetting ("rate", rateBox.getSelectedId() - 1); };
        recallT.onClick = [this] { proc.setSetting ("recallOnLoad", recallT.getToggleState()); };
        manualT.onClick = [this] { proc.setSetting ("manual", manualT.getToggleState()); };
        thruT.onClick = [this] { proc.setSetting ("thru", thruT.getToggleState()); };
        hintsT.onClick = [this] { proc.setSetting ("hints", hintsT.getToggleState()); onChanged(); };
        auto btn = [this] (TextButton& b, const char* t, const char* tip) { addAndMakeVisible (b); b.setButtonText (t); b.setTooltip (tip); };
        btn (refreshB, "Refresh ports", "Look for MIDI ports again (after you plug the MX400 in).");
        btn (sendAllB, "Send all touched now", "Sends every control you have moved, in one go. Controls you never touched are not sent.");
        btn (forgetB, "Forget touched", "Clears the list of controls you have touched, so Send all sends nothing until you move something.");
        btn (progB, "Send programs again", "Sends bank select and program change for program A (and B in Dual Stereo).");
        btn (srcB, "Source code", "Opens the GitHub page in your browser. This is also where you check for new versions: the plugin never connects to the internet by itself.");
        btn (donB, "Support on Ko-fi", "Opens ko-fi.com/studio87 in your browser. The plugin is free; this is optional.");
        btn (dbgB, "Export debug info", "Writes a text file with your settings and the last messages sent. Nothing leaves your computer.");
        btn (resetB, "Reset window to 100%", "Back to the default size. Double clicking a wood side does the same.");
        refreshB.onClick = [this] { fillPorts(); };
        sendAllB.onClick = [this] { proc.sendAllTouched(); };
        forgetB.onClick = [this] { proc.clearTouched(); };
        progB.onClick = [this] { proc.resendPrograms(); };
        srcB.onClick = [] { URL ("https://github.com/SpeakingColors/mx400-vst-linux").launchInDefaultBrowser(); };
        donB.onClick = [] { URL ("https://ko-fi.com/studio87").launchInDefaultBrowser(); };
        resetB.onClick = std::move (reset100);
        dbgB.onClick = [this]
        {
            auto dir = Mx400Processor::dataDir(); dir.createDirectory();
            auto f = dir.getChildFile ("mx400-debug.txt");
            String s = "MX400 controller debug info\nversion " + String (JucePlugin_VersionString) + "\nsystem " + SystemStats::getOperatingSystemName() + "\n";
            s += "config " + String (proc.getInt ("config", 1)) + "  channels A " + String (proc.getInt ("chA", 1)) + " B " + String (proc.getInt ("chB", 2)) + "  route " + String (proc.getInt ("route", 0)) + "  rate " + String (proc.getInt ("rate", 1)) + "\n";
            s += "output: " + proc.getStr ("outName") + (proc.outputOpen() ? " (open)" : " (closed)") + "\ninput: " + proc.getStr ("inName") + (proc.inputOpen() ? " (open)" : " (closed)") + "\n";
            s += "messages sent: " + String (proc.sentCount()) + "\ncaptured SysEx: " + String ((int) proc.captures().size()) + "\nlast sent:\n";
            for (auto& l : proc.lastSent()) s += "  " + String (l) + "\n";
            f.replaceWithText (s); f.revealToUser();
        };
        addAndMakeVisible (monitor); monitor.setMultiLine (true); monitor.setReadOnly (true); monitor.setScrollbarsShown (false); monitor.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 11.5f, Font::plain));
        monitor.setTooltip ("The last messages the plugin sent to the MX400, newest at the bottom.");
        fillPorts(); sync();
    }
    void fillPorts()
    {
        outBox.clear (dontSendNotification); inBox.clear (dontSendNotification);
        outBox.addItem ("(none)", 1); inBox.addItem ("(none)", 1);
        int i = 2; for (auto& n : proc.outputNames()) outBox.addItem (n, i++);
        i = 2; for (auto& n : proc.inputNames()) inBox.addItem (n, i++);
        const auto on = proc.getStr ("outName"), in = proc.getStr ("inName");
        outBox.setSelectedId (on.isEmpty() ? 1 : (proc.outputNames().indexOf (on) + 2 > 1 ? proc.outputNames().indexOf (on) + 2 : 1), dontSendNotification);
        inBox.setSelectedId (in.isEmpty() ? 1 : (proc.inputNames().indexOf (in) + 2 > 1 ? proc.inputNames().indexOf (in) + 2 : 1), dontSendNotification);
    }
    void sync()
    {
        routeBox.setSelectedId (proc.getInt ("route", 0) + 1, dontSendNotification);
        chABox.setSelectedId (proc.getInt ("chA", 1), dontSendNotification); chBBox.setSelectedId (proc.getInt ("chB", 2), dontSendNotification);
        rateBox.setSelectedId (proc.getInt ("rate", 1) + 1, dontSendNotification);
        recallT.setToggleState (proc.getBool ("recallOnLoad", false), dontSendNotification);
        manualT.setToggleState (proc.getBool ("manual", false), dontSendNotification);
        thruT.setToggleState (proc.getBool ("thru", true), dontSendNotification);
        hintsT.setToggleState (proc.getBool ("hints", true), dontSendNotification);
    }
    void tick()
    {
        if (! isVisible()) return;
        String s; for (auto& l : proc.lastSent()) s += String (l) + "\n";
        if (s != lastText) { lastText = s; monitor.setText (s, dontSendNotification); monitor.moveCaretToEnd(); }
    }
    void resized() override
    {
        auto r = getLocalBounds();
        midiBox = r.removeFromLeft (900).withHeight (470);
        auto m = midiBox.reduced (16, 8); m.removeFromTop (32);
        auto rowOf = [&m] (Component& c, int w = 360) { auto rr = m.removeFromTop (34); rr.removeFromLeft (220); c.setBounds (rr.removeFromLeft (w).reduced (0, 2)); };
        rowOf (outBox, 520); rowOf (inBox, 520); rowOf (routeBox, 240); rowOf (chABox, 90); rowOf (chBBox, 90); rowOf (rateBox, 150);
        refreshB.setBounds (midiBox.getRight() - 16 - 140, midiBox.getY() + 8 + 32 + 2, 140, 30);
        recallT.setBounds (m.removeFromTop (32).reduced (0, 2)); manualT.setBounds (m.removeFromTop (32).reduced (0, 2)); thruT.setBounds (m.removeFromTop (32).reduced (0, 2)); hintsT.setBounds (m.removeFromTop (32).reduced (0, 2));
        r.removeFromLeft (16);
        actBox = r.removeFromTop (470);
        auto a = actBox.reduced (16, 8); a.removeFromTop (32);
        sendAllB.setBounds (a.removeFromTop (34).reduced (0, 2)); forgetB.setBounds (a.removeFromTop (34).reduced (0, 2)); progB.setBounds (a.removeFromTop (34).reduced (0, 2)); a.removeFromTop (8);
        monitor.setBounds (a);
        r.removeFromTop (0);
        auto bottom = getLocalBounds().withTrimmedTop (486);
        aboutBox = bottom.removeFromTop (190);
        auto b = aboutBox.reduced (16, 8); b.removeFromTop (30 + 56);
        auto bb = b.removeFromTop (34);
        srcB.setBounds (bb.removeFromLeft (130)); bb.removeFromLeft (8); donB.setBounds (bb.removeFromLeft (150)); bb.removeFromLeft (8); dbgB.setBounds (bb.removeFromLeft (170)); bb.removeFromLeft (8); resetB.setBounds (bb.removeFromLeft (190));
    }
    void paint (Graphics& g) override
    {
        panelBox (g, midiBox, "MIDI and sending"); panelBox (g, actBox, "Actions and monitor"); panelBox (g, aboutBox, "About");
        g.setColour (C::muted()); g.setFont (FontOptions (13.0f));
        const char* labels[] = { "Output to MX400", "Input from MX400", "Output route", "MIDI channel A", "MIDI channel B", "Send rate" };
        for (int i = 0; i < 6; ++i) g.drawText (labels[i], midiBox.getX() + 16, midiBox.getY() + 8 + 32 + i * 34, 210, 34, Justification::centredLeft);
        g.setColour (C::text()); g.setFont (FontOptions (13.0f));
        g.drawFittedText ("MX400 Controller " + String (JucePlugin_VersionString) + ", part of the Studio 87 cat plugins. Free and open source (AGPL-3.0-or-later). It collects nothing, has no network code and never checks for updates by itself.\n"
                          "Not affiliated with or endorsed by the maker of the MX400. The device name is used for compatibility only. No manufacturer logos or files are included.",
                          Rectangle<int> (aboutBox.getX() + 16, aboutBox.getY() + 40, aboutBox.getWidth() - 32, 56), Justification::topLeft, 4);
    }
private:
    Mx400Processor& proc; std::function<void()> onChanged;
    ComboBox outBox, inBox, routeBox, chABox, chBBox, rateBox;
    ToggleButton recallT, manualT, thruT, hintsT;
    TextButton refreshB, sendAllB, forgetB, progB, srcB, donB, dbgB, resetB;
    TextEditor monitor; String lastText;
    Rectangle<int> midiBox, actBox, aboutBox;
};

// ---- the whole window (design size 1500 x 940, scaled) ----------------------------------------
class Content : public Component
{
public:
    Content (Mx400Processor& p, std::function<void()> reset100)
        : proc (p), onReset (reset100),
          progA (p, 0), progB (p, 1),
          routing (p, [this] { rebuildAll(); }),
          library (p),
          settings (p, [this] { rebuildAll(); }, reset100),
          gate (*this)
    {
        look = std::make_unique<Look>(); setLookAndFeel (look.get());
        for (int i = 0; i < 3; ++i)
        {
            auto& b = cfgBtn[i]; addAndMakeVisible (b); b.setRadioGroupId (94); b.setClickingTogglesState (true);
            b.setButtonText (i == 0 ? "Stereo" : i == 1 ? "Dual Stereo" : "Surround");
            b.setTooltip ("Which configuration the MX400 is running. Set it on the unit (System menu); this selects the matching CC map. It does not change the unit.");
            b.onClick = [this, i] { proc.setSetting ("config", i); rebuildAll(); };
        }
        addAndMakeVisible (progA); addAndMakeVisible (progB);
        const char* names[] = { "PANEL", "LIBRARY", "SURROUND", "SETTINGS" };
        for (int i = 0; i < 4; ++i)
        {
            auto& b = tabBtn[i]; addAndMakeVisible (b); b.setRadioGroupId (95); b.setClickingTogglesState (true); b.setButtonText (names[i]);
            b.onClick = [this, i] { showPage (i); };
        }
        addAndMakeVisible (sendAll); sendAll.setButtonText ("SEND ALL");
        sendAll.setTooltip ("Sends every control you have moved to the MX400 in one go. Nothing is sent automatically when a project loads.");
        sendAll.onClick = [this] { proc.sendAllTouched(); };
        addAndMakeVisible (gearB); gearB.setButtonText ("Settings"); gearB.onClick = [this] { showPage (3); };
        addAndMakeVisible (resetB); resetB.setButtonText ("100%"); resetB.onClick = [this] { onReset(); };
        resetB.setTooltip ("Back to the default size. Double clicking a wood side does the same.");
        addAndMakeVisible (infoB); infoB.setButtonText ("i");
        infoB.setTooltip ("What this plugin does.");
        infoB.onClick = [this]
        {
            auto l = std::make_unique<Label>();
            l->setText ("MX400 Controller\n\nControls the Lexicon MX400 from your DAW over MIDI (CC, program change, bank select) and captures its SysEx.\n\n"
                        "The MX400 sends nothing back over CC, so the knobs show what you sent, not what the unit holds. Nothing is sent when a project loads. "
                        "Value ranges and effect or routing codes are not printed in the manual, so values are raw 0 to 127.\n\n"
                        "Audio is not processed here: use your DAW's hardware insert for the MX400.", dontSendNotification);
            l->setJustificationType (Justification::topLeft); l->setColour (Label::textColourId, C::text()); l->setColour (Label::backgroundColourId, Colour (0xff1c1c4a));
            l->setSize (460, 250);
            CallOutBox::launchAsynchronously (std::move (l), infoB.getScreenBounds(), nullptr);
        };

        effects.add (new EffectPanel (p, 1, false, [this] { rebuildAll(); })); effects.add (new EffectPanel (p, 2, false, [this] { rebuildAll(); }));
        effects.add (new EffectPanel (p, 3, false, [this] { rebuildAll(); })); effects.add (new EffectPanel (p, 4, false, [this] { rebuildAll(); }));
        for (auto* e : effects) addChildComponent (e);
        surroundFx = std::make_unique<EffectPanel> (p, 1, true, [this] { rebuildAll(); });
        addChildComponent (surroundFx.get());
        addChildComponent (routing); addChildComponent (library); addChildComponent (settings);

        gate.onFrame = [this]
        {
            for (auto* e : effects) if (e->isShowing()) e->graph.repaint();
            if (surroundFx->isShowing()) surroundFx->graph.repaint();
            settings.tick(); repaint (statusArea);
            if (library.isShowing()) library.refresh();
        };
        gate.start (20);
        rebuildAll();
        showPage (0);
        setSize (Mx400Editor::kW, Mx400Editor::kH);
    }
    ~Content() override { gate.stop(); setLookAndFeel (nullptr); }

    void rebuildAll()
    {
        const int c = proc.getInt ("config", 1);
        for (int i = 0; i < 3; ++i) cfgBtn[i].setToggleState (i == c, dontSendNotification);
        routing.rebuild();
        const int cur = routing.current;
        for (auto* e : effects)
        {
            const bool mine = (cur == 0) ? (e->program() == 0) : (e->program() == 1);
            if (mine || c != 1) { e->rebuild(); if (c != 1 && e->program() == 1) e->setVisible (false); }
            else e->setVisible (false);
        }
        if (c == 1 && cur == 1) { effects[2]->rebuild(); effects[3]->rebuild(); effects[0]->setVisible (false); effects[1]->setVisible (false); }
        if (c == 2) { effects[0]->setVisible (false); effects[1]->setVisible (false); }
        surroundFx->rebuild();
        progB.setVisible (c == 1);
        library.setDual (c == 1);
        settings.sync();
        showPage (page);
    }
    void showPage (int p)
    {
        page = p;
        for (int i = 0; i < 4; ++i) tabBtn[i].setToggleState (i == p, dontSendNotification);
        const int c = proc.getInt ("config", 1);
        const bool panelPage = p == 0;
        routing.setVisible (panelPage);
        const int cur = routing.current;
        for (size_t i = 0; i < (size_t) effects.size(); ++i)
        {
            auto* e = effects[(int) i];
            bool want = panelPage;
            if (c == 2) want = false;
            if (c == 1 && ((cur == 0) != (e->program() == 0))) want = false;
            if (want) e->rebuild(); else e->setVisible (false);
        }
        if (panelPage && c == 2) surroundFx->rebuild(); else if (p == 2) surroundFx->rebuild(); else surroundFx->setVisible (false);
        library.setVisible (p == 1); settings.setVisible (p == 3);
        resized (); repaint();
    }

    void resized() override
    {
        constexpr int cheek = 34;
        auto r = getLocalBounds().reduced (cheek + 20, 0);
        auto head = r.removeFromTop (70).withTrimmedTop (18).withHeight (50);
        headArea = head;
        auto h = head.reduced (10, 8);
        h.removeFromLeft (190);
        auto cfgR = h.removeFromLeft (250);
        for (int i = 0; i < 3; ++i) cfgBtn[i].setBounds (cfgR.removeFromLeft (cfgR.getWidth() / (3 - i)));
        h.removeFromLeft (14);
        auto right = h.removeFromRight (262);
        resetB.setBounds (right.removeFromRight (56).reduced (0, 2)); right.removeFromRight (4); infoB.setBounds (right.removeFromRight (34).reduced (0, 2)); right.removeFromRight (4); gearB.setBounds (right.removeFromRight (80).reduced (0, 2)); right.removeFromRight (4); sendAll.setBounds (right.removeFromRight (90).reduced (0, 2));
        h.removeFromRight (14);
        const int bw = (h.getWidth() - 16) / 2;
        progA.setBounds (h.removeFromLeft (bw)); h.removeFromLeft (16); progB.setBounds (h.removeFromLeft (bw));
        r.removeFromTop (14);
        auto tabs = r.removeFromTop (32); tabArea = tabs;
        statusArea = tabs.removeFromRight (420).withTrimmedTop (0);
        for (int i = 0; i < 4; ++i) { tabBtn[i].setBounds (tabs.removeFromLeft (i == 2 ? 108 : 100)); tabs.removeFromLeft (8); }
        r.removeFromTop (14);
        auto area = r.removeFromTop (790);
        pageArea = area;
        auto a = area;
        auto left = a.removeFromLeft (300); a.removeFromLeft (16);
        routing.setBounds (left.withHeight (area.getHeight()));
        const int eh = (a.getHeight() - 16) / 2;
        auto top = a.removeFromTop (eh); a.removeFromTop (16);
        for (int i = 0; i < effects.size(); ++i) effects[i]->setBounds ((i % 2 == 0) ? top : a.withHeight (eh));
        const int c = proc.getInt ("config", 1);
        if (c == 2) surroundFx->setBounds (area.removeFromTop (eh).withLeft (area.getX() + 316));
        else surroundFx->setBounds (area.withTrimmedLeft (316).withHeight (eh));
        library.setBounds (area); settings.setBounds (area);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (C::page());
        catui::drawWoodCheeks (g, getLocalBounds(), 34);
        g.setColour (C::panel()); g.fillRoundedRectangle (headArea.toFloat(), 6.0f);
        g.setColour (Colour (0xff3d4278)); g.drawRoundedRectangle (headArea.toFloat().reduced (0.5f), 6.0f, 1.0f);
        catui::drawCatHead (g, Rectangle<float> ((float) headArea.getX() + 14, (float) headArea.getY() + 7, 44, 36), C::title());
        g.setColour (C::text()); g.setFont (FontOptions (24.0f, Font::bold)); g.drawText ("MX400", headArea.getX() + 66, headArea.getY() + 4, 120, 28, Justification::centredLeft);
        g.setColour (C::muted()); g.setFont (FontOptions (10.0f)); g.drawText ("CONTROLLER", headArea.getX() + 67, headArea.getY() + 30, 120, 14, Justification::centredLeft);
        // status: MIDI out / in
        auto st = statusArea;
        auto dot = [&g, &st] (bool ok, const String& t, Colour on) { g.setColour (ok ? on : C::amber()); g.fillEllipse ((float) st.getX(), (float) st.getY() + 4, 8.0f, 8.0f); g.setColour (C::muted()); g.setFont (FontOptions (11.0f)); g.drawText (t, st.getX() + 12, st.getY(), st.getWidth() - 12, 16, Justification::centredLeft, true); st.translate (0, 16); };
        const bool dm = proc.getInt ("route", 0) != Mx400Processor::HostOut;
        dot (dm ? proc.outputOpen() : true, dm ? (proc.outputOpen() ? "MIDI out: " + proc.getStr ("outName") : "MIDI out: no port chosen") : "MIDI out: DAW output", C::mint());
        dot (proc.inputOpen(), proc.inputOpen() ? "MIDI in: " + proc.getStr ("inName") : "MIDI in: off (SysEx capture)", C::mint());
        if (page == 2) paintSurroundPage (g);
    }
    void mouseDoubleClick (const MouseEvent& e) override
    {
        if (e.x < 34 || e.x > getWidth() - 34) onReset();
    }
    void hintsChanged() { }
    int page = 0;
private:
    void paintSurroundPage (Graphics& g)
    {
        auto a = pageArea.removeFromLeft (300);
        panelBox (g, a.withHeight (pageArea.getHeight()), "Surround");
        g.setColour (C::text()); g.setFont (FontOptions (13.0f));
        g.drawFittedText ("Surround Hall controls are on the right. They send only while the configuration (top left) is Surround.\n\n"
                          "Not built yet, because they need an audio engine inside the plugin: per channel level trims, the front to rear depth, the pre mixed 5.1 and stereo downmix modes, and the return ducker. "
                          "The MX400 itself handles the surround reverb; use your DAW's hardware insert to route audio.",
                          a.reduced (16, 44), Justification::topLeft, 14);
    }
    Mx400Processor& proc; std::function<void()> onReset;
    std::unique_ptr<Look> look;
    ProgramBox progA, progB;
    TextButton cfgBtn[3], tabBtn[4], sendAll, gearB, resetB, infoB;
    OwnedArray<EffectPanel> effects;
    std::unique_ptr<EffectPanel> surroundFx;
    RoutingPanel routing; LibraryPage library; SettingsPage settings;
    Rectangle<int> headArea, statusArea, tabArea, pageArea;
    catui::RenderGate gate;
};
}

// ---- editor: scaled content, eight resize handles, wood double click ------------------------------
Mx400Editor::Mx400Editor (Mx400Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    content = std::make_unique<mxui::Content> (p, [this] { resetTo100(); });
    addAndMakeVisible (*content);
    constrainer.setFixedAspectRatio ((double) kW / kH);
    constrainer.setSizeLimits (kW / 2, kH / 2, kW * 2, kH * 2);
    setConstrainer (&constrainer);
    border = std::make_unique<ResizableBorderComponent> (this, &constrainer);
    border->setBorderThickness (BorderSize<int> (10));
    addAndMakeVisible (*border);
    setResizable (true, false);
    setSize (kW, kH);
    tooltips.setMillisecondsBeforeTipAppears (proc.getBool ("hints", true) ? 900 : 100000000);
}

Mx400Editor::~Mx400Editor() { content.reset(); }
void Mx400Editor::paint (Graphics& g) { g.fillAll (juce::Colour (0xff161638)); }
void Mx400Editor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    content->setBounds (0, 0, kW, kH);
    content->setTransform (AffineTransform::scale (s));
    border->setBounds (getLocalBounds());
    tooltips.setMillisecondsBeforeTipAppears (proc.getBool ("hints", true) ? 900 : 100000000);
}
void Mx400Editor::resetTo100() { setSize (kW, kH); }
void Mx400Editor::showPage (int page) { content->showPage (page); }
void Mx400Editor::rebuildAll() { content->rebuildAll(); }
