#include "Widgets.h"

namespace ui
{
const theme::Palette* pal = &theme::dark();
float scale = 1.0f;

juce::String fmtHz (double v)
{
    return v >= 1000.0 ? juce::String (v / 1000.0, v >= 10000.0 ? 1 : 2) + " kHz"
                       : juce::String (v, 0) + " Hz";
}

void setHint (juce::Component& c, const juce::String& title, const juce::String& text)
{
    c.getProperties().set ("hintTitle", title);
    c.getProperties().set ("hint", text);
}

bool findHint (juce::Component* c, juce::String& title, juce::String& text)
{
    while (c != nullptr)
    {
        if (c->getProperties().contains ("hint"))
        {
            title = c->getProperties()["hintTitle"].toString();
            text  = c->getProperties()["hint"].toString();
            return true;
        }
        c = c->getParentComponent();
    }
    return false;
}

//==============================================================================
Knob::Knob (juce::String n, ColourId col) : name (std::move (n)), clr (col) {}

void Knob::attach (juce::RangedAudioParameter* p)
{
    param = p;
    att.reset();
    if (edit != nullptr) edit->setVisible (false);
    if (param != nullptr)
    {
        att = std::make_unique<juce::ParameterAttachment> (*param, [this] (float) { repaint(); });
        att->sendInitialUpdate();
    }
    setVisible (param != nullptr);
}

float Knob::shownNorm() const
{
    const float n = param != nullptr ? param->getValue() : 0.0f;
    return reversed ? 1.0f - n : n;
}

void Knob::paint (juce::Graphics& g)
{
    if (param == nullptr) return;
    auto r = getLocalBounds().toFloat();
    const float nameH = scf (14.0f), valH = scf (14.0f);

    g.setColour (isMouseOverOrDragging() ? (clr == tailClr ? pal->tail : clr == openClr ? pal->open : pal->accent)
                                         : pal->txt);
    g.setFont (font (11.0f, true));
    g.drawText (name, r.removeFromTop (nameH), juce::Justification::centred);

    // dial + value stacked from the top (compact like the mockup)
    const float d = juce::jmin (r.getWidth(), r.getHeight() - valH, scf (50.0f));
    auto dial = juce::Rectangle<float> (d, d)
                    .withCentre ({ r.getCentreX(), 0.0f })
                    .withY (r.getY() + scf (1.0f));

    g.setColour (pal->dim);
    g.setFont (font (11.0f));
    const float real = param->convertFrom0to1 (param->getValue());
    g.drawText (fmt ? fmt (real) : param->getText (param->getValue(), 24),
                (int) r.getX(), (int) dial.getBottom(), (int) r.getWidth(), (int) valH,
                juce::Justification::centred);
    const float cx = dial.getCentreX(), cy = dial.getCentreY();
    const float rad = d * 0.38f, lw = juce::jmax (2.0f, d * 0.10f);
    const float a0 = juce::MathConstants<float>::pi * 0.75f;
    const float a1 = juce::MathConstants<float>::pi * 2.25f;
    const float a  = a0 + (a1 - a0) * shownNorm();

    auto arc = [&] (float from, float to)
    {
        juce::Path p;
        p.addCentredArc (cx, cy, rad, rad, 0.0f,
                         from + juce::MathConstants<float>::halfPi,
                         to + juce::MathConstants<float>::halfPi, true);
        return p;
    };
    g.setColour (pal->knobEdge);
    g.strokePath (arc (a0, a1), juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (shownNorm() > 0.001f)
    {
        g.setColour (clr == tailClr ? pal->tail : clr == openClr ? pal->open : pal->accent);
        g.strokePath (arc (a0, a), juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const float br = d * 0.25f;
    g.setColour (pal->knob);
    g.fillEllipse (cx - br, cy - br, br * 2, br * 2);
    g.setColour (pal->knobEdge);
    g.drawEllipse (cx - br, cy - br, br * 2, br * 2, 1.0f);
    g.setColour (pal->txt);
    g.drawLine (cx, cy, cx + (br - 1.5f) * std::cos (a), cy + (br - 1.5f) * std::sin (a), scf (2.4f));
}

juce::Rectangle<int> Knob::valueBounds() const
{
    // Same geometry as paint(): name strip, dial, then the value strip.
    auto r = getLocalBounds().toFloat();
    const float nameH = scf (14.0f), valH = scf (14.0f);
    r.removeFromTop (nameH);
    const float d = juce::jmin (r.getWidth(), r.getHeight() - valH, scf (50.0f));
    return juce::Rectangle<float> (r.getX(), r.getY() + scf (1.0f) + d, r.getWidth(), valH).toNearestInt();
}

void Knob::showEditor()
{
    if (edit == nullptr)
    {
        edit = std::make_unique<juce::TextEditor>();
        edit->setJustification (juce::Justification::centred);
        edit->setInputRestrictions (10, "0123456789.-kK");
        addChildComponent (*edit);
        auto commit = [this]
        {
            edit->setVisible (false);
            applyTyped (edit->getText());
        };
        edit->onReturnKey = commit;
        edit->onFocusLost = commit;
        edit->onEscapeKey = [this] { edit->setVisible (false); };
    }
    edit->setFont (font (11.0f));
    auto b = valueBounds();
    edit->setBounds (b.withSizeKeepingCentre (juce::jmax (b.getWidth(), sc (56)), sc (18)));
    const float real = param->convertFrom0to1 (param->getValue());
    edit->setText (juce::String (real, std::abs (real) < 10.0f ? 2 : 1), juce::dontSendNotification);
    edit->setVisible (true);
    edit->grabKeyboardFocus();
    edit->selectAll();
}

void Knob::applyTyped (const juce::String& text)
{
    if (param == nullptr || text.trim().isEmpty()) return;
    auto s = text.trim().toLowerCase();
    double mul = 1.0;
    if (s.endsWith ("k")) { mul = 1000.0; s = s.dropLastCharacters (1); }
    const double v = s.getDoubleValue() * mul;

    if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (param))
    {
        // Choice knobs (e.g. slope): snap to the choice whose leading number is nearest.
        int best = 0; double bd = 1.0e18;
        for (int i = 0; i < ch->choices.size(); ++i)
        {
            const double d = std::abs (ch->choices[i].getDoubleValue() - v);
            if (d < bd) { bd = d; best = i; }
        }
        att->setValueAsCompleteGesture ((float) best);
        return;
    }
    const auto& r = param->getNormalisableRange();
    att->setValueAsCompleteGesture (juce::jlimit (r.start, r.end, (float) v));
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (param == nullptr) return;
    if (valueBounds().contains (e.getPosition()))   // click the value → type it
        return;
    draggingKnob = true;
    dragStartNorm = param->getValue();
    dragStartY = e.getScreenY();
    att->beginGesture();
}

void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (param == nullptr || ! draggingKnob) return;
    const float sens = e.mods.isShiftDown() ? 900.0f : 190.0f;
    float delta = (float) (dragStartY - e.getScreenY()) / sens;
    if (reversed) delta = -delta;
    const float n = juce::jlimit (0.0f, 1.0f, dragStartNorm + delta);
    att->setValueAsPartOfGesture (param->convertFrom0to1 (n));
}

void Knob::mouseUp (const juce::MouseEvent& e)
{
    if (draggingKnob)
    {
        att->endGesture();
        draggingKnob = false;
    }
    else if (param != nullptr && valueBounds().contains (e.getPosition())
             && (edit == nullptr || ! edit->isVisible()))
        showEditor();
}

void Knob::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (param != nullptr && att != nullptr && ! valueBounds().contains (e.getPosition()))
        att->setValueAsCompleteGesture (param->convertFrom0to1 (param->getDefaultValue()));
}

//==============================================================================
LevelMeter::LevelMeter (juce::String cap, std::function<float()> get,
                        juce::RangedAudioParameter* thr, bool sCap, bool sVal)
    : caption (std::move (cap)), getDb (std::move (get)), threshold (thr),
      showCaption (sCap), showValue (sVal)
{
    startTimerHz (30);
}

void LevelMeter::timerCallback()
{
    const float v = juce::jlimit (-90.0f, 6.0f, getDb());
    shown = v > shown ? v : shown + 0.12f * (v - shown);
    if (v >= pk) { pk = v; pkT = 1.0f; }
    else if ((pkT -= 1.0f / 30.0f) <= 0.0f) pk = juce::jmax (v, pk - 0.9f);
    repaint();
}

juce::Rectangle<float> LevelMeter::barArea() const
{
    auto r = getLocalBounds().toFloat();
    if (showCaption) r.removeFromTop (scf (13.0f));
    if (showValue)   r.removeFromBottom (scf (13.0f));
    return r;
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setFont (font (9.0f, true));
    g.setColour (pal->faint);
    if (showCaption)
    {
        auto cr = r.removeFromTop (scf (13.0f));
        g.drawSingleLineText (caption, (int) cr.getCentreX(), (int) cr.getBottom() - sc (3),
                              juce::Justification::horizontallyCentred);
    }
    juce::String valTxt = shown <= -59.5f ? juce::String::fromUTF8 ("-\xe2\x88\x9e") : juce::String (shown, 1);
    if (showValue)
    {
        g.setFont (font (9.5f));
        g.drawText (valTxt, r.removeFromBottom (scf (13.0f)), juce::Justification::centred);
    }
    auto bar = r;
    g.setColour (pal->panel2); g.fillRoundedRectangle (bar, 3.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (bar, 3.0f, 1.0f);

    auto yFor = [&] (float db) { return bar.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * bar.getHeight(); };
    g.setColour (pal->line);
    for (int d = -12; d >= -48; d -= 12)
        g.drawHorizontalLine ((int) yFor ((float) d), bar.getX() + 1, bar.getRight() - 1);

    const float top = yFor (shown);
    if (top < bar.getBottom() - 1)
    {
        g.setGradientFill (juce::ColourGradient (pal->accent, 0, bar.getBottom(),
                                                 pal->accent.brighter (0.5f), 0, bar.getY(), false));
        g.fillRect (juce::Rectangle<float> (bar.getX() + 1, top, bar.getWidth() - 2, bar.getBottom() - top - 1));
    }
    if (pk > -59.5f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillRect (juce::Rectangle<float> (bar.getX() + 1, yFor (pk) - 1, bar.getWidth() - 2, 2.0f));
    }
    if (threshold != nullptr)
    {
        const float ty = yFor (threshold->convertFrom0to1 (threshold->getValue()));
        g.setColour (pal->warn);
        const float dash[2] = { scf (4.0f), scf (3.0f) };
        g.drawDashedLine (juce::Line<float> (bar.getX() - 1, ty, bar.getRight() + 1, ty), dash, 2, 2.0f);
        g.fillRoundedRectangle (bar.getRight() - scf (7.0f), ty - scf (4.0f), scf (8.0f), scf (8.0f), 2.0f);
    }
}

juce::MouseCursor LevelMeter::getMouseCursor()
{
    return threshold != nullptr ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor;
}

void LevelMeter::mouseDown (const juce::MouseEvent& e)
{
    if (threshold == nullptr) return;
    dragging = true;
    threshold->beginChangeGesture();
    mouseDrag (e);
}

void LevelMeter::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    auto bar = barArea();
    const float db = -60.0f + juce::jlimit (0.0f, 1.0f, (bar.getBottom() - e.position.y) / bar.getHeight()) * 60.0f;
    threshold->setValueNotifyingHost (threshold->convertTo0to1 (db));
}

void LevelMeter::mouseUp (const juce::MouseEvent&)
{
    if (dragging) { threshold->endChangeGesture(); dragging = false; }
}

//==============================================================================
GateMeter::GateMeter (Getter g) : get (std::move (g))
{
    startTimerHz (30);
}

void GateMeter::timerCallback()
{
    state = get (open01, tail01);
    repaint();
}

void GateMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setFont (font (9.0f, true));
    g.setColour (pal->faint);
    auto cr = r.removeFromTop (scf (13.0f));
    g.drawSingleLineText ("GATE", (int) cr.getCentreX(), (int) cr.getBottom() - sc (3),
                          juce::Justification::horizontallyCentred);

    auto vr = r.removeFromBottom (scf (13.0f));
    g.setFont (font (9.5f));
    g.setColour (state == 2 ? pal->open : state == 1 ? pal->tail : pal->faint);
    g.drawText (state == 2 ? "OPEN" : state == 1 ? "TAIL" : juce::String::fromUTF8 ("\xe2\x80\x94"),
                vr, juce::Justification::centred);

    auto bar = r;
    g.setColour (pal->panel2); g.fillRoundedRectangle (bar, 3.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (bar, 3.0f, 1.0f);
    for (int t = 1; t < 5; ++t)
        g.drawHorizontalLine ((int) (bar.getY() + bar.getHeight() * (float) t / 5.0f),
                              bar.getX() + 1, bar.getRight() - 1);

    const float v = state == 2 ? open01 : state == 1 ? tail01 : 0.0f;
    const float h = juce::jlimit (0.0f, 1.0f, v) * bar.getHeight();
    if (h > 1.0f)
    {
        const auto c = state == 2 ? pal->open : pal->tail;
        g.setGradientFill (juce::ColourGradient (c.darker (0.4f), 0, bar.getBottom(),
                                                 c, 0, bar.getY(), false));
        g.fillRect (juce::Rectangle<float> (bar.getX() + 1, bar.getBottom() - h, bar.getWidth() - 2, h - 1));
    }
}

//==============================================================================
PercentMeter::PercentMeter (std::function<float()> g01) : get01 (std::move (g01))
{
    startTimerHz (30);
}

void PercentMeter::timerCallback()
{
    const float v = juce::jlimit (0.0f, 1.0f, get01());
    shown = v > shown ? v : shown + 0.15f * (v - shown);
    repaint();
}

void PercentMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setFont (font (9.0f, true));
    g.setColour (pal->faint);
    g.drawText ("TAIL", r.removeFromTop (scf (13.0f)), juce::Justification::centred);
    g.setFont (font (9.5f));
    g.drawText (juce::String ((int) std::round (shown * 100.0f)) + "%",
                r.removeFromBottom (scf (13.0f)), juce::Justification::centred);
    auto bar = r;
    g.setColour (pal->panel2); g.fillRoundedRectangle (bar, 3.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (bar, 3.0f, 1.0f);
    const float h = shown * bar.getHeight();
    if (h > 1.0f)
    {
        g.setGradientFill (juce::ColourGradient (pal->tail.darker (0.4f), 0, bar.getBottom(),
                                                 pal->tail, 0, bar.getY(), false));
        g.fillRect (juce::Rectangle<float> (bar.getX() + 1, bar.getBottom() - h, bar.getWidth() - 2, h - 1));
    }
}

//==============================================================================
AmountFader::AmountFader (juce::RangedAudioParameter* p) : param (p)
{
    att = std::make_unique<juce::ParameterAttachment> (*param, [this] (float) { repaint(); });
    att->sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void AmountFader::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float inset = scf (7.0f);
    auto track = juce::Rectangle<float> (r.getCentreX() - scf (3.5f), r.getY() + inset,
                                         scf (7.0f), r.getHeight() - inset * 2);
    g.setColour (pal->panel2); g.fillRoundedRectangle (track, 4.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (track, 4.0f, 1.0f);

    const float v = param->getValue();
    g.setColour (pal->accent.withAlpha (0.85f));
    const float fh = v * track.getHeight();
    if (fh > 1.0f)
        g.fillRoundedRectangle (track.getX(), track.getBottom() - fh, track.getWidth(), fh, 4.0f);

    g.setFont (font (8.0f));
    for (int t = 0; t <= 100; t += 25)
    {
        const float y = track.getBottom() - (t / 100.0f) * track.getHeight();
        g.setColour (pal->knobEdge);
        g.fillRect (juce::Rectangle<float> (r.getX(), y, scf (7.0f), 1.0f));
        if (t % 50 == 0)
        {
            g.setColour (pal->faint);
            g.drawText (juce::String (t), (int) r.getRight() - sc (18), (int) y - sc (5), sc (18), sc (10),
                        juce::Justification::centredRight);
        }
    }

    // thumb centre = 7px + v·trackHeight from the bottom
    const float cy = r.getBottom() - inset - v * track.getHeight();
    auto thumb = juce::Rectangle<float> (scf (34.0f), scf (17.0f)).withCentre ({ r.getCentreX(), cy });
    g.setGradientFill (juce::ColourGradient (pal->knobEdge.brighter (0.25f), 0, thumb.getY(),
                                             pal->knob, 0, thumb.getBottom(), false));
    g.fillRoundedRectangle (thumb, 3.0f);
    g.setColour (pal->knobEdge); g.drawRoundedRectangle (thumb, 3.0f, 1.0f);
    g.setColour (pal->accent);
    g.fillRect (juce::Rectangle<float> (thumb.getX() + 5, thumb.getCentreY() - 0.5f, thumb.getWidth() - 10, 1.0f));
}

void AmountFader::setFromY (float y)
{
    const float inset = scf (7.0f);
    const float h = (float) getHeight() - inset * 2;
    att->setValueAsPartOfGesture (param->convertFrom0to1 (
        juce::jlimit (0.0f, 1.0f, ((float) getHeight() - inset - y) / juce::jmax (1.0f, h))));
}

void AmountFader::mouseDown (const juce::MouseEvent& e)  { dragging = true; att->beginGesture(); setFromY (e.position.y); }
void AmountFader::mouseDrag (const juce::MouseEvent& e)  { if (dragging) setFromY (e.position.y); }
void AmountFader::mouseUp (const juce::MouseEvent&)      { if (dragging) { att->endGesture(); dragging = false; } }
void AmountFader::mouseDoubleClick (const juce::MouseEvent&)
{
    att->setValueAsCompleteGesture (param->convertFrom0to1 (param->getDefaultValue()));
}

//==============================================================================
void MonitorFader::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setFont (font (8.5f, true));
    g.setColour (pal->faint);
    g.drawText ("MON", r.removeFromTop (scf (12.0f)), juce::Justification::centred);
    auto vr = r.removeFromBottom (scf (12.0f));
    g.setFont (font (8.5f));
    g.drawText ((value > 0.05f ? "+" : "") + juce::String (value, std::abs (value) < 10.0f ? 1 : 0),
                vr, juce::Justification::centred);

    auto track = juce::Rectangle<float> (r.getCentreX() - scf (2.5f), r.getY() + scf (4.0f),
                                         scf (5.0f), r.getHeight() - scf (8.0f));
    g.setColour (pal->panel2); g.fillRoundedRectangle (track, 3.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (track, 3.0f, 1.0f);
    g.setColour (pal->knobEdge);
    g.drawHorizontalLine ((int) track.getCentreY(), r.getX() + scf (2.0f), r.getRight() - scf (2.0f));

    const float pos = track.getCentreY() - (value / 24.0f) * track.getHeight() * 0.5f;
    auto thumb = juce::Rectangle<float> (scf (16.0f), scf (9.0f)).withCentre ({ r.getCentreX(), pos });
    g.setColour (pal->knob);     g.fillRoundedRectangle (thumb, 2.0f);
    g.setColour (pal->knobEdge); g.drawRoundedRectangle (thumb, 2.0f, 1.0f);
}

void MonitorFader::setFromY (float y)
{
    auto r = getLocalBounds().toFloat();
    r.removeFromTop (scf (12.0f));
    r.removeFromBottom (scf (12.0f));
    const float half = juce::jmax (1.0f, (r.getHeight() - scf (8.0f)) * 0.5f);
    value = juce::jlimit (-24.0f, 24.0f, (r.getCentreY() - y) / half * 24.0f);
    if (onChange) onChange (value);
    repaint();
}

void MonitorFader::mouseDown (const juce::MouseEvent& e)        { setFromY (e.position.y); }
void MonitorFader::mouseDrag (const juce::MouseEvent& e)        { setFromY (e.position.y); }
void MonitorFader::mouseDoubleClick (const juce::MouseEvent&)
{
    value = 0.0f;
    if (onChange) onChange (value);
    repaint();
}

//==============================================================================
CheckToggle::CheckToggle (juce::String l, bool tc) : label (std::move (l)), tailColour (tc) {}

void CheckToggle::setState (bool on, bool notify)
{
    if (state == on) return;
    state = on;
    if (notify && onChange) onChange (state);
    repaint();
}

void CheckToggle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto box = juce::Rectangle<float> (scf (14.0f), scf (14.0f)).withCentre ({ r.getX() + scf (8.0f), r.getCentreY() });
    if (state)
    {
        g.setColour (tailColour ? pal->tail : pal->accent);
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour (pal->bg);
        juce::Path tick;
        tick.startNewSubPath (box.getX() + box.getWidth() * 0.25f, box.getCentreY());
        tick.lineTo (box.getCentreX() - box.getWidth() * 0.05f, box.getBottom() - box.getHeight() * 0.28f);
        tick.lineTo (box.getRight() - box.getWidth() * 0.2f, box.getY() + box.getHeight() * 0.25f);
        g.strokePath (tick, juce::PathStrokeType (2.0f * scale));
    }
    else
    {
        g.setColour (pal->panel2); g.fillRoundedRectangle (box, 3.0f);
        g.setColour (pal->knobEdge); g.drawRoundedRectangle (box, 3.0f, 1.0f);
    }
    g.setColour (state ? pal->txt : pal->dim);
    g.setFont (font (11.5f));
    g.drawText (label, (int) box.getRight() + sc (6), 0, getWidth() - (int) box.getRight() - sc (6), getHeight(),
                juce::Justification::centredLeft);
}

void CheckToggle::mouseUp (const juce::MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()))
        setState (! state, true);
}
} // namespace ui
