#pragma once

/*
    PresetBrowser — factory + user preset selection, saving, deleting.

    Factory presets come from PresetDefaults.h. User presets are XML files in
    the user's application-data folder (see presetDirectory()).

    THE PER-TRACK CALIBRATION SET (eqids::preservedParamIds — thresholds,
    Selectivity, Hysteresis, Ratio, MIDI, Ext SC) IS NEVER TOUCHED BY
    PRESETS: factory presets apply around it, user presets are saved without
    it and loaded around it. DAW session state
    (getStateInformation/setStateInformation) still includes everything.
*/

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../ThemeColors.h"
#include "../PresetDefaults.h"

class PresetBrowser : public juce::Component
{
public:
    explicit PresetBrowser (MagicDrumDeBleedAudioProcessor& proc);

    void setPalette (const theme::Palette& p);
    void resized() override;

    static juce::File presetDirectory();

private:
    void refreshPresetList();
    void presetSelected();
    void applyFactoryPreset (const presets::FactoryPreset& preset);
    void resetAllParametersExceptPreserved();
    void setParameterValue (const juce::String& paramID, float realValue);
    void promptForSaveName();
    void saveUserPreset (const juce::String& name);
    void deleteSelectedUserPreset();

    MagicDrumDeBleedAudioProcessor& processor;
    const theme::Palette* pal = &theme::dark();

    juce::ComboBox presetBox;
    juce::TextButton saveButton   { "Save" };
    juce::TextButton deleteButton { "Delete" };
    juce::Array<juce::File> userPresetFiles;

    static constexpr int kUserIdOffset = 1000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
};
