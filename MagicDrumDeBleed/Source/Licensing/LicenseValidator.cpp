#include "LicenseValidator.h"

namespace license
{

// 32-character alphabet without easily-confused glyphs (0/O, 1/I).
static const char* kAlphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

juce::uint64 LicenseValidator::saltedHash (const juce::String& text)
{
    // FNV-1a over the salted, lower-cased input. Not secure — by design.
    const juce::String salted = juce::String (kKeySalt) + "|" + text.toLowerCase().trim();
    juce::uint64 hash = 1469598103934665603ULL;
    for (const char* c = salted.toRawUTF8(); *c != 0; ++c)
    {
        hash ^= (juce::uint64) (juce::uint8) *c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

juce::String LicenseValidator::encode (juce::uint64 hash, int numChars)
{
    juce::String out;
    for (int i = 0; i < numChars; ++i)
    {
        out << kAlphabet[hash & 31];
        hash >>= 5;
        if (hash == 0)
            hash = 0x9E3779B97F4A7C15ULL * (juce::uint64) (i + 1);   // keep bits flowing
    }
    return out;
}

juce::String LicenseValidator::checksumFor (const juce::String& body12)
{
    return encode (saltedHash ("checksum|" + body12), 4);
}

juce::String LicenseValidator::normalise (const juce::String& key)
{
    return key.toUpperCase().retainCharacters (juce::String (kAlphabet));
}

juce::String LicenseValidator::generateKeyForString (const juce::String& nameOrEmail)
{
    const juce::String body = encode (saltedHash ("body|" + nameOrEmail), 12);
    const juce::String full = body + checksumFor (body);

    juce::String formatted;
    for (int i = 0; i < 16; i += 4)
    {
        if (i > 0) formatted << "-";
        formatted << full.substring (i, i + 4);
    }
    return formatted;
}

bool LicenseValidator::isKeyValid (const juce::String& key)
{
    const juce::String clean = normalise (key);
    if (clean.length() != 16)
        return false;

    return checksumFor (clean.substring (0, 12)) == clean.substring (12, 16);
}

std::unique_ptr<juce::PropertiesFile> LicenseValidator::openProperties()
{
    juce::PropertiesFile::Options options;
    options.applicationName     = kPropertiesFileName;
    options.folderName          = kPropertiesFolder;
    options.filenameSuffix      = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    return std::make_unique<juce::PropertiesFile> (options);
}

bool LicenseValidator::storeKeyIfValid (const juce::String& key)
{
    if (! isKeyValid (key))
        return false;

    if (auto props = openProperties())
    {
        props->setValue (kStoredKeyProperty, normalise (key));
        props->saveIfNeeded();
        return true;
    }
    return false;
}

bool LicenseValidator::shouldShowNag()
{
   #if ENABLE_DONATION_NAG
    if (auto props = openProperties())
        return ! isKeyValid (props->getValue (kStoredKeyProperty));
    return true;
   #else
    return false;
   #endif
}

} // namespace license
