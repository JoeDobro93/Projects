#pragma once
/*  SimpleView — stripped-back: TRIGGER meter (draggable threshold),
    REDUCTION, AMOUNT fader, OUT meter, and an Advanced View button. */
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Widgets.h"

class SimpleView : public juce::Component
{
public:
    static constexpr int kDefaultW = 380, kDefaultH = 430;
    static constexpr int kMinW = 330,  kMinH = 360;

    SimpleView (MagicDrumDeBleedAudioProcessor& proc, std::function<void()> onAdvancedView);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MagicDrumDeBleedAudioProcessor& processor;
    ui::LevelMeter trigMeter;
    ui::ReductionMeter redMeter;
    ui::AmountFader fader;
    juce::Label amountVal;
    ui::LevelMeter outMeter;
    juce::TextButton advancedBtn { "Advanced View" };
    std::unique_ptr<juce::ParameterAttachment> amtAtt;
    int amountX = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleView)
};
