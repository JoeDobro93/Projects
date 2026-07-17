#pragma once

/*
    UIHelpers — small shared UI building blocks:
      - LabelledKnob : a rotary slider with a bold caption and a value box.
      - SlideSwitch  : a compact two-state switch flanked by two labels, the
                       active side highlighted (used for Input/Output levels).
*/

#include <JuceHeader.h>
#include "../ThemeColors.h"

namespace ui
{

struct LabelledKnob : public juce::Component
{
    explicit LabelledKnob (const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int labelHeight   = juce::jlimit (12, 20, bounds.getHeight() / 5);
        const int textBoxHeight = juce::jlimit (16, 22, bounds.getHeight() / 5);

        label.setBounds (bounds.removeFromTop (labelHeight));
        label.setFont (juce::Font (juce::FontOptions ((float) labelHeight - 2.0f, juce::Font::bold)));
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                                juce::jmin (bounds.getWidth(), 76), textBoxHeight);
        slider.setBounds (bounds);
    }

    juce::Slider slider;
    juce::Label  label;
};

//==============================================================================
class SlideSwitch : public juce::Component
{
public:
    SlideSwitch (juce::String left, juce::String right, std::function<void (bool)> onChange)
        : leftLabel (std::move (left)), rightLabel (std::move (right)), callback (std::move (onChange)) {}

    void setPalette (const theme::Palette* p)   { pal = p; repaint(); }
    void setOnChange (std::function<void (bool)> cb)   { callback = std::move (cb); }

    // false = left option active, true = right option active.
    void setState (bool rightActive, bool notify = false)
    {
        if (rightActive == state)
            return;
        state = rightActive;
        if (notify && callback)
            callback (state);
        repaint();
    }
    bool getState() const   { return state; }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float trackW = juce::jlimit (26.0f, 40.0f, b.getWidth() * 0.22f);
        auto track = b.withSizeKeepingCentre (trackW, juce::jmin (b.getHeight() - 4.0f, 16.0f));

        auto leftArea  = b.withRight (track.getX() - 4.0f);
        auto rightArea = b.withLeft (track.getRight() + 4.0f);

        g.setFont (juce::Font (juce::FontOptions (juce::jlimit (9.0f, 12.0f, b.getHeight() * 0.5f))));
        g.setColour (state ? pal->textDim : pal->text);
        g.drawText (leftLabel, leftArea, juce::Justification::centredRight, false);
        g.setColour (state ? pal->text : pal->textDim);
        g.drawText (rightLabel, rightArea, juce::Justification::centredLeft, false);

        g.setColour (pal->knobTrack);
        g.fillRoundedRectangle (track, track.getHeight() * 0.5f);
        g.setColour (pal->highlight);
        const float knob = track.getHeight() - 3.0f;
        const float kx = state ? track.getRight() - knob - 1.5f : track.getX() + 1.5f;
        g.fillEllipse (kx, track.getCentreY() - knob * 0.5f, knob, knob);
    }

    void mouseDown (const juce::MouseEvent&) override   { setState (! state, true); }

private:
    juce::String leftLabel, rightLabel;
    std::function<void (bool)> callback;
    bool state = false;
    const theme::Palette* pal = &theme::dark();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlideSwitch)
};

//==============================================================================
// A button that paints one of the notch-curve icons (0 bell, 1 flat, 2 notch).
class CurveIconButton : public juce::Button
{
public:
    CurveIconButton() : juce::Button ({}) { setClickingTogglesState (false); }

    void setShape (int s)   { shape = s; repaint(); }
    void setPalette (const theme::Palette* p)   { pal = p; repaint(); }
    void setSelected (bool s)   { selected = s; repaint(); }

    void paintButton (juce::Graphics& g, bool highlighted, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.5f);
        g.setColour (selected ? pal->highlight.withAlpha (0.5f) : pal->buttonOff);
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (selected ? pal->text : pal->buttonOutline);
        g.drawRoundedRectangle (b, 3.0f, highlighted ? 1.4f : 1.0f);

        auto p = b.reduced (b.getWidth() * 0.18f, b.getHeight() * 0.26f);
        const float top = p.getY(), bot = p.getBottom(), l = p.getX(), rr = p.getRight();
        const float mid = p.getCentreX();

        juce::Path path;
        path.startNewSubPath (l, top);
        if (shape == 0)         // Bell — smooth rounded dip
        {
            path.cubicTo (mid - p.getWidth() * 0.18f, top, mid - p.getWidth() * 0.10f, bot,
                          mid, bot);
            path.cubicTo (mid + p.getWidth() * 0.10f, bot, mid + p.getWidth() * 0.18f, top,
                          rr, top);
        }
        else if (shape == 1)    // Flat — flat-bottom trough
        {
            path.lineTo (mid - p.getWidth() * 0.22f, top);
            path.lineTo (mid - p.getWidth() * 0.10f, bot);
            path.lineTo (mid + p.getWidth() * 0.10f, bot);
            path.lineTo (mid + p.getWidth() * 0.22f, top);
            path.lineTo (rr, top);
        }
        else                    // Notch — sharp deep V
        {
            path.lineTo (mid, bot);
            path.lineTo (rr, top);
        }

        g.setColour (selected ? pal->title : pal->text);
        g.strokePath (path, juce::PathStrokeType (1.6f));
    }

private:
    int shape = 0;
    bool selected = false;
    const theme::Palette* pal = &theme::dark();
};

} // namespace ui
