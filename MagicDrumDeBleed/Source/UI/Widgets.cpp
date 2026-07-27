#include "Widgets.h"

namespace ui
{
const theme::Palette* pal = &theme::dark();
float scale = 1.0f;

juce::String fmtHz (double v)
{
    return v >= 1000.0 ? juce::String (v / 1000.0, v >= 10000.0 ? 1 : 2) + " kHz"
                       : juce::String (juce::roundToInt (v)) + " Hz";
}

void drawLogo (juce::Graphics& g)
{
    // Coordinates live in the 46-unit-tall toolbar mockup space, × ui::scale.
    // Snare construction mirrors the Simple-view preset icon (shell, hoops, lugs).
    const juce::Rectangle<float> shell (scf (12.0f), scf (17.5f), scf (26.0f), scf (13.0f));
    g.setColour (pal->accent);
    g.drawRect (shell, scf (1.6f));
    const float ov = scf (1.43f), hoopH = scf (2.4f);
    for (float yy : { shell.getY(), shell.getBottom() })
        g.fillRoundedRectangle (shell.getX() - ov, yy - hoopH * 0.5f,
                                shell.getWidth() + ov * 2.0f, hoopH, hoopH * 0.5f);
    g.setColour (pal->dim);
    const float m = scf (2.86f), lugW = scf (1.44f);
    for (int i = 0; i < 4; ++i)
    {
        const float lx = shell.getX() + shell.getWidth() * (0.16f + 0.226f * (float) i);
        g.fillRect (lx - lugW * 0.5f, shell.getY() + m, lugW, shell.getHeight() - m * 2.0f);
    }

    {   // drumstick striking down from the right, acorn bead just off the rim
        const juce::Point<float> butt (scf (42.5f), scf (5.5f)), tip (scf (27.5f), scf (13.5f));
        const float ang = std::atan2 (tip.y - butt.y, tip.x - butt.x);
        const juce::Point<float> perp (-std::sin (ang), std::cos (ang));
        const float wb = scf (1.5f), wt = scf (0.75f);
        juce::Path stick;
        stick.startNewSubPath (butt + perp * wb);
        stick.lineTo (tip + perp * wt);
        stick.lineTo (tip - perp * wt);
        stick.lineTo (butt - perp * wb);
        stick.closeSubPath();
        stick.addEllipse (butt.x - wb, butt.y - wb, wb * 2.0f, wb * 2.0f);
        juce::Path bead;
        bead.addEllipse (-scf (2.3f), -scf (1.35f), scf (4.6f), scf (2.7f));
        bead.applyTransform (juce::AffineTransform::rotation (ang)
                                 .translated (tip.x + std::cos (ang) * scf (1.9f),
                                              tip.y + std::sin (ang) * scf (1.9f)));
        stick.addPath (bead);
        g.setColour (pal->txt);
        g.fillPath (stick);
    }

    auto sparkle = [&g] (float cx, float cy, float sz, float alpha)
    {
        const float s = scf (sz), k = s * 0.2f, x = scf (cx), y = scf (cy);
        juce::Path p;
        p.startNewSubPath (x, y - s);
        p.quadraticTo (x + k, y - k, x + s, y);
        p.quadraticTo (x + k, y + k, x, y + s);
        p.quadraticTo (x - k, y + k, x - s, y);
        p.quadraticTo (x - k, y - k, x, y - s);
        p.closeSubPath();
        g.setColour (pal->gold.withAlpha (alpha));
        g.fillPath (p);
    };
    sparkle (19.0f, 10.5f, 3.3f, 1.0f);
    sparkle (24.0f,  6.2f, 1.8f, 0.8f);

    const float tx = scf (50.0f);
    const juce::Font f = font (16.0f, true).withExtraKerningFactor (0.06f);
    juce::AttributedString as;
    as.setJustification (juce::Justification::centredLeft);
    as.setWordWrap (juce::AttributedString::none);
    as.append ("MAGIC ", f, pal->accent);
    as.append ("DRUM GATE", f, pal->txt);
    juce::TextLayout tl;
    tl.createLayout (as, scf (400.0f));
    tl.draw (g, { tx, 0.0f, tl.getWidth() + scf (4.0f), scf (46.0f) });

    // underline: gate-open green crossfading to tail gold, fading out at the end
    const float uy = scf (35.5f), ut = scf (2.2f), uw = tl.getWidth();
    juce::ColourGradient grad (pal->open, tx, uy, pal->tail.withAlpha (0.0f), tx + uw, uy, false);
    grad.addColour (0.55, pal->tail);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (tx, uy - ut * 0.5f, uw, ut, ut * 0.5f);
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
Knob::Knob (juce::String n, ColourId col) : name (std::move (n)), clr (col)
{
    // paint() highlights the name while hovered/dragged — without this the
    // highlight only updates when something ELSE repaints the knob (the
    // compressor stage's 90 Hz history timer masked the omission there;
    // everywhere else the last mid-drag paint stayed stuck highlighted).
    setRepaintsOnMouseActivity (true);
}

/*  While the value editor is open, any click landing outside it commits and
    closes it — needed because most controls never take keyboard focus, so
    the editor would otherwise stay open until Enter. */
struct Knob::ClickAway : public juce::MouseListener
{
    explicit ClickAway (Knob& k) : knob (k) {}
    void mouseDown (const juce::MouseEvent& e) override
    {
        auto* ed = knob.edit.get();
        if (ed != nullptr && ed->isVisible()
            && e.eventComponent != ed && ! ed->isParentOf (e.eventComponent)
            && ed->onFocusLost)
            ed->onFocusLost();
    }
    Knob& knob;
};

Knob::~Knob()
{
    if (clickAway != nullptr)
        juce::Desktop::getInstance().removeGlobalMouseListener (clickAway.get());
}

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
    // The editor reveals the exact value; the knob display rounds it.
    const float real = param->convertFrom0to1 (param->getValue());
    edit->setText (juce::String (real, 4), juce::dontSendNotification);
    edit->setVisible (true);
    edit->grabKeyboardFocus();
    edit->selectAll();
    if (clickAway == nullptr)
    {
        clickAway = std::make_unique<ClickAway> (*this);
        juce::Desktop::getInstance().addGlobalMouseListener (clickAway.get());
    }
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
    att->setValueAsCompleteGesture (r.snapToLegalValue ((float) v));
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (param == nullptr) return;
    if (edit != nullptr && edit->isVisible() && edit->onFocusLost)
        edit->onFocusLost();                        // commit before this click acts
    if (valueBounds().contains (e.getPosition()))   // click the value → type it
        return;
    draggingKnob = true;
    dragNorm = param->getValue();
    lastDragY = e.getScreenY();
    att->beginGesture();
}

void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (param == nullptr || ! draggingKnob) return;
    const int dy = lastDragY - e.getScreenY();
    if (dy == 0) return;
    lastDragY = e.getScreenY();

    // Velocity-sensitive: slow mouse movement gets finer resolution.
    const float speed = std::abs (dy) <= 2 ? 0.5f : std::abs (dy) <= 6 ? 0.85f : 1.0f;
    const float sens = e.mods.isShiftDown() ? 740.0f : 136.0f;
    float delta = (float) dy * speed / sens;
    if (reversed) delta = -delta;
    dragNorm = juce::jlimit (0.0f, 1.0f, dragNorm + delta);
    const auto& r = param->getNormalisableRange();
    att->setValueAsPartOfGesture (r.snapToLegalValue (r.convertFrom0to1 (dragNorm)));
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
SnowButton::SnowButton() : juce::Button ("Freeze")
{
    setClickingTogglesState (true);
}

void SnowButton::paintButton (juce::Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (pal->panel2.withAlpha (getToggleState() || over ? 0.95f : 0.55f));
    g.fillRoundedRectangle (b, 4.0f);
    if (getToggleState())
    {
        g.setColour (pal->accent.withAlpha (0.9f));
        g.drawRoundedRectangle (b, 4.0f, 1.0f);
    }
    g.setColour (getToggleState() ? pal->accent : over ? pal->txt : pal->dim);
    const auto c = b.getCentre();
    const float R = b.getWidth() * 0.30f, lw = juce::jmax (1.0f, scf (1.1f));
    for (int i = 0; i < 6; ++i)
    {
        const float a = juce::MathConstants<float>::pi / 3.0f * (float) i;
        const juce::Point<float> dir (std::cos (a), std::sin (a));
        g.drawLine ({ c, c + dir * R }, lw);
        const auto mid = c + dir * (R * 0.55f);          // side branches
        for (const float ba : { a + 0.9f, a - 0.9f })
            g.drawLine (mid.x, mid.y,
                        mid.x + std::cos (ba) * R * 0.32f, mid.y + std::sin (ba) * R * 0.32f, lw * 0.9f);
    }
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
    if (v >= pk) { pk = v; pkT = 1.6f; }
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
    const float shownVal = pk > -59.5f ? pk : shown;   // the peak line's value
    juce::String valTxt = shownVal <= -59.5f ? juce::String::fromUTF8 ("-\xe2\x88\x9e") : juce::String (shownVal, 1);
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
GrMeter::GrMeter (std::function<float()> g) : getDb (std::move (g))
{
    startTimerHz (30);
}

void GrMeter::timerCallback()
{
    const float v = getDb();
    shown = v < shown ? v : shown + 0.25f * (v - shown);   // instant down, eased back
    repaint();
}

void GrMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setFont (font (9.0f, true));
    g.setColour (pal->faint);
    auto cr = r.removeFromTop (scf (13.0f));
    g.drawSingleLineText ("GR", (int) cr.getCentreX(), (int) cr.getBottom() - sc (3),
                          juce::Justification::horizontallyCentred);

    auto vr = r.removeFromBottom (scf (13.0f));
    g.setFont (font (9.5f));
    const bool active = shown < -0.5f;
    g.setColour (active ? pal->open : pal->faint);
    g.drawText (active ? juce::String (juce::roundToInt (shown)) : juce::String::fromUTF8 ("\xe2\x80\x94"),
                vr, juce::Justification::centred);

    auto bar = r;
    g.setColour (pal->panel2); g.fillRoundedRectangle (bar, 3.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (bar, 3.0f, 1.0f);
    for (int t = 1; t < 4; ++t)                             // ticks every 12 dB
        g.drawHorizontalLine ((int) (bar.getY() + bar.getHeight() * (float) t / 4.0f),
                              bar.getX() + 1, bar.getRight() - 1);

    const float h = juce::jlimit (0.0f, 1.0f, -shown / 48.0f) * (bar.getHeight() - 2.0f);
    if (h > 1.0f)
    {
        g.setGradientFill (juce::ColourGradient (pal->open, 0, bar.getY(),
                                                 pal->open.darker (0.4f), 0, bar.getBottom(), false));
        g.fillRect (juce::Rectangle<float> (bar.getX() + 1, bar.getY() + 1, bar.getWidth() - 2, h));
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
static juce::Colour selColour (Knob::ColourId c)
{
    return c == Knob::tailClr ? pal->tail : c == Knob::openClr ? pal->open : pal->accent;
}

static void drawSelectorIcon (juce::Graphics& g, LightSelector::Icon ic,
                              juce::Rectangle<float> b, juce::Colour col)
{
    const float w = b.getWidth(), h = b.getHeight(), x = b.getX(), y = b.getY();
    juce::Path p;
    switch (ic)
    {
        case LightSelector::iconHP:
            p.startNewSubPath (x, y + h);
            p.quadraticTo (x + w * 0.30f, y + h, x + w * 0.45f, y);
            p.lineTo (x + w, y);
            break;
        case LightSelector::iconLP:
            p.startNewSubPath (x, y);
            p.lineTo (x + w * 0.55f, y);
            p.quadraticTo (x + w * 0.70f, y + h, x + w, y + h);
            break;
        case LightSelector::iconBP:           // same smooth bell as iconBell
        case LightSelector::iconBell:
            p.startNewSubPath (x, y + h);
            p.cubicTo (x + w * 0.32f, y + h, x + w * 0.28f, y, x + w * 0.5f, y);
            p.cubicTo (x + w * 0.72f, y, x + w * 0.68f, y + h, x + w, y + h);
            break;
        case LightSelector::iconPropQ:
            p.startNewSubPath (x, y + h);
            p.lineTo (x + w * 0.36f, y + h);
            p.cubicTo (x + w * 0.46f, y + h, x + w * 0.44f, y, x + w * 0.5f, y);
            p.cubicTo (x + w * 0.56f, y, x + w * 0.54f, y + h, x + w * 0.64f, y + h);
            p.lineTo (x + w, y + h);
            break;
        case LightSelector::iconShelf:
            p.startNewSubPath (x, y + h);
            p.lineTo (x + w * 0.28f, y);
            p.lineTo (x + w * 0.72f, y);
            p.lineTo (x + w, y + h);
            break;
        case LightSelector::iconNone:
        default: return;
    }
    g.setColour (col);
    g.strokePath (p, juce::PathStrokeType (1.4f * scale, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
}

LightSelector::LightSelector (std::vector<Option> o, Knob::ColourId c, bool horiz)
    : opts (std::move (o)), clr (c), horizontal (horiz) {}

juce::Rectangle<float> LightSelector::cell (int i) const
{
    auto r = getLocalBounds().toFloat();
    const float n = (float) juce::jmax (1, (int) opts.size());
    return horizontal ? r.withWidth (r.getWidth() / n).translated (r.getWidth() / n * (float) i, 0)
                      : r.withHeight (r.getHeight() / n).translated (0, r.getHeight() / n * (float) i);
}

void LightSelector::setSelected (int i, bool notify)
{
    i = juce::jlimit (0, (int) opts.size() - 1, i);
    if (sel != i)
    {
        sel = i;
        repaint();
    }
    if (notify && onChange) onChange (sel);
}

void LightSelector::paint (juce::Graphics& g)
{
    const auto on = selColour (clr);
    for (int i = 0; i < (int) opts.size(); ++i)
    {
        auto r = cell (i).reduced (scf (1.0f));
        const bool isSel = i == sel;
        const float led = scf (7.0f);
        auto lr = juce::Rectangle<float> (led, led).withCentre ({ r.getX() + led * 0.5f + scf (2.0f), r.getCentreY() });
        if (isSel)
        {
            g.setColour (on.withAlpha (0.35f));
            g.fillEllipse (lr.expanded (scf (2.5f)));
            g.setColour (on);
            g.fillEllipse (lr);
        }
        else
        {
            g.setColour (pal->panel2); g.fillEllipse (lr);
            g.setColour (pal->knobEdge); g.drawEllipse (lr, 1.0f);
        }
        float tx = lr.getRight() + scf (5.0f);
        if (opts[(size_t) i].icon != iconNone)
        {
            auto ib = juce::Rectangle<float> (tx, r.getCentreY() - scf (4.5f), scf (17.0f), scf (9.0f));
            drawSelectorIcon (g, opts[(size_t) i].icon, ib, isSel ? on : pal->dim);
            tx = ib.getRight() + scf (5.0f);
        }
        g.setColour (isSel ? pal->txt : pal->dim);
        g.setFont (font (10.5f));
        g.drawText (opts[(size_t) i].label, (int) tx, (int) r.getY(),
                    (int) (r.getRight() - tx), (int) r.getHeight(), juce::Justification::centredLeft);
    }
}

void LightSelector::mouseUp (const juce::MouseEvent& e)
{
    for (int i = 0; i < (int) opts.size(); ++i)
        if (cell (i).contains (e.position))
        {
            setSelected (i, true);
            return;
        }
}

//==============================================================================
LightToggle::LightToggle (juce::String l, Knob::ColourId c) : label (std::move (l)), clr (c)
{
    setRepaintsOnMouseActivity (true);
}

void LightToggle::setState (bool on)
{
    if (state != on)
    {
        state = on;
        repaint();
    }
}

void LightToggle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const bool over = isMouseOverOrDragging();

    const auto on = ledOverride.isTransparent() ? selColour (clr) : ledOverride;
    const float led = scf (7.0f);
    auto lr = juce::Rectangle<float> (led, led).withCentre ({ r.getX() + scf (6.0f), r.getCentreY() });
    if (state)
    {
        g.setColour (on.withAlpha (0.35f));
        g.fillEllipse (lr.expanded (scf (2.5f)));
        g.setColour (on);
        g.fillEllipse (lr);
    }
    else
    {
        g.setColour (pal->panel2);   g.fillEllipse (lr);
        g.setColour (pal->knobEdge); g.drawEllipse (lr, 1.0f);
    }
    g.setColour (state || over ? pal->txt : pal->dim);
    g.setFont (font (11.0f));
    g.drawText (label, (int) lr.getRight() + sc (6), 0,
                getWidth() - (int) lr.getRight() - sc (8), getHeight(), juce::Justification::centredLeft);
}

void LightToggle::mouseUp (const juce::MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()) && onClick)
        onClick();
}

//==============================================================================
TextSwitch::TextSwitch (juce::String l, juce::String r) : lLab (std::move (l)), rLab (std::move (r))
{
    setRepaintsOnMouseActivity (true);
}

void TextSwitch::setLeftActive (bool left, bool notify)
{
    if (leftActive != left)
    {
        leftActive = left;
        repaint();
        if (notify && onChange)
            onChange (leftActive);
    }
}

void TextSwitch::paint (juce::Graphics& g)
{
    auto pill = getLocalBounds().toFloat();
    pill = pill.withSizeKeepingCentre (pill.getWidth(), juce::jmin (pill.getHeight(), scf (20.0f)));
    const float rad = pill.getHeight() * 0.5f;
    g.setColour (pal->panel2);
    g.fillRoundedRectangle (pill, rad);
    g.setColour (pal->line);
    g.drawRoundedRectangle (pill, rad, 1.0f);

    auto thumb = pill.reduced (scf (1.5f));
    thumb = leftActive ? thumb.removeFromLeft (thumb.getWidth() * 0.5f)
                       : thumb.removeFromRight (thumb.getWidth() * 0.5f);
    const auto lc = lClr.isTransparent() ? pal->open : lClr;
    const auto rc = rClr.isTransparent() ? pal->accent : rClr;
    g.setColour (leftActive ? lc : rc);
    g.fillRoundedRectangle (thumb, thumb.getHeight() * 0.5f);

    g.setFont (font (9.5f, true));
    const auto lHalf = pill.withTrimmedRight (pill.getWidth() * 0.5f);
    const auto rHalf = pill.withTrimmedLeft (pill.getWidth() * 0.5f);
    g.setColour (leftActive ? pal->bg : pal->dim);
    g.drawText (lLab, lHalf.toNearestInt(), juce::Justification::centred);
    g.setColour (leftActive ? pal->dim : pal->bg);
    g.drawText (rLab, rHalf.toNearestInt(), juce::Justification::centred);
}

void TextSwitch::mouseUp (const juce::MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()))
        setLeftActive (! leftActive, true);
}

//==============================================================================
MiniSwitch::MiniSwitch (juce::String l) : label (std::move (l)) {}

void MiniSwitch::setState (bool on, bool notify)
{
    if (state != on)
    {
        state = on;
        repaint();
    }
    if (notify && onChange) onChange (state);
}

void MiniSwitch::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    auto r = full.reduced (0.5f);
    if (label.isNotEmpty())     // pill on the left, label text beside it
        r = juce::Rectangle<float> (scf (22.0f), scf (12.0f))
                .withCentre ({ full.getX() + scf (11.5f), full.getCentreY() });
    const auto on = onColour.isTransparent() ? pal->accent : onColour;
    g.setColour (state ? on.withAlpha (0.35f) : pal->panel2);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (state ? on : pal->knobEdge);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    const float d = r.getHeight() - scf (3.0f);
    const float cx = state ? r.getRight() - d * 0.5f - scf (1.5f) : r.getX() + d * 0.5f + scf (1.5f);
    g.setColour (state ? on : pal->knob);
    g.fillEllipse (cx - d * 0.5f, r.getCentreY() - d * 0.5f, d, d);
    if (label.isNotEmpty())
    {
        g.setColour (state ? pal->txt : pal->dim);
        g.setFont (font (11.5f));
        g.drawText (label, (int) r.getRight() + sc (6), 0,
                    getWidth() - (int) r.getRight() - sc (6), getHeight(),
                    juce::Justification::centredLeft);
    }
}

void MiniSwitch::mouseUp (const juce::MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()))
        setState (! state, true);
}

//==============================================================================
MiniSlider::MiniSlider (float defaultValue, bool vertical)
    : value (defaultValue), def (defaultValue), vert (vertical)
{
    setMouseCursor (vert ? juce::MouseCursor::UpDownResizeCursor
                         : juce::MouseCursor::LeftRightResizeCursor);
}

void MiniSlider::setFromPos (juce::Point<float> p)
{
    const float inset = scf (5.0f);
    const float len = juce::jmax (1.0f, (float) (vert ? getHeight() : getWidth()) - inset * 2);
    const float along = vert ? (float) getHeight() - inset - p.y : p.x - inset;
    value = juce::jlimit (0.0f, 1.0f, along / len);
    if (onChange) onChange (value);
    repaint();
}

void MiniSlider::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float inset = scf (5.0f);
    auto track = vert
        ? juce::Rectangle<float> (r.getCentreX() - scf (2.0f), r.getY() + inset,
                                  scf (4.0f), r.getHeight() - inset * 2)
        : juce::Rectangle<float> (r.getX() + inset, r.getCentreY() - scf (2.0f),
                                  r.getWidth() - inset * 2, scf (4.0f));
    g.setColour (pal->panel2); g.fillRoundedRectangle (track, 2.0f);
    g.setColour (pal->line);   g.drawRoundedRectangle (track, 2.0f, 1.0f);
    g.setColour (pal->accent.withAlpha (0.8f));
    g.fillRoundedRectangle (vert ? track.withTrimmedTop (track.getHeight() * (1.0f - value))
                                 : track.withWidth (track.getWidth() * value), 2.0f);
    const auto centre = vert
        ? juce::Point<float> (r.getCentreX(), track.getBottom() - track.getHeight() * value)
        : juce::Point<float> (track.getX() + track.getWidth() * value, r.getCentreY());
    auto thumb = (vert ? juce::Rectangle<float> (scf (11.0f), scf (7.0f))
                       : juce::Rectangle<float> (scf (7.0f), scf (11.0f))).withCentre (centre);
    g.setColour (pal->knob);     g.fillRoundedRectangle (thumb, 2.0f);
    g.setColour (pal->knobEdge); g.drawRoundedRectangle (thumb, 2.0f, 1.0f);
}

void MiniSlider::mouseDown (const juce::MouseEvent& e)        { setFromPos (e.position); }
void MiniSlider::mouseDrag (const juce::MouseEvent& e)        { setFromPos (e.position); }
void MiniSlider::mouseDoubleClick (const juce::MouseEvent&)
{
    value = def;
    if (onChange) onChange (value);
    repaint();
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

} // namespace ui
