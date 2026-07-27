#include "PresetBrowser.h"
#include "BandIds.h"

//==============================================================================
namespace
{
    // Small name-entry component shown in a CallOutBox when saving.
    class NamePrompt : public juce::Component
    {
    public:
        explicit NamePrompt (std::function<void (juce::String)> onConfirm)
            : confirmCallback (std::move (onConfirm))
        {
            label.setText ("Preset name:", juce::dontSendNotification);
            addAndMakeVisible (label);

            editor.setTextToShowWhenEmpty ("My preset", juce::Colours::grey);
            editor.onReturnKey = [this] { confirm(); };
            addAndMakeVisible (editor);

            okButton.onClick = [this] { confirm(); };
            addAndMakeVisible (okButton);

            setSize (220, 76);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (8);
            label.setBounds (r.removeFromTop (18));
            r.removeFromTop (4);
            auto row = r.removeFromTop (24);
            okButton.setBounds (row.removeFromRight (56));
            row.removeFromRight (4);
            editor.setBounds (row);
        }

        void visibilityChanged() override
        {
            if (isVisible())
                editor.grabKeyboardFocus();
        }

    private:
        void confirm()
        {
            const auto name = editor.getText().trim();
            if (name.isEmpty())
                return;

            if (confirmCallback)
                confirmCallback (name);

            if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
                box->dismiss();
        }

        std::function<void (juce::String)> confirmCallback;
        juce::Label label;
        juce::TextEditor editor;
        juce::TextButton okButton { "Save" };
    };
}

//==============================================================================
PresetBrowser::PresetBrowser (MagicDrumDeBleedAudioProcessor& proc)
    : processor (proc)
{
    presetDirectory().createDirectory();

    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.onChange = [this] { presetSelected(); };
    addAndMakeVisible (presetBox);

    saveButton.onClick = [this] { promptForSaveName(); };
    addAndMakeVisible (saveButton);

    deleteButton.onClick = [this] { deleteSelectedUserPreset(); };
    deleteButton.setEnabled (false);
    addAndMakeVisible (deleteButton);

    refreshPresetList();
}

juce::File PresetBrowser::presetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
             .getChildFile ("MagicDrumDeBleed")
             .getChildFile ("Presets");
}

void PresetBrowser::setPalette (const theme::Palette& p)
{
    pal = &p;
    repaint();
}

void PresetBrowser::resized()
{
    auto r = getLocalBounds();
    deleteButton.setBounds (r.removeFromRight (juce::jlimit (44, 60, getWidth() / 6)).reduced (1));
    r.removeFromRight (3);
    saveButton.setBounds (r.removeFromRight (juce::jlimit (40, 56, getWidth() / 7)).reduced (1));
    r.removeFromRight (3);
    presetBox.setBounds (r.reduced (0, 1));
}

//==============================================================================
void PresetBrowser::refreshPresetList()
{
    presetBox.clear (juce::dontSendNotification);

    presetBox.addSectionHeading ("Factory");
    for (int i = 0; i < presets::kNumFactoryPresets; ++i)
        presetBox.addItem (presets::kFactoryPresets[i].name, i + 1);

    userPresetFiles = presetDirectory().findChildFiles (juce::File::findFiles, false, "*.xml");
    userPresetFiles.sort();

    if (! userPresetFiles.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("User");
        for (int i = 0; i < userPresetFiles.size(); ++i)
            presetBox.addItem (userPresetFiles[i].getFileNameWithoutExtension(), kUserIdOffset + i);
    }
}

void PresetBrowser::presetSelected()
{
    const int id = presetBox.getSelectedId();
    deleteButton.setEnabled (id >= kUserIdOffset);

    if (id >= 1 && id <= presets::kNumFactoryPresets)
    {
        applyFactoryPreset (presets::kFactoryPresets[id - 1]);
    }
    else if (id >= kUserIdOffset)
    {
        const int index = id - kUserIdOffset;
        if (index < userPresetFiles.size())
        {
            if (auto xml = juce::parseXML (userPresetFiles[index]))
            {
                resetAllParametersExceptPreserved();
                for (auto* param : xml->getChildWithTagNameIterator ("PARAM"))
                {
                    const auto paramID = param->getStringAttribute ("id");
                    if (! eqids::preservedParamIds().contains (paramID))
                        setParameterValue (paramID, (float) param->getDoubleAttribute ("value"));
                }
            }
        }
    }
}

void PresetBrowser::setParameterValue (const juce::String& paramID, float realValue)
{
    if (auto* param = processor.apvts.getParameter (paramID))
        param->setValueNotifyingHost (param->convertTo0to1 (realValue));
}

void PresetBrowser::resetAllParametersExceptPreserved()
{
    for (auto* p : processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! eqids::preservedParamIds().contains (ranged->paramID))
                ranged->setValueNotifyingHost (ranged->getDefaultValue());
}

void PresetBrowser::applyFactoryPreset (const presets::FactoryPreset& preset)
{
    eqids::applyFactoryPreset (processor, preset);
}

//==============================================================================
void PresetBrowser::promptForSaveName()
{
    auto prompt = std::make_unique<NamePrompt> (
        [safe = juce::Component::SafePointer<PresetBrowser> (this)] (juce::String name)
        {
            if (safe != nullptr)
                safe->saveUserPreset (name);
        });

    juce::CallOutBox::launchAsynchronously (std::move (prompt),
                                            saveButton.getScreenBounds(), nullptr);
}

void PresetBrowser::saveUserPreset (const juce::String& name)
{
    auto xml = processor.apvts.copyState().createXml();
    if (xml == nullptr)
        return;

    // Presets never carry the per-track calibration set, nor any editor/
    // display state (theme, view sizes, history prefs — all stored as root
    // attributes; the loader only reads PARAM children anyway).
    for (int i = xml->getNumChildElements(); --i >= 0;)
    {
        auto* child = xml->getChildElement (i);
        if (child->hasTagName ("PARAM")
            && eqids::preservedParamIds().contains (child->getStringAttribute ("id")))
            xml->removeChildElement (child, true);
    }
    xml->removeAllAttributes();

    const auto file = presetDirectory().getChildFile (juce::File::createLegalFileName (name) + ".xml");
    xml->writeTo (file);

    refreshPresetList();
    for (int i = 0; i < userPresetFiles.size(); ++i)
        if (userPresetFiles[i] == file)
            presetBox.setSelectedId (kUserIdOffset + i, juce::dontSendNotification);
    deleteButton.setEnabled (presetBox.getSelectedId() >= kUserIdOffset);
}

void PresetBrowser::deleteSelectedUserPreset()
{
    const int id = presetBox.getSelectedId();
    if (id < kUserIdOffset)
        return;

    const int index = id - kUserIdOffset;
    if (index < userPresetFiles.size())
        userPresetFiles[index].deleteFile();

    refreshPresetList();
    presetBox.setSelectedId (0, juce::dontSendNotification);
    presetBox.setText ("", juce::dontSendNotification);
    deleteButton.setEnabled (false);
}
