#pragma once

/*
    NagDialog — the (optional) donationware popup. Compiled ONLY when
    ENABLE_DONATION_NAG == 1 in LicenseConfig.h; with the flag at 0 this
    header exposes nothing and the .cpp is empty.

    Behaviour when enabled:
      - launchIfNeeded() shows a non-modal window on editor open if no valid
        stored key exists (LicenseValidator::shouldShowNag()).
      - "Close" dismisses it — the plugin stays fully functional.
      - "Register" reveals a key field; a valid key is stored via
        LicenseValidator and the window closes for good.
*/

#include <JuceHeader.h>
#include "LicenseConfig.h"

#if ENABLE_DONATION_NAG

namespace license
{

class NagDialog : public juce::Component
{
public:
    NagDialog();

    // Shows the dialog (non-modal, desktop window) if no valid key is stored.
    static void launchIfNeeded();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void showRegistrationField();
    void tryRegister();
    void closeWindow();

    juce::Label      messageLabel;
    juce::Label      statusLabel;
    juce::TextEditor keyEditor;
    juce::TextButton closeButton    { "Close" };
    juce::TextButton registerButton { "Register" };
    juce::TextButton submitButton   { "Submit Key" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NagDialog)
};

} // namespace license

#endif // ENABLE_DONATION_NAG
