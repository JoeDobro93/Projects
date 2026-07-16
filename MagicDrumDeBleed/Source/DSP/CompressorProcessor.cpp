#include "CompressorProcessor.h"
#include "BiquadFilter.h"   // for mdd::exactlyEqual

namespace mdd
{

void CompressorProcessor::prepare (double sampleRate, int numChannels, int maxLookaheadSamples)
{
    sr = sampleRate;

    delays.resize ((size_t) juce::jmax (1, numChannels));
    for (auto& d : delays)
        d.prepare (maxLookaheadSamples);

    // Worst-case RMS window is 100 ms.
    rmsBuffer.assign ((size_t) juce::jmax (1, (int) std::ceil (0.1 * sr) + 1), 0.0);
    currentRmsWindowMs = -1.0;
    setRmsWindow (10.0);

    reset();
}

void CompressorProcessor::reset()
{
    for (auto& d : delays)
        d.reset();

    std::fill (rmsBuffer.begin(), rmsBuffer.end(), 0.0);
    rmsSum = 0.0;
    rmsIndex = 0;
    rmsRefreshCounter = 0;

    currentGainDb = 0.0;
    holdCounter = 0;
    lastBlockGrDb = 0.0f;
}

void CompressorProcessor::setRmsWindow (double windowMs)
{
    if (exactlyEqual (windowMs, currentRmsWindowMs))
        return;

    currentRmsWindowMs = windowMs;
    const int newLength = juce::jlimit (1, (int) rmsBuffer.size(),
                                        (int) std::lround (windowMs * 0.001 * sr));
    if (newLength != rmsLength)
    {
        rmsLength = newLength;
        // Window length changed: clear and start accumulating fresh. A brief
        // detector dip on a manual parameter tweak is inaudible.
        std::fill (rmsBuffer.begin(), rmsBuffer.end(), 0.0);
        rmsSum = 0.0;
        rmsIndex = 0;
        rmsRefreshCounter = 0;
    }
}

void CompressorProcessor::rebuildRmsSum()
{
    // Periodically recompute the running sum exactly to stop floating-point
    // drift from accumulating over hours of processing.
    double sum = 0.0;
    for (int i = 0; i < rmsLength; ++i)
        sum += rmsBuffer[(size_t) i];
    rmsSum = sum;
}

void CompressorProcessor::setParameters (double newThresholdDb, double newReductionDb,
                                         int newLookaheadSamples, double rmsWindowMs,
                                         double holdMs, double releaseMs)
{
    thresholdDb = newThresholdDb;
    reductionDb = newReductionDb;

    lookaheadSamples = juce::jmax (0, newLookaheadSamples);
    for (auto& d : delays)
        d.setDelay (lookaheadSamples);

    setRmsWindow (rmsWindowMs);

    holdSamples = (int) std::lround (holdMs * 0.001 * sr);

    // Attack: exponential ramp fast enough to hit the reduction target within
    // the lookahead window (5 time constants ≈ full settle).
    const double attackTauSamples = juce::jmax (1.0, (double) lookaheadSamples / 5.0);
    attackCoeff = 1.0 - std::exp (-1.0 / attackTauSamples);

    const double releaseTauSamples = juce::jmax (1.0, releaseMs * 0.001 * sr);
    releaseCoeff = 1.0 - std::exp (-1.0 / releaseTauSamples);
}

void CompressorProcessor::process (juce::AudioBuffer<double>& audio, const double* detector,
                                   int numSamples, bool applyGain)
{
    const int numChannels = juce::jmin (audio.getNumChannels(), (int) delays.size());
    float minGainDb = 0.0f;
    float maxRmsDb = -120.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // ---- RMS detector (un-delayed sidechain) ----
        const double det = detector[i];
        rmsSum += det * det - rmsBuffer[(size_t) rmsIndex];
        rmsBuffer[(size_t) rmsIndex] = det * det;
        if (++rmsIndex >= rmsLength)
            rmsIndex = 0;
        if (++rmsRefreshCounter >= rmsLength)
        {
            rmsRefreshCounter = 0;
            rebuildRmsSum();
        }

        const double meanSquare = juce::jmax (0.0, rmsSum) / (double) rmsLength;
        const double rmsDb = 10.0 * std::log10 (meanSquare + 1.0e-30);
        maxRmsDb = juce::jmax (maxRmsDb, (float) rmsDb);

        // ---- Binary engage/disengage with hold ----
        double targetDb = 0.0;
        if (rmsDb > thresholdDb)
        {
            targetDb = reductionDb;
            holdCounter = holdSamples;      // retrigger hold while above threshold
        }
        else if (holdCounter > 0)
        {
            --holdCounter;
            targetDb = reductionDb;
        }

        // ---- Smooth the envelope (attack towards reduction, release back to 0 dB) ----
        if (applyGain)
        {
            const double coeff = (targetDb < currentGainDb) ? attackCoeff : releaseCoeff;
            currentGainDb += coeff * (targetDb - currentGainDb);
        }
        else
        {
            currentGainDb = 0.0;            // bypassed: keep envelope released
        }

        const double gain = applyGain ? std::pow (10.0, currentGainDb / 20.0) : 1.0;
        minGainDb = juce::jmin (minGainDb, (float) currentGainDb);

        // ---- Delay the audio by the lookahead and apply the gain ----
        for (int ch = 0; ch < numChannels; ++ch)
        {
            double* data = audio.getWritePointer (ch);
            data[i] = delays[(size_t) ch].processSample (data[i]) * gain;
        }
    }

    lastBlockGrDb = minGainDb;
    lastBlockRmsDb = maxRmsDb;
}

} // namespace mdd
