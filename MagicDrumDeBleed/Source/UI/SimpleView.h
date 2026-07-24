#pragma once
/*  SimpleView — stripped-back: TRIGGER meter (draggable threshold), GR,
    AMOUNT fader, OUT meter; Resonance (= Focus) with Learn under it,
    Reso Amt (= K1 ring level) and Tail hold; Kick/Snare/Toms preset row,
    and an Advanced View button. */
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Widgets.h"
#include "BandIds.h"

class SimpleView : public juce::Component
{
public:
    static constexpr int kDefaultW = 380, kDefaultH = 692;
    static constexpr int kMinW = 330,  kMinH = 608;

    SimpleView (MagicDrumDeBleedAudioProcessor& proc, std::function<void()> onThemeToggle,
                std::function<void()> onAdvancedView);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MagicDrumDeBleedAudioProcessor& processor;
    ui::LevelMeter trigMeter;
    ui::GrMeter grMeter;                                // duck depth = escape
    ui::AmountFader fader;
    juce::Label amountVal;
    ui::LevelMeter outMeter;
    ui::Knob resonance { "Resonance" }, resoAmt { "Reso Amt", ui::Knob::tailClr },
             tailHold { "Tail hold", ui::Knob::tailClr }, tailFade { "Tail fade", ui::Knob::tailClr };
    eqids::LearnButton learnBtn { processor, 3 };       // fills the lowest 3 bands it can
    ui::LightToggle midiTg { "MIDI" };
    class DrumButton;
    std::unique_ptr<juce::Button> presetBtns[4];
    juce::TextButton advancedBtn { "Advanced View" }, themeBtn;
    std::unique_ptr<juce::ParameterAttachment> amtAtt, midiAtt;
    int amountX = 0, amountY = 0;
    juce::Rectangle<int> divider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleView)
};
