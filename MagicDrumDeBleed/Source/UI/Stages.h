#pragma once
/*  Stages — the three numbered stage panels, the right rail and the hint bar.
    Layout follows the mockup: fixed heights for stages 1/2 (164s/172s), stage 3
    stretches; rail 150s wide; hint bar 34s. All hint strings verbatim from the
    mockup's data-hint attributes. */
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Widgets.h"
#include "TailCanvas.h"
#include "BandIds.h"

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
    void updateWidthKnob();

    MagicDrumDeBleedAudioProcessor& processor;
    StageHeader header { 1, "TRIGGER", 0 };
    juce::TextButton bypassBtn { "Bypass" };
    ui::LevelMeter trigMeter;
    ui::Knob threshold { "Threshold" }, smoothing { "Smoothing" }, focus { "Focus" }, width { "Width" },
             contrastK { "Selectivity" };
    ui::LightSelector typeSel { { { "High Pass", ui::LightSelector::iconHP },
                                  { "Low Pass",  ui::LightSelector::iconLP },
                                  { "Bandpass",  ui::LightSelector::iconBP } } };
    ui::LightToggle enableBtn { "Enabled" }, linkBtn { "Link to K1" };
    eqids::LearnButton learnBtn { processor };
    std::unique_ptr<juce::ParameterAttachment> typeAtt, bypassAtt, scEnableAtt, linkAtt;
    int sensLabelX = 0, filtLabelX = 0, dividerX = 0;
    bool filterOn = true;
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
             release { "Release", ui::Knob::openClr },
             attackK { "Attack", ui::Knob::openClr }, hystK { "Hysteresis", ui::Knob::openClr };
    ui::MiniSlider speedSlider { 0.5f, true };          // history scroll speed

    std::unique_ptr<juce::ParameterAttachment> dimAtt;  // dulls knobs on bypass
    struct Sample { float det, fast, off; int state; }; // state 0 closed 1 tail 2 open
    static constexpr int kHist = 460;
    std::vector<Sample> hist;
    juce::Rectangle<int> canvasArea, stateArea, speedLabelArea;
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
    ui::MiniSwitch tailGateSw { "Tail gate" };
    juce::TextButton bypassBtn { "Bypass" };
    juce::TextButton internalsBtn { "Show internals" }, accumBtn { "Accumulate" }, freezeBtn { "Freeze" };
    ui::LightSelector scaleSel { { { "Fine", ui::LightSelector::iconNone },
                                   { "Med",  ui::LightSelector::iconNone },
                                   { "Wide", ui::LightSelector::iconNone } }, ui::Knob::tailClr };
    ui::MonitorFader mon;
    TailCanvas canvas;

    juce::TextButton bandBtns[7], soloBtns[7];
    ui::MiniSwitch bandSw[7];                           // enable switches
    std::unique_ptr<juce::ParameterAttachment> onAtts[7];
    juce::Label bandLabel;
    ui::Knob freq { "Frequency", ui::Knob::tailClr }, widthK { "Q", ui::Knob::tailClr },
             ring { "Ring level", ui::Knob::tailClr },
             slopeK { "Slope", ui::Knob::tailClr };     // LOWS/HIGHS only
    ui::LightSelector shapeSel { { { "Bell",   ui::LightSelector::iconBell },
                                   { "Prop Q", ui::LightSelector::iconPropQ },
                                   { "Shelf",  ui::LightSelector::iconShelf } }, ui::Knob::tailClr };
    bool selIsKeep = true;
    std::unique_ptr<juce::ParameterAttachment> shapeAtt, tailGateAtt, bypassAtt;
    ui::Knob tailHold { "Tail hold", ui::Knob::tailClr }, tailFade { "Tail fade", ui::Knob::tailClr };
    ui::PercentMeter tailMeter;
    void updateDim();
    int sel = 0;
    int bandLabelsY = 0, shapeX = 0, scaleLabelY = 0;
    bool eqByp = false, tgOn = true;
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
    int listenY = 0, amountX = 0, amountY = 0;
};

//==============================================================================
class HintBar : public juce::Component, private juce::Timer
{
public:
    HintBar();
    ~HintBar() override;
    void paint (juce::Graphics& g) override;
    juce::TextButton simpleBtn { "Simple view" };
    void resized() override;
    // Global mouse listener: locks the hint to the pressed control while
    // dragging, instead of whatever the pointer happens to cross.
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    juce::Component::SafePointer<juce::Component> pressed;
    juce::String title, text;
};
