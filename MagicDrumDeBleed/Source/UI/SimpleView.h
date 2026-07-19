#pragma once
/*  SimpleView — stripped-back: TRIGGER meter (draggable threshold), GATE,
    AMOUNT fader, OUT meter, plus Focus / Tail hold knobs and a Learn button,
    and an Advanced View button. */
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Widgets.h"

class SimpleView : public juce::Component
{
public:
    static constexpr int kDefaultW = 380, kDefaultH = 520;
    static constexpr int kMinW = 330,  kMinH = 450;

    SimpleView (MagicDrumDeBleedAudioProcessor& proc, std::function<void()> onAdvancedView);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MagicDrumDeBleedAudioProcessor& processor;
    ui::LevelMeter trigMeter;
    ui::GateMeter gateMeter;
    ui::AmountFader fader;
    juce::Label amountVal;
    ui::LevelMeter outMeter;
    ui::Knob focus { "Focus" }, tailHold { "Tail hold", ui::Knob::tailClr };
    juce::TextButton learnBtn { "Learn" };
    juce::TextButton advancedBtn { "Advanced View" };
    std::unique_ptr<juce::ParameterAttachment> amtAtt;
    int amountX = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleView)
};
