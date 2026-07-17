#pragma once

/*
    PluginEditor — hosts two views (Advanced / Simple) and gives them a
    locked aspect ratio with uniform scaling.

    Each view is laid out ONCE at a fixed logical size; the editor applies an
    AffineTransform scale so every element grows/shrinks together and the
    proportions are identical at any window size. The window's aspect ratio is
    pinned to the active view's logical ratio, so nothing ever stretches or
    squishes. Each view remembers its own last size.
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ThemeColors.h"
#include "UI/AdvancedView.h"
#include "UI/SimpleView.h"

//==============================================================================
class StyledLookAndFeel : public juce::LookAndFeel_V4
{
public:
    StyledLookAndFeel()   { setPalette (theme::dark()); }

    void setPalette (const theme::Palette& p);

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    // Wider, taller value box so typed text is never clipped.
    juce::Label* createSliderTextBox (juce::Slider& slider) override;

private:
    const theme::Palette* pal = &theme::dark();
};

//==============================================================================
class MagicDrumDeBleedAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MagicDrumDeBleedAudioProcessorEditor (MagicDrumDeBleedAudioProcessor& proc);
    ~MagicDrumDeBleedAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void applyTheme();
    void toggleTheme();
    void setView (bool simple);
    void configureConstrainerForView (bool simple);

    // Scale limits (window size = logical × scale). Minimum is 1.0 so the
    // design never renders below its comfortable logical proportions.
    static constexpr float kMinScale = 1.0f;
    static constexpr float kMaxScale = 2.2f;

    MagicDrumDeBleedAudioProcessor& processor;
    StyledLookAndFeel lookAndFeel;
    const theme::Palette* pal = &theme::dark();

    AdvancedView advancedView;
    SimpleView   simpleView;
    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagicDrumDeBleedAudioProcessorEditor)
};
