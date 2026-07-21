#pragma once

/*  ============================================================================
    LicenseConfig.h — donationware licensing scaffold configuration.

    HOW TO ENABLE / DISABLE
    -----------------------
    The entire donation-nag system is controlled by ONE flag:

        ENABLE_DONATION_NAG == 0   (default)  → No nag popup is ever shown and
                                                the NagDialog code is not even
                                                compiled into the binary. The
                                                plugin is fully functional.

        ENABLE_DONATION_NAG == 1              → On editor open, if no valid
                                                license key is stored, a
                                                non-modal popup politely asks
                                                the user to consider
                                                registering. "Close" dismisses
                                                it with zero functional
                                                consequences; "Register" lets
                                                the user paste a key, which is
                                                then stored in a JUCE
                                                PropertiesFile so the popup
                                                never appears again.

    Change the value below and recompile. Nothing else needs to change.

    REGISTRATION NEVER GATES FEATURES. The plugin ships fully functional
    regardless of registration status — the popup is the entire mechanism.

    KEY FORMAT & GENERATION
    -----------------------
    Keys look like  XXXX-XXXX-XXXX-XXXX  (16 chars from A–Z / 2–9, grouped in
    fours). The first 12 characters are derived from a salted hash of the
    registrant's name/email; the last 4 are a salted checksum of the first 12.
    Validation only re-computes the checksum — deliberately simple, NOT
    cryptographically secure, just enough to reject random strings.

    To generate a key for someone, call (from any debug build, a unit test,
    or a tiny throwaway console app that links LicenseValidator.cpp):

        juce::String key = license::LicenseValidator::generateKeyForString ("customer@email.com");

    Example throwaway generator (uncomment into a main.cpp of a console app):

        // #include "LicenseValidator.h"
        // #include <iostream>
        // int main (int argc, char** argv)
        // {
        //     if (argc < 2) { std::cout << "usage: keygen <name-or-email>\n"; return 1; }
        //     std::cout << license::LicenseValidator::generateKeyForString (argv[1]) << "\n";
        //     return 0;
        // }

    If you ship builds with the nag enabled, change kKeySalt below to your own
    secret value first — anyone with this source can generate keys for the
    default salt.
    ============================================================================ */

#ifndef ENABLE_DONATION_NAG
 #define ENABLE_DONATION_NAG 0
#endif

namespace license
{
    // Salt mixed into both the key body and the checksum. Change before shipping.
    constexpr const char* kKeySalt = "MagicDrumDeBleed-v1-salt";

    // Where the key is stored (JUCE PropertiesFile in the user's app-data dir).
    constexpr const char* kPropertiesFileName = "MagicDrumDeBleed";
    constexpr const char* kPropertiesFolder   = "MagicDrumDeBleed";
    constexpr const char* kStoredKeyProperty  = "licenseKey";
}
