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

void setHint (juce::Component& c, const juce::String& title, const juce::String& text);
// Returns "<b>title</b> — text" style pair for the component chain under `c`, or empty.
bool findHint (juce::Component* c, juce::String& title, juce::String& text);

//==============================================================================
/*  Rotary knob bound to a RangedAudioParameter. Drag vertically (Shift = fine),
    double-click resets. `reversed` flips the displayed direction (used for the
    octave-width and ring-level knobs). `fmt` overrides the value text. */
class Knob : public juce::Component
{
public:
    enum ColourId { accentClr = 0, openClr, tailClr };

    Knob (juce::String name, ColourId clr = accentClr);
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

    juce::String name;
    ColourId clr;
    bool reversed = false;
    std::function<juce::String (float)> fmt;
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<juce::ParameterAttachment> att;
    float dragStartNorm = 0.0f;
    int dragStartY = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
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
/*  REDUCTION meter: hangs downward from the top (classic GR style), warn
    gradient, value text ("—" when idle). */
class ReductionMeter : public juce::Component, private juce::Timer
{
public:
    ReductionMeter (std::function<float()> getDb, bool showCaption = true, bool showValue = true);
    void paint (juce::Graphics& g) override;
private:
    void timerCallback() override;
    std::function<float()> getDb;
    bool showCaption, showValue;
    float shown = -90.0f;
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
/*  Mockup-style checkbox toggle: 14px rounded box + tick + label. */
class CheckToggle : public juce::Component
{
public:
    CheckToggle (juce::String label, bool tailColour = false);
    void setState (bool on, bool notify);
    bool getState() const                    { return state; }
    std::function<void (bool)> onChange;
    void paint (juce::Graphics& g) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::String label;
    bool tailColour, state = true;
};
} // namespace ui
