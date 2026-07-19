#pragma once
/*  Band → parameter-ID mapping. Index 0=LOWS(hpf) 1=HIGHS(lpf) 2..6=K1..K5. */
#include "../PluginProcessor.h"

namespace eqids
{
inline juce::String onId (int b)    { return b == 0 ? ParamIDs::hpfOn   : b == 1 ? ParamIDs::lpfOn   : ParamIDs::notchOn (b - 2); }
inline juce::String freqId (int b)  { return b == 0 ? ParamIDs::hpfFreq : b == 1 ? ParamIDs::lpfFreq : ParamIDs::notchFreq (b - 2); }
inline juce::String qId (int b)     { return b >= 2 ? ParamIDs::notchQ (b - 2)    : juce::String(); }
inline juce::String gainId (int b)  { return b >= 2 ? ParamIDs::notchGain (b - 2) : juce::String(); }
inline juce::String shapeId (int b) { return b == 0 ? ParamIDs::hpfSlope : b == 1 ? ParamIDs::lpfSlope : ParamIDs::notchShape (b - 2); }
}
