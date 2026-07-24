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
    // The complete gate lives here: sensitivity row + hysteresis/hold + MIDI.
    ui::Knob threshold { "Threshold" }, smoothing { "Smoothing" },
             hystK { "Hysteresis" }, holdK { "Hold" },
             focus { "Focus" }, width { "Width" }, contrastK { "Selectivity" };
    ui::LightSelector typeSel { { { "High Pass", ui::LightSelector::iconHP },
                                  { "Low Pass",  ui::LightSelector::iconLP },
                                  { "Bandpass",  ui::LightSelector::iconBP } } };
    ui::LightToggle enableBtn { "Enabled" }, linkBtn { "Link to K1" }, midiBtn { "MIDI" },
                    extScBtn { "Ext SC" };
    eqids::LearnButton learnBtn { processor };
    std::unique_ptr<juce::ParameterAttachment> typeAtt, bypassAtt, scEnableAtt, linkAtt, midiAtt,
                                               extScAtt;
    void updateSelectivityDim();
    int sensLabelX = 0, filtLabelX = 0, dividerX = 0;
    bool filterOn = true;
};

//==============================================================================
class GateStage : public juce::Component, private juce::Timer,
                  private juce::AudioProcessorParameter::Listener
{
public:
    explicit GateStage (MagicDrumDeBleedAudioProcessor& proc);
    ~GateStage() override;
    void paint (juce::Graphics& g) override;
    void resized() override;
private:
    void timerCallback() override;

    // Auto-follow for the GATE/COMP line switch: a user gesture on any of
    // the four threshold/range knobs flips the history to that stage's
    // lines. Hosts never send gestures for automation, so only real
    // touches switch the view.
    void parameterValueChanged (int, float) override {}
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;
    void setLinesComp (bool comp);

    MagicDrumDeBleedAudioProcessor& processor;
    StageHeader header { 2, "COMPRESSOR", 1 };
    ui::Knob compThreshK { "Threshold", ui::Knob::openClr }, ratioK { "Ratio", ui::Knob::openClr },
             compAtk { "Attack", ui::Knob::openClr }, compRmsK { "RMS", ui::Knob::openClr },
             compRel { "Release", ui::Knob::openClr }, lookahead { "Lookahead", ui::Knob::openClr };
    ui::MiniSlider speedSlider { 0.5f, true };          // history scroll speed
    // Trace legend chips (top-left overlay; LED = trace colour, click to
    // hide, persisted) + the GATE/COMP dashed-line switch (top-right).
    ui::LightToggle outTg { "Output", ui::Knob::openClr }, trigTg { "Trigger" },
                    smoothTg { "Average" }, offTg { "Dry", ui::Knob::tailClr };
    ui::TextSwitch lineSw { "GATE", "COMP" };
    ui::SnowButton histFreeze;
    bool showOut = false, showTrig = true, showSmooth = false, showOff = false;
    bool linesComp = false;                             // which line pair shows
    juce::RangedAudioParameter* lineParams[4] = {};     // thr, hyst, compThr, tailRange
    struct Sample { float det, fast, off, out, o01, t01; };
    static constexpr int kHist = 460;
    std::vector<Sample> hist;
    juce::Rectangle<int> canvasArea, speedLabelArea;
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
    juce::TextButton bypassBtn { "Bypass" };            // global, latency-preserving
    ui::AmountFader fader;
    juce::Label amountVal;
    ui::LevelMeter outMeter;
    ui::GrMeter grMeter;                                // duck depth = escape
    juce::TextButton monBtns[3];                        // Output / Removed bleed / Trigger signal
    std::unique_ptr<juce::ParameterAttachment> monAtt, amtAtt, contrastAtt, bypassAtt;
    void updateLatencyText (float contrastDb);
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
