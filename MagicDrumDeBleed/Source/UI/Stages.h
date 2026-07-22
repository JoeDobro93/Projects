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
    void setTitle (juce::String t)             { title = std::move (t); repaint(); }
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
    ui::LightToggle enableBtn { "Enabled" }, linkBtn { "Link to K1" }, midiBtn { "MIDI" };
    ui::TextSwitch modeSw { "COMP", "GATE" };
    eqids::LearnButton learnBtn { processor };
    std::unique_ptr<juce::ParameterAttachment> typeAtt, bypassAtt, scEnableAtt, linkAtt, midiAtt,
                                               compModeAtt;
    void updateSelectivityDim();
    int sensLabelX = 0, filtLabelX = 0, dividerX = 0;
    juce::Rectangle<int> modeLabelArea;
    bool filterOn = true, compOn = false;
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

    void applyMode (bool comp);

    MagicDrumDeBleedAudioProcessor& processor;
    StageHeader header { 2, "GATE", 1 };
    ui::Knob lookahead { "Lookahead", ui::Knob::openClr }, hold { "Hold", ui::Knob::openClr },
             release { "Release", ui::Knob::openClr }, hystK { "Hysteresis", ui::Knob::openClr };
    // Compressor-mode knobs share the grid slots (visibility follows the mode)
    ui::Knob ratioK { "Ratio", ui::Knob::openClr }, compAtk { "Attack", ui::Knob::openClr },
             compRel { "Release", ui::Knob::openClr };
    ui::MiniSlider speedSlider { 0.5f, true };          // history scroll speed
    ui::LightToggle outTg { "Output", ui::Knob::openClr }, trigTg { "Input" },
                    smoothTg { "Average" }, offTg { "Dry", ui::Knob::tailClr };
    ui::SnowButton histFreeze;
    bool showOut = true, showTrig = true, showSmooth = true, showOff = true;

    std::unique_ptr<juce::ParameterAttachment> dimAtt;  // dulls knobs on bypass
    std::unique_ptr<juce::ParameterAttachment> modeAtt;
    bool compOn = false;
    struct Sample { float det, fast, off, out, gr01, o01, t01; int state; bool forced; };
    static constexpr int kHist = 460;
    std::vector<Sample> hist;
    juce::Rectangle<int> canvasArea, stateArea, speedLabelArea;
    float open01 = 0.0f, tail01 = 0.0f;
    int state = 0;
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
    juce::TextButton internalsBtn { "Show internals" }, accumBtn { "Accumulate" };
    ui::SnowButton freezeBtn;
    ui::LightSelector scaleSel { { { "Fine", ui::LightSelector::iconNone },
                                   { "Med",  ui::LightSelector::iconNone },
                                   { "Wide", ui::LightSelector::iconNone } }, ui::Knob::tailClr };
    ui::MonitorFader mon;
    TailCanvas canvas;

    juce::TextButton bandBtns[7], soloBtns[7];
    ui::MiniSwitch bandSw[7];                           // enable switches
    std::unique_ptr<eqids::BandLearnButton> bandLearnBtns[5];   // K1..K5
    eqids::LearnButton learnAllBtn { processor, 5 };
    juce::TextButton lockBtn { "Lock freq" };
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
    ui::Knob tailHold { "Tail hold", ui::Knob::tailClr }, tailFade { "Tail fade", ui::Knob::tailClr },
             tailRangeK { "Tail range", ui::Knob::tailClr }, tailBaseK { "Tail base", ui::Knob::tailClr };
    ui::PercentMeter tailMeter;
    std::unique_ptr<juce::ParameterAttachment> compModeAtt;
    void updateDim();
    int sel = 0;
    int bandLabelsY = 0, shapeX = 0, scaleLabelY = 0;
    bool eqByp = false, tgOn = true, compOn = false;
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
    ui::GrMeter grMeter;                                // comp mode: replaces GATE
    juce::TextButton monBtns[3];                        // Output / Removed bleed / Trigger signal
    std::unique_ptr<juce::ParameterAttachment> monAtt, amtAtt, modeAtt, contrastAtt;
    void updateLatencyText();
    juce::String latencyText;
    int latencyY = 0;
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
