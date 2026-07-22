#pragma once
/*  Widgets — shared UI primitives for the Magic Drum Gate UI.
    Global `ui::pal` (palette) and `ui::scale` (legibility scale) are set by the
    editor before layout; every widget reads them directly (message thread only).
    Hints: setHint(comp, title, text) stores strings the HintBar picks up by
    walking the component tree under the mouse. */
#include <JuceHeader.h>
#include "../ThemeColors.h"

namespace ui
{
extern const theme::Palette* pal;
extern float scale;
inline int sc (float v)               { return juce::roundToInt (v * scale); }
inline float scf (float v)            { return v * scale; }
inline juce::Font font (float px, bool bold = false)
{
    return juce::Font (juce::FontOptions (px * scale, bold ? juce::Font::bold : juce::Font::plain));
}
inline double qToOct (double q)       { return (2.0 / std::log (2.0)) * std::asinh (1.0 / (2.0 * q)); }
juce::String fmtHz (double v);

// Toolbar logo: blue snare + striking stick + sparkles, "MAGIC DRUM GATE"
// wordmark with a gate-green → tail-gold underline. Draws at x = sc(12),
// designed for (and vertically centred in) the sc(46) toolbar strip.
void drawLogo (juce::Graphics& g);

void setHint (juce::Component& c, const juce::String& title, const juce::String& text);
// Returns "<b>title</b> — text" style pair for the component chain under `c`, or empty.
bool findHint (juce::Component* c, juce::String& title, juce::String& text);

//==============================================================================
/*  Rotary knob bound to a RangedAudioParameter. Drag vertically (Shift = fine),
    double-click resets, click the value text to type a value ("2k" = 2000).
    `reversed` flips the displayed direction (used for the ring-level knob).
    `fmt` overrides the value text. */
class Knob : public juce::Component
{
public:
    enum ColourId { accentClr = 0, openClr, tailClr };

    Knob (juce::String name, ColourId clr = accentClr);
    ~Knob() override;
    void attach (juce::RangedAudioParameter* p);          // may be re-attached
    void setNameText (const juce::String& n)              { name = n; repaint(); }
    void setFormat (std::function<juce::String (float)> f){ fmt = std::move (f); }
    void setReversed (bool r)                             { reversed = r; repaint(); }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    float shownNorm() const;
    juce::Rectangle<int> valueBounds() const;
    void showEditor();
    void applyTyped (const juce::String& text);

    juce::String name;
    ColourId clr;
    bool reversed = false;
    std::function<juce::String (float)> fmt;
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<juce::ParameterAttachment> att;
    std::unique_ptr<juce::TextEditor> edit;               // lazily created, reused
    struct ClickAway;
    std::unique_ptr<ClickAway> clickAway;                 // closes the editor on outside clicks
    float dragNorm = 0.0f;
    int lastDragY = 0;
    bool draggingKnob = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};

//==============================================================================
/*  Radio selector with LED lights: one row (or column) per option, each with
    a light + optional filter/shape icon + label. */
class LightSelector : public juce::Component
{
public:
    enum Icon { iconNone = 0, iconHP, iconLP, iconBP, iconBell, iconPropQ, iconShelf };
    struct Option { juce::String label; Icon icon; };

    LightSelector (std::vector<Option> options, Knob::ColourId clr = Knob::accentClr,
                   bool horizontal = false);
    std::function<void (int)> onChange;
    void setSelected (int i, bool notify);
    int  getSelected() const                  { return sel; }
    void paint (juce::Graphics& g) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::Rectangle<float> cell (int i) const;
    std::vector<Option> opts;
    Knob::ColourId clr;
    bool horizontal;
    int sel = 0;
};

//==============================================================================
/*  Toggle button styled like a LightSelector row: rounded button background
    with an LED light next to the label. */
class LightToggle : public juce::Component
{
public:
    explicit LightToggle (juce::String label, Knob::ColourId clr = Knob::accentClr);
    std::function<void()> onClick;
    void setText (juce::String l)              { label = std::move (l); repaint(); }
    void setColourId (Knob::ColourId c)        { clr = c; repaint(); }
    void setState (bool on);
    bool getState() const                      { return state; }
    void paint (juce::Graphics& g) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::String label;
    Knob::ColourId clr;
    bool state = false;
};

//==============================================================================
/*  Two-position mode switch: a pill holding both labels; the sliding thumb
    covers — and names — the ACTIVE side. Optional small caption above. */
class TextSwitch : public juce::Component
{
public:
    TextSwitch (juce::String leftLabel, juce::String rightLabel);
    std::function<void (bool leftActive)> onChange;
    void setCaption (juce::String c)           { caption = std::move (c); repaint(); }
    void setLeftActive (bool left, bool notify);
    bool isLeftActive() const                  { return leftActive; }
    void paint (juce::Graphics& g) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::String lLab, rLab, caption;
    bool leftActive = true;
};

//==============================================================================
/*  Tiny pill switch: dark knob left = off, highlighted knob right = on. */
class MiniSwitch : public juce::Component
{
public:
    explicit MiniSwitch (juce::String label = {});     // labelled: pill left, text right
    std::function<void (bool)> onChange;
    void setOnColour (juce::Colour c)          { onColour = c; repaint(); }
    void setState (bool on, bool notify);
    bool getState() const                      { return state; }
    void paint (juce::Graphics& g) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::String label;
    juce::Colour onColour;
    bool state = false;
};

//==============================================================================
/*  Small 0..1 slider (display/update-rate style controls), horizontal or
    vertical. Double-click resets to the default value. */
class MiniSlider : public juce::Component
{
public:
    explicit MiniSlider (float defaultValue = 0.5f, bool vertical = false);
    std::function<void (float)> onChange;
    float getValue() const                     { return value; }
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
private:
    void setFromPos (juce::Point<float> p);
    float value, def;
    bool vert;
};

//==============================================================================
/*  Icon-only freeze toggle: a snowflake meant to overlay a display corner. */
class SnowButton : public juce::Button
{
public:
    SnowButton();
    void paintButton (juce::Graphics& g, bool over, bool down) override;
};

//==============================================================================
/*  Vertical level meter, −60..0 dB, fill from bottom, peak-hold line, optional
    draggable threshold line (dashed, warn colour). */
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter (juce::String caption, std::function<float()> getDb,
                juce::RangedAudioParameter* thresholdParam = nullptr,
                bool showCaption = true, bool showValue = true);
    void setCaption (juce::String c)           { caption = std::move (c); repaint(); }
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    juce::MouseCursor getMouseCursor() override;

private:
    void timerCallback() override;
    juce::Rectangle<float> barArea() const;

    juce::String caption;
    std::function<float()> getDb;
    juce::RangedAudioParameter* threshold;
    bool showCaption, showValue, dragging = false;
    float shown = -90.0f, pk = -100.0f, pkT = 0.0f;
};

//==============================================================================
/*  GATE meter: mirrors the gate state chip. OPEN = green fill, TAIL = tail
    colour at the fading tail level, CLOSED = empty. `get` fills state
    (0 closed, 1 tail, 2 open), open01 and tail01. */
class GateMeter : public juce::Component, private juce::Timer
{
public:
    using Getter = std::function<int (float& open01, float& tail01)>;
    explicit GateMeter (Getter get);
    void paint (juce::Graphics& g) override;
private:
    void timerCallback() override;
    Getter get;
    int state = 0;
    float open01 = 0.0f, tail01 = 0.0f;
};

//==============================================================================
/*  GR meter (compressor mode): how hard the cancelling copy is compressed,
    0 at the top down to −48 dB (anything deeper is visually clipped). Green
    fill from the top — depth here means the hit is escaping the null. */
class GrMeter : public juce::Component, private juce::Timer
{
public:
    explicit GrMeter (std::function<float()> getDb);      // negative dB in
    void paint (juce::Graphics& g) override;
private:
    void timerCallback() override;
    std::function<float()> getDb;
    float shown = 0.0f;
};

//==============================================================================
/*  TAIL meter: 0..100 % ringing, tail colour. */
class PercentMeter : public juce::Component, private juce::Timer
{
public:
    explicit PercentMeter (std::function<float()> get01);
    void paint (juce::Graphics& g) override;
private:
    void timerCallback() override;
    std::function<float()> get01;
    float shown = 0.0f;
};

//==============================================================================
/*  Amount fader: thin ticked track with a real grip thumb. Bound to the
    intensity parameter. Thumb centre travels 7px + v·trackHeight from bottom. */
class AmountFader : public juce::Component
{
public:
    explicit AmountFader (juce::RangedAudioParameter* intensityParam);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
private:
    void setFromY (float y);
    juce::RangedAudioParameter* param;
    std::unique_ptr<juce::ParameterAttachment> att;
    bool dragging = false;
};

//==============================================================================
/*  Monitor-gain fader: vertical ±24 dB, display-only offset for the spectrum
    layers. Double-click resets to 0. */
class MonitorFader : public juce::Component
{
public:
    std::function<void (float)> onChange;
    float getValueDb() const                 { return value; }
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
private:
    void setFromY (float y);
    float value = 0.0f;
};

} // namespace ui
