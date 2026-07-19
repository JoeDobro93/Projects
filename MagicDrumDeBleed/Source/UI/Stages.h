#pragma once
/*  Stages — the three numbered stage panels, the right rail and the hint bar.
    Layout follows the mockup: fixed heights for stages 1/2 (164s/172s), stage 3
    stretches; rail 150s wide; hint bar 34s. All hint strings verbatim from the
    mockup's data-hint attributes. */
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Widgets.h"
#include "TailCanvas.h"

//==============================================================================
class StageHeader : public juce::Component
{
public:
    StageHeader (int number, juce::String title, int colourSel); // 0 accent,1 open,2 tail
    void paint (juce::Graphics& g) override;
private:
    int num, clr;
    juce::String title;
};

//==============================================================================
class TriggerStage : public juce::Component
{
public:
    explicit TriggerStage (MagicDrumDeBleedAudioProcessor& proc);
    void paint (juce::Graphics& g) override;
    void resized() override;
private:
    void rebuildTypeButtons();
    void updateWidthKnob();

    MagicDrumDeBleedAudioProcessor& processor;
    StageHeader header { 1, "TRIGGER", 0 };
    juce::TextButton bypassBtn { "Bypass" };
    ui::LevelMeter trigMeter;
    ui::Knob threshold { "Threshold" }, smoothing { "Smoothing" }, focus { "Focus" }, width { "Width" };
    juce::TextButton typeBtns[3];
    juce::TextButton enableBtn { "Enabled" }, learnBtn { "Learn" }, linkBtn { "Link to K1" };
    std::unique_ptr<juce::ParameterAttachment> typeAtt, bypassAtt, scEnableAtt, linkAtt;
    int sensLabelX = 0, filtLabelX = 0, dividerX = 0;
};

//==============================================================================
class GateStage : public juce::Component, private juce::Timer
{
public:
    explicit GateStage (MagicDrumDeBleedAudioProcessor& proc);
    void paint (juce::Graphics& g) override;
    void resized() override;
private:
    void timerCallback() override;

    MagicDrumDeBleedAudioProcessor& processor;
    StageHeader header { 2, "GATE", 1 };
    ui::Knob lookahead { "Lookahead", ui::Knob::openClr }, hold { "Hold", ui::Knob::openClr },
             release { "Release", ui::Knob::openClr };

    struct Sample { float det; int state; };            // state 0 closed 1 tail 2 open
    static constexpr int kHist = 460;
    std::vector<Sample> hist;
    juce::Rectangle<int> canvasArea, stateArea;
    float open01 = 0.0f, tail01 = 0.0f;
    int state = 0;
    juce::String latencyText;
};

//==============================================================================
class TailStage : public juce::Component
{
public:
    explicit TailStage (MagicDrumDeBleedAudioProcessor& proc);
    ~TailStage() override;                              // clears band solo
    void paint (juce::Graphics& g) override;
    void resized() override;
private:
    void selectBand (int b);
    void rebuildBandRow();
    void rebuildShapeSeg();
    void updateSoloButtons();

    MagicDrumDeBleedAudioProcessor& processor;
    StageHeader header { 3, "TAIL", 2 };
    ui::CheckToggle tailGateTg { "Tail gate", true };
    juce::TextButton internalsBtn { "Show internals" }, accumBtn { "Accumulate" }, freezeBtn { "Freeze" };
    ui::MonitorFader mon;
    TailCanvas canvas;

    juce::TextButton bandBtns[7], soloBtns[7];
    juce::TextButton dotBtns[7];                        // enable dots
    std::unique_ptr<juce::ParameterAttachment> onAtts[7];
    juce::Label bandLabel;
    ui::Knob freq { "Frequency", ui::Knob::tailClr }, widthK { "Q", ui::Knob::tailClr },
             ring { "Ring level", ui::Knob::tailClr },
             slopeK { "Slope", ui::Knob::tailClr };     // LOWS/HIGHS only
    juce::TextButton shapeBtns[3];
    bool selIsKeep = true;
    std::unique_ptr<juce::ParameterAttachment> shapeAtt, tailGateAtt;
    ui::Knob tailHold { "Tail hold", ui::Knob::tailClr }, tailFade { "Tail fade", ui::Knob::tailClr };
    ui::PercentMeter tailMeter;
    int sel = 0;
    int bandLabelsY = 0, shapeX = 0;
};

//==============================================================================
class RightRail : public juce::Component
{
public:
    explicit RightRail (MagicDrumDeBleedAudioProcessor& proc);
    void paint (juce::Graphics& g) override;
    void resized() override;
private:
    MagicDrumDeBleedAudioProcessor& processor;
    ui::AmountFader fader;
    juce::Label amountVal;
    ui::LevelMeter outMeter;
    ui::GateMeter gateMeter;
    juce::TextButton monBtns[3];                        // Output / Removed bleed / Trigger signal
    std::unique_ptr<juce::ParameterAttachment> monAtt, amtAtt;
    int listenY = 0;
};

//==============================================================================
class HintBar : public juce::Component, private juce::Timer
{
public:
    HintBar();
    void paint (juce::Graphics& g) override;
    juce::TextButton simpleBtn { "Simple view" };
    void resized() override;
private:
    void timerCallback() override;
    juce::String title, text;
};
