#include "EQPanel.h"

namespace eqids
{
    juce::String onId (int band)
    {
        if (band == 0) return ParamIDs::hpfOn;
        if (band == 1) return ParamIDs::lpfOn;
        return ParamIDs::notchOn (band - 2);
    }

    juce::String freqId (int band)
    {
        if (band == 0) return ParamIDs::hpfFreq;
        if (band == 1) return ParamIDs::lpfFreq;
        return ParamIDs::notchFreq (band - 2);
    }

    juce::String qId (int band)      { return band >= 2 ? ParamIDs::notchQ (band - 2)    : juce::String(); }
    juce::String gainId (int band)   { return band >= 2 ? ParamIDs::notchGain (band - 2) : juce::String(); }

    juce::String shapeId (int band)
    {
        if (band == 0) return ParamIDs::hpfSlope;
        if (band == 1) return ParamIDs::lpfSlope;
        return ParamIDs::notchShape (band - 2);
    }
}

//==============================================================================
//  SpectrumDisplay
//==============================================================================
SpectrumDisplay::SpectrumDisplay (MagicDrumDeBleedAudioProcessor& proc)
    : processor (proc)
{
    sampleFifo.resize ((size_t) kFftSize, 0.0f);
    fftData.resize ((size_t) kFftSize * 2, 0.0f);
    smoothedDb.assign ((size_t) kNumBins, -100.0f);
    accumDb.assign ((size_t) kNumBins, -100.0f);

    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void SpectrumDisplay::setAccumulate (bool shouldAccumulate)
{
    accumulate = shouldAccumulate;
    if (! accumulate)
        std::fill (accumDb.begin(), accumDb.end(), -100.0f);
    repaint();
}

void SpectrumDisplay::setFrozen (bool shouldFreeze)
{
    frozen = shouldFreeze;
    if (! frozen)
        fifoIndex = 0;
    repaint();
}

void SpectrumDisplay::timerCallback()
{
    bool updated = false;
    for (;;)
    {
        const int got = processor.readSpectrumSamples (pullBuffer, 512);
        if (got <= 0)
            break;
        if (frozen)
            continue;

        for (int i = 0; i < got; ++i)
        {
            sampleFifo[(size_t) fifoIndex] = pullBuffer[i];
            if (++fifoIndex >= kFftSize)
            {
                fifoIndex = 0;
                runFFT();
                updated = true;
            }
        }
    }
    if (updated)
        repaint();
}

void SpectrumDisplay::runFFT()
{
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::copy (sampleFifo.begin(), sampleFifo.end(), fftData.begin());

    window.multiplyWithWindowingTable (fftData.data(), (size_t) kFftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    const float scale = 4.0f / (float) kFftSize;

    for (int bin = 0; bin < kNumBins; ++bin)
    {
        const float db = juce::Decibels::gainToDecibels (fftData[(size_t) bin] * scale, -100.0f);
        float& s = smoothedDb[(size_t) bin];
        s += ((db > s) ? 0.55f : 0.18f) * (db - s);

        if (accumulate)
        {
            float& a = accumDb[(size_t) bin];
            a = juce::jmax (a - 0.08f, db);
        }
    }
}

void SpectrumDisplay::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (pal->spectrumBackground);
    g.fillRoundedRectangle (area, 4.0f);

    constexpr float topDb = 0.0f, bottomDb = -90.0f;
    auto dbToY = [&] (float db)
    {
        return juce::jmap (juce::jlimit (bottomDb, topDb, db), topDb, bottomDb,
                           area.getY() + 2.0f, area.getBottom() - 2.0f);
    };

    g.setColour (pal->spectrumGrid);
    for (float f : { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
        g.drawVerticalLine ((int) eqmap::freqToX (f, area), area.getY(), area.getBottom());
    for (float db : { -18.0f, -36.0f, -54.0f, -72.0f })
        g.drawHorizontalLine ((int) dbToY (db), area.getX(), area.getRight());

    g.setColour (pal->textDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    for (float f : { 100.0f, 1000.0f, 10000.0f })
        g.drawText (f >= 1000.0f ? juce::String (f / 1000.0f, 0) + "k" : juce::String (f, 0),
                    (int) eqmap::freqToX (f, area) + 3, (int) area.getBottom() - 14, 34, 12,
                    juce::Justification::left, false);

    if (area.getWidth() > 320.0f)
    {
        g.setColour (pal->textDim.withAlpha (0.8f));
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        g.drawText ("cuts here are preserved in the output",
                    area.reduced (6.0f, 4.0f), juce::Justification::topLeft, false);
    }

    const double sr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
    const float binHz = (float) (sr / (double) kFftSize);

    auto levelAtX = [&] (float x, const std::vector<float>& bins)
    {
        const float freq = eqmap::xToFreq (x, area);
        const float binF = juce::jlimit (0.0f, (float) (kNumBins - 2), freq / binHz);
        const int bin = (int) binF;
        const float frac = binF - (float) bin;
        return bins[(size_t) bin] * (1.0f - frac) + bins[(size_t) (bin + 1)] * frac;
    };

    auto buildPath = [&] (const std::vector<float>& bins)
    {
        juce::Path path;
        bool started = false;
        for (float x = area.getX(); x <= area.getRight(); x += 2.0f)
        {
            const float y = dbToY (levelAtX (x, bins));
            if (! started) { path.startNewSubPath (x, y); started = true; }
            else             path.lineTo (x, y);
        }
        return path;
    };

    {
        auto path = buildPath (smoothedDb);
        juce::Path fill (path);
        fill.lineTo (area.getRight(), area.getBottom());
        fill.lineTo (area.getX(), area.getBottom());
        fill.closeSubPath();
        g.setColour (pal->spectrumFill);
        g.fillPath (fill);
        g.setColour (pal->spectrumLine);
        g.strokePath (path, juce::PathStrokeType (1.4f));
    }

    if (accumulate)
    {
        g.setColour (pal->spectrumAccum);
        g.strokePath (buildPath (accumDb), juce::PathStrokeType (1.2f));
    }

    if (frozen)
    {
        g.setColour (pal->spectrumFrozen);
        g.fillRoundedRectangle (area, 4.0f);
        g.setColour (pal->textDim);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText ("FROZEN", area.reduced (6.0f), juce::Justification::topRight, false);
    }

    g.setColour (pal->panelOutline);
    g.drawRoundedRectangle (area, 4.0f, 1.0f);
}

//==============================================================================
//  BandOverlay
//==============================================================================
BandOverlay::BandOverlay (MagicDrumDeBleedAudioProcessor& proc, std::function<void (int)> onBandSelected)
    : processor (proc), bandSelectedCallback (std::move (onBandSelected))
{
    for (int b = 0; b < 7; ++b)
    {
        onParams[b]    = processor.apvts.getParameter (eqids::onId (b));
        freqParams[b]  = processor.apvts.getParameter (eqids::freqId (b));
        shapeParams[b] = processor.apvts.getParameter (eqids::shapeId (b));
        if (b >= 2)
        {
            qParams[b]    = processor.apvts.getParameter (eqids::qId (b));
            gainParams[b] = processor.apvts.getParameter (eqids::gainId (b));
        }
    }
    startTimerHz (24);
}

BandOverlay::BandState BandOverlay::getBand (int band) const
{
    auto value = [] (juce::RangedAudioParameter* p)
    {
        return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f;
    };

    BandState s;
    s.enabled = value (onParams[band]) > 0.5f;
    s.freq    = value (freqParams[band]);
    s.q       = band >= 2 ? value (qParams[band]) : 0.707f;
    s.gainDb  = band >= 2 ? value (gainParams[band]) : 0.0f;
    s.shape   = (int) value (shapeParams[band]);
    return s;
}

juce::Point<float> BandOverlay::handlePosition (int band, const BandState& s, juce::Rectangle<float> area) const
{
    const float x = eqmap::freqToX (s.freq, area);
    if (band < 2)
        return { x, area.getY() + area.getHeight() * 0.15f };

    const float y = juce::jmap (s.gainDb, 0.0f, -48.0f,
                                area.getY() + area.getHeight() * 0.12f,
                                area.getBottom() - area.getHeight() * 0.12f);
    return { x, y };
}

int BandOverlay::findBandAt (juce::Point<float> pos) const
{
    const auto area = getLocalBounds().toFloat().reduced (2.0f);
    int best = -1;
    float bestDist = 16.0f;
    for (int b = 0; b < 7; ++b)
    {
        const auto s = getBand (b);
        if (! s.enabled)
            continue;
        const float dist = handlePosition (b, s, area).getDistanceFrom (pos);
        if (dist < bestDist)
        {
            bestDist = dist;
            best = b;
        }
    }
    return best;
}

void BandOverlay::drawEqCurve (juce::Graphics& g, juce::Rectangle<float> area) const
{
    struct ActiveBand { mdd::BiquadFilter::Coeffs coeffs[2]; int numStages; };
    ActiveBand active[7];
    int numActive = 0;

    const double sr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;

    for (int b = 0; b < 7; ++b)
    {
        const auto s = getBand (b);
        if (! s.enabled)
            continue;

        mdd::BandParams bp;
        bp.enabled = true;
        bp.freqHz  = s.freq;
        bp.q       = s.q;
        bp.gainDb  = s.gainDb;
        bp.shape   = s.shape;
        bp.slope   = s.shape;

        const int kind = b == 0 ? 0 : (b == 1 ? 1 : 2);
        active[numActive].numStages = mdd::EQProcessor::computeCoefficients (bp, kind, sr, active[numActive].coeffs);
        ++numActive;
    }

    if (numActive == 0)
        return;

    const float y0      = area.getY() + area.getHeight() * 0.12f;
    const float pxPerDb = (area.getHeight() * 0.76f) / 48.0f;

    juce::Path path;
    bool started = false;
    for (float x = area.getX(); x <= area.getRight(); x += 2.0f)
    {
        const double freq = (double) eqmap::xToFreq (x, area);
        double mag = 1.0;
        for (int i = 0; i < numActive; ++i)
            for (int stage = 0; stage < active[i].numStages; ++stage)
                mag *= mdd::BiquadFilter::magnitudeAt (active[i].coeffs[stage], freq, sr);

        const float db = (float) (20.0 * std::log10 (juce::jmax (1.0e-6, mag)));
        const float y  = y0 - db * pxPerDb;
        if (! started) { path.startNewSubPath (x, y); started = true; }
        else             path.lineTo (x, y);
    }

    g.setColour (pal->eqCurve.withAlpha (0.75f));
    g.strokePath (path, juce::PathStrokeType (1.8f));
}

void BandOverlay::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced (2.0f);
    static const char* labels[7] = { "H", "L", "1", "2", "3", "4", "5" };

    drawEqCurve (g, area);

    for (int b = 0; b < 7; ++b)
    {
        const auto s = getBand (b);
        if (! s.enabled)
            continue;

        const auto colour = pal->bandColours[b];
        const auto pos = handlePosition (b, s, area);
        const float radius = juce::jlimit (7.0f, 11.0f, area.getWidth() * 0.014f);

        if (b >= 2)
        {
            const float halfBw = std::pow (2.0f, 1.0f / (2.0f * juce::jmax (0.1f, s.q)));
            const float x1 = eqmap::freqToX (s.freq / halfBw, area);
            const float x2 = eqmap::freqToX (s.freq * halfBw, area);
            g.setColour (colour.withAlpha (0.55f));
            g.drawLine (x1, pos.y - radius - 5.0f, x1, pos.y + radius + 5.0f, 1.2f);
            g.drawLine (x2, pos.y - radius - 5.0f, x2, pos.y + radius + 5.0f, 1.2f);
            g.setColour (colour.withAlpha (0.25f));
            g.drawLine (pos.x, pos.y + radius, pos.x, area.getBottom(), 1.0f);
        }

        juce::Rectangle<float> handle (pos.x - radius, pos.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour (colour.withAlpha (0.9f));
        if (b >= 2 && s.shape == 1)
            g.fillRoundedRectangle (handle, 3.0f);
        else if (b >= 2 && s.shape == 2)
        {
            juce::Path tri;
            tri.addTriangle (handle.getX(), handle.getY(), handle.getRight(), handle.getY(), handle.getCentreX(), handle.getBottom());
            g.fillPath (tri);
        }
        else
            g.fillEllipse (handle);

        if (b == selectedBand)
        {
            g.setColour (pal->text);
            g.drawEllipse (handle.expanded (2.5f), 1.6f);
        }

        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.setFont (juce::Font (juce::FontOptions (radius * 1.2f, juce::Font::bold)));
        g.drawText (labels[b], handle, juce::Justification::centred, false);
    }
}

void BandOverlay::mouseDown (const juce::MouseEvent& e)
{
    draggedBand = findBandAt (e.position);
    if (draggedBand < 0)
        return;

    if (bandSelectedCallback)
        bandSelectedCallback (draggedBand);

    freqParams[draggedBand]->beginChangeGesture();
    if (gainParams[draggedBand] != nullptr)
        gainParams[draggedBand]->beginChangeGesture();
}

void BandOverlay::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedBand < 0)
        return;

    const auto area = getLocalBounds().toFloat().reduced (2.0f);

    auto* freqParam = freqParams[draggedBand];
    const auto& freqRange = freqParam->getNormalisableRange();
    const float freq = juce::jlimit (freqRange.start, freqRange.end, eqmap::xToFreq (e.position.x, area));
    freqParam->setValueNotifyingHost (freqParam->convertTo0to1 (freq));

    if (auto* gainParam = gainParams[draggedBand])
    {
        const float gain = juce::jmap (juce::jlimit (area.getY() + area.getHeight() * 0.12f,
                                                     area.getBottom() - area.getHeight() * 0.12f,
                                                     e.position.y),
                                       area.getY() + area.getHeight() * 0.12f,
                                       area.getBottom() - area.getHeight() * 0.12f,
                                       0.0f, -48.0f);
        gainParam->setValueNotifyingHost (gainParam->convertTo0to1 (gain));
    }
}

void BandOverlay::mouseUp (const juce::MouseEvent&)
{
    if (draggedBand < 0)
        return;
    freqParams[draggedBand]->endChangeGesture();
    if (gainParams[draggedBand] != nullptr)
        gainParams[draggedBand]->endChangeGesture();
    draggedBand = -1;
}

void BandOverlay::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const int band = draggedBand >= 0 ? draggedBand : findBandAt (e.position);
    if (band < 2 || qParams[band] == nullptr)
        return;

    auto* qParam = qParams[band];
    const auto& range = qParam->getNormalisableRange();
    const float currentQ = qParam->convertFrom0to1 (qParam->getValue());
    const float newQ = juce::jlimit (range.start, range.end,
                                     currentQ * std::pow (2.0f, wheel.deltaY * 1.5f));
    qParam->beginChangeGesture();
    qParam->setValueNotifyingHost (qParam->convertTo0to1 (newQ));
    qParam->endChangeGesture();
}

//==============================================================================
//  EQPanel
//==============================================================================
EQPanel::EQPanel (MagicDrumDeBleedAudioProcessor& proc)
    : processor (proc),
      spectrum (proc),
      overlay (proc, [this] (int band) { selectBand (band); })
{
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, ParamIDs::eqBypass, bypassButton);

    addAndMakeVisible (spectrum);
    addAndMakeVisible (overlay);
    overlay.toFront (false);

    // Input/Output spectrum tap. Right ("Output") == post-EQ.
    levelsSwitch.setState (processor.isSpectrumPostEq(), false);
    levelsSwitch.setOnChange ([this] (bool outputActive) { processor.setSpectrumPostEq (outputActive); });
    levelsSwitch.setPalette (pal);
    addAndMakeVisible (levelsSwitch);

    accumulateButton.setClickingTogglesState (true);
    accumulateButton.onClick = [this] { spectrum.setAccumulate (accumulateButton.getToggleState()); };
    addAndMakeVisible (accumulateButton);

    freezeButton.setClickingTogglesState (true);
    freezeButton.onClick = [this] { spectrum.setFrozen (freezeButton.getToggleState()); };
    addAndMakeVisible (freezeButton);

    static const char* names[7] = { "HPF", "LPF", "N1", "N2", "N3", "N4", "N5" };
    for (int b = 0; b < 7; ++b)
    {
        bandButtons[b].setButtonText (names[b]);
        bandButtons[b].setClickingTogglesState (false);
        bandButtons[b].onClick = [this, b] { selectBand (b); };
        addAndMakeVisible (bandButtons[b]);

        enableButtons[b].setClickingTogglesState (true);
        addAndMakeVisible (enableButtons[b]);
        enableAttachments[b] = std::make_unique<ButtonAttachment> (processor.apvts, eqids::onId (b),
                                                                   enableButtons[b]);

        soloButtons[b].setButtonText ("S");
        soloButtons[b].setClickingTogglesState (false);
        soloButtons[b].onClick = [this, b]
        {
            const int current = processor.getSoloBand();
            processor.setSoloBand (current == b ? -1 : b);
            updateSoloButtons();
            selectBand (b);
        };
        addAndMakeVisible (soloButtons[b]);

        if (auto* onParam = processor.apvts.getParameter (eqids::onId (b)))
        {
            bandOnAttachments[b] = std::make_unique<juce::ParameterAttachment> (*onParam,
                [this, b] (float v) { bandButtons[b].setToggleState (v > 0.5f, juce::dontSendNotification); });
            bandOnAttachments[b]->sendInitialUpdate();
        }
    }

    addAndMakeVisible (freqKnob);
    addAndMakeVisible (gainKnob);
    addAndMakeVisible (qKnob);

    for (int i = 0; i < 3; ++i)
    {
        shapeButtons[i].setShape (i);
        shapeButtons[i].onClick = [this, i] { setShape (i); };
        addChildComponent (shapeButtons[i]);
    }

    static const char* slopeNames[2] = { "6 dB/oct", "12 dB/oct" };
    for (int i = 0; i < 2; ++i)
    {
        slopeButtons[i].setButtonText (slopeNames[i]);
        slopeButtons[i].setClickingTogglesState (false);
        slopeButtons[i].onClick = [this, i] { setShape (i); };
        addChildComponent (slopeButtons[i]);
    }

    selectBand (0);
    updateSoloButtons();
}

EQPanel::~EQPanel()
{
    processor.setSoloBand (-1);
}

void EQPanel::selectBand (int band)
{
    selectedBand = juce::jlimit (0, 6, band);
    overlay.setSelectedBand (selectedBand);
    rebuildAttachments();
    updateShapeSlopeControls();
    updateBandButtonColours();
    resized();
}

void EQPanel::setShape (int shapeIndex)
{
    if (auto* param = processor.apvts.getParameter (eqids::shapeId (selectedBand)))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) shapeIndex));
        param->endChangeGesture();
    }
}

void EQPanel::rebuildAttachments()
{
    freqAttachment.reset();
    gainAttachment.reset();
    qAttachment.reset();
    shapeAttachment.reset();

    auto& apvts = processor.apvts;
    const bool isNotch = selectedBand >= 2;

    freqAttachment = std::make_unique<SliderAttachment> (apvts, eqids::freqId (selectedBand), freqKnob.slider);

    gainKnob.setVisible (isNotch);
    qKnob.setVisible (isNotch);
    if (isNotch)
    {
        gainAttachment = std::make_unique<SliderAttachment> (apvts, eqids::gainId (selectedBand), gainKnob.slider);
        qAttachment    = std::make_unique<SliderAttachment> (apvts, eqids::qId (selectedBand), qKnob.slider);
    }

    if (auto* shapeParam = apvts.getParameter (eqids::shapeId (selectedBand)))
    {
        shapeAttachment = std::make_unique<juce::ParameterAttachment> (*shapeParam,
            [this] (float) { updateShapeSlopeControls(); });
        shapeAttachment->sendInitialUpdate();
    }
}

void EQPanel::updateShapeSlopeControls()
{
    const bool isNotch = selectedBand >= 2;
    int current = 0;
    if (auto* raw = processor.apvts.getRawParameterValue (eqids::shapeId (selectedBand)))
        current = (int) raw->load();

    for (int i = 0; i < 3; ++i)
    {
        shapeButtons[i].setVisible (isNotch);
        shapeButtons[i].setSelected (isNotch && current == i);
    }
    for (int i = 0; i < 2; ++i)
    {
        slopeButtons[i].setVisible (! isNotch);
        slopeButtons[i].setToggleState (! isNotch && current == i, juce::dontSendNotification);
        slopeButtons[i].setColour (juce::TextButton::buttonColourId,
                                   (! isNotch && current == i) ? pal->buttonOn : pal->buttonOff);
        slopeButtons[i].setColour (juce::TextButton::textColourOffId,
                                   (! isNotch && current == i) ? pal->buttonTextOn : pal->buttonText);
    }
}

void EQPanel::updateSoloButtons()
{
    const int solo = processor.getSoloBand();
    for (int b = 0; b < 7; ++b)
        soloButtons[b].setToggleState (b == solo, juce::dontSendNotification);
}

void EQPanel::updateBandButtonColours()
{
    for (int b = 0; b < 7; ++b)
    {
        bandButtons[b].setColour (juce::TextButton::buttonOnColourId, pal->bandColours[b].withAlpha (0.85f));
        bandButtons[b].setColour (juce::TextButton::buttonColourId,
                                  b == selectedBand ? pal->highlight.withAlpha (0.45f) : pal->buttonOff);
        bandButtons[b].setColour (juce::TextButton::textColourOffId,
                                  b == selectedBand ? pal->buttonTextOn : pal->buttonText);
        bandButtons[b].setColour (juce::TextButton::textColourOnId, pal->buttonTextOn);

        enableButtons[b].setColour (juce::TextButton::buttonOnColourId, pal->bandColours[b]);
        enableButtons[b].setColour (juce::TextButton::buttonColourId, pal->buttonOff);

        soloButtons[b].setColour (juce::TextButton::buttonOnColourId, pal->soloActive);
        soloButtons[b].setColour (juce::TextButton::buttonColourId, pal->buttonOff);
        soloButtons[b].setColour (juce::TextButton::textColourOffId, pal->buttonText);
        soloButtons[b].setColour (juce::TextButton::textColourOnId, juce::Colours::black.withAlpha (0.8f));
    }
    for (auto& s : shapeButtons)
        s.setPalette (pal);
    updateShapeSlopeControls();
    repaint();
}

void EQPanel::setPalette (const theme::Palette& p)
{
    pal = &p;
    spectrum.setPalette (pal);
    overlay.setPalette (pal);
    levelsSwitch.setPalette (pal);
    titleLabel.setColour (juce::Label::textColourId, pal->title);
    updateBandButtonColours();
}

void EQPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (pal->panelBackground);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (pal->panelOutline);
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
}

void EQPanel::resized()
{
    auto r = getLocalBounds().reduced (juce::jmax (4, getHeight() / 50));

    const int titleHeight = juce::jlimit (16, 22, getHeight() / 11);
    auto titleRow = r.removeFromTop (titleHeight);

    freezeButton.setBounds (titleRow.removeFromRight (juce::jlimit (52, 76, getWidth() / 11)).reduced (1));
    titleRow.removeFromRight (3);
    accumulateButton.setBounds (titleRow.removeFromRight (juce::jlimit (72, 96, getWidth() / 8)).reduced (1));
    titleRow.removeFromRight (6);
    levelsSwitch.setBounds (titleRow.removeFromRight (juce::jlimit (108, 150, getWidth() / 6)));
    titleLabel.setFont (juce::Font (juce::FontOptions ((float) titleHeight - 5.0f, juce::Font::bold)));
    titleLabel.setBounds (titleRow);

    const int controlsHeight = juce::jlimit (66, 108, juce::roundToInt ((float) getHeight() * 0.32f));
    auto controls = r.removeFromBottom (controlsHeight);
    r.removeFromBottom (2);

    spectrum.setBounds (r);
    overlay.setBounds (r);

    // ---- Left: band rows + bypass anchored bottom-left ----
    auto bandArea = controls.removeFromLeft (juce::roundToInt ((float) controls.getWidth() * 0.42f));

    const int bypassH = juce::jlimit (18, 24, bandArea.getHeight() / 4);
    auto bypassRow = bandArea.removeFromBottom (bypassH);
    bypassButton.setBounds (bypassRow.removeFromLeft (juce::jlimit (66, 96, getWidth() / 9))
                                .withSizeKeepingCentre (juce::jlimit (66, 96, getWidth() / 9), bypassH - 2));

    const int selectorHeight = juce::jlimit (18, 26, bandArea.getHeight() / 2);
    auto selectorRow = bandArea.removeFromTop (selectorHeight);
    auto toggleRow = bandArea;   // remainder for enable+solo

    const int bw = selectorRow.getWidth() / 7;
    for (int b = 0; b < 7; ++b)
    {
        bandButtons[b].setBounds (selectorRow.getX() + b * bw, selectorRow.getY(), bw - 2, selectorHeight);

        auto cell = juce::Rectangle<int> (toggleRow.getX() + b * bw, toggleRow.getY() + 1,
                                          bw - 2, juce::jmin (toggleRow.getHeight() - 2, 20));
        const int half = cell.getWidth() / 2;
        enableButtons[b].setBounds (cell.removeFromLeft (half).reduced (1, 0));
        soloButtons[b].setBounds (cell.reduced (1, 0));
    }

    // ---- Right: selected-band strip (knobs + shape/slope selector) ----
    controls.removeFromLeft (8);
    auto selectorArea = controls.removeFromRight (juce::jlimit (62, 92, controls.getWidth() / 4));

    const bool isNotch = selectedBand >= 2;
    if (isNotch)
    {
        // three curve-icon buttons in a row
        const int iconW = selectorArea.getWidth() / 3;
        auto iconRow = selectorArea.withSizeKeepingCentre (selectorArea.getWidth(),
                                                           juce::jmin (selectorArea.getHeight(), 34));
        for (int i = 0; i < 3; ++i)
            shapeButtons[i].setBounds (iconRow.getX() + i * iconW, iconRow.getY(), iconW - 2, iconRow.getHeight());
    }
    else
    {
        // two stacked slope buttons
        auto stack = selectorArea.withSizeKeepingCentre (selectorArea.getWidth(),
                                                         juce::jmin (selectorArea.getHeight(), 52));
        const int h = stack.getHeight() / 2;
        slopeButtons[0].setBounds (stack.removeFromTop (h).reduced (1));
        slopeButtons[1].setBounds (stack.reduced (1));
    }

    const int knobW = controls.getWidth() / (isNotch ? 3 : 1);
    freqKnob.setBounds (controls.removeFromLeft (knobW));
    if (isNotch)
    {
        gainKnob.setBounds (controls.removeFromLeft (knobW));
        qKnob.setBounds (controls);
    }
}
