#pragma once

/*
    LicenseValidator — key generation/validation and persistent storage for
    the donationware scaffold. See LicenseConfig.h for the full picture.

    This file compiles regardless of ENABLE_DONATION_NAG so the validation
    logic is always available (and testable); only the popup UI is gated.
    No licensing logic exists anywhere in the DSP or UI code — the editor
    just asks shouldShowNag() once at startup.
*/

#include <JuceHeader.h>
#include "LicenseConfig.h"

namespace license
{

class LicenseValidator
{
public:
    // True only when the nag system is compiled in AND no valid stored key exists.
    static bool shouldShowNag();

    // Format + checksum validation of a key string (dashes/spaces/case tolerated).
    static bool isKeyValid (const juce::String& key);

    // Validates and, if valid, persists the key. Returns false for bad keys.
    static bool storeKeyIfValid (const juce::String& key);

    // Returns a valid formatted key (XXXX-XXXX-XXXX-XXXX) for a name/email.
    // Developer utility — see LicenseConfig.h for usage notes.
    static juce::String generateKeyForString (const juce::String& nameOrEmail);

private:
    static juce::String normalise (const juce::String& key);
    static juce::String encode (juce::uint64 hash, int numChars);
    static juce::uint64 saltedHash (const juce::String& text);
    static juce::String checksumFor (const juce::String& body12);
    static std::unique_ptr<juce::PropertiesFile> openProperties();
};

} // namespace license
