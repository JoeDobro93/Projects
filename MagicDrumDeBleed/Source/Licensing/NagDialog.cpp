#include "NagDialog.h"

#if ENABLE_DONATION_NAG

#include "LicenseValidator.h"

namespace license
{

NagDialog::NagDialog()
{
    setSize (420, 210);

    messageLabel.setText ("Thanks for using Magic Drum De-Bleed!\n\n"
                          "This plugin is donationware: it is fully functional either way, "
                          "but if it earns its keep in your sessions, please consider "
                          "registering to support development.",
                          juce::dontSendNotification);
    messageLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (messageLabel);

    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);
    statusLabel.setVisible (false);

    keyEditor.setTextToShowWhenEmpty ("XXXX-XXXX-XXXX-XXXX", juce::Colours::grey);
    addAndMakeVisible (keyEditor);
    keyEditor.setVisible (false);

    closeButton.onClick    = [this] { closeWindow(); };
    registerButton.onClick = [this] { showRegistrationField(); };
    submitButton.onClick   = [this] { tryRegister(); };

    addAndMakeVisible (closeButton);
    addAndMakeVisible (registerButton);
    addAndMakeVisible (submitButton);
    submitButton.setVisible (false);
}

void NagDialog::launchIfNeeded()
{
    if (! LicenseValidator::shouldShowNag())
        return;

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (new NagDialog());
    options.dialogTitle                   = "Magic Drum De-Bleed — Registration";
    options.escapeKeyTriggersCloseButton  = true;
    options.useNativeTitleBar             = true;
    options.resizable                     = false;
    options.launchAsync();                       // non-modal: the DAW keeps working
}

void NagDialog::paint (juce::Graphics& g)
{
    g.fillAll (findColour (juce::ResizableWindow::backgroundColourId));
}

void NagDialog::resized()
{
    auto area = getLocalBounds().reduced (16);
    messageLabel.setBounds (area.removeFromTop (100));

    auto keyRow = area.removeFromTop (28);
    keyEditor.setBounds (keyRow.removeFromLeft (keyRow.getWidth() - 110));
    submitButton.setBounds (keyRow.withTrimmedLeft (6));

    statusLabel.setBounds (area.removeFromTop (24));

    auto buttons = area.removeFromBottom (30);
    registerButton.setBounds (buttons.removeFromLeft (110));
    closeButton.setBounds (buttons.removeFromRight (110));
}

void NagDialog::showRegistrationField()
{
    keyEditor.setVisible (true);
    submitButton.setVisible (true);
    statusLabel.setVisible (true);
    statusLabel.setText ("Paste your license key and click Submit.", juce::dontSendNotification);
    keyEditor.grabKeyboardFocus();
}

void NagDialog::tryRegister()
{
    if (LicenseValidator::storeKeyIfValid (keyEditor.getText()))
    {
        statusLabel.setText ("Key accepted — thank you for registering!", juce::dontSendNotification);
        juce::Timer::callAfterDelay (1200, [safe = juce::Component::SafePointer<NagDialog> (this)]
        {
            if (safe != nullptr)
                safe->closeWindow();
        });
    }
    else
    {
        statusLabel.setText ("That key doesn't look valid — please check it.", juce::dontSendNotification);
    }
}

void NagDialog::closeWindow()
{
    if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
        dialog->exitModalState (0);
}

} // namespace license

#endif // ENABLE_DONATION_NAG
