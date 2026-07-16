#pragma once

/*
    UIHelpers — tiny shared UI building blocks (a labelled rotary knob).
    Header-only; used by the panel components.
*/

#include <JuceHeader.h>

namespace ui
{

struct LabelledKnob : public juce::Component
{
    explicit LabelledKnob (const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
        addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int labelHeight   = juce::jlimit (11, 18, bounds.getHeight() / 5);
        const int textBoxHeight = juce::jlimit (12, 18, bounds.getHeight() / 5);

        label.setBounds (bounds.removeFromTop (labelHeight));
        label.setFont (juce::Font (juce::FontOptions ((float) labelHeight - 2.0f)));
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                                juce::jmin (bounds.getWidth(), 72), textBoxHeight);
        slider.setBounds (bounds);
    }

    juce::Slider slider;
    juce::Label  label;
};

} // namespace ui
