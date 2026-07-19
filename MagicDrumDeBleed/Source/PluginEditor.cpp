#include "PluginEditor.h"
#if ENABLE_DONATION_NAG
 #include "Licensing/NagDialog.h"
#endif

using namespace ui;

//==============================================================================
void StyledLookAndFeel::setPalette (const theme::Palette& p)
{
    setColour (juce::ResizableWindow::backgroundColourId, p.bg);
    setColour (juce::Label::textColourId,          p.txt);
    setColour (juce::TextButton::buttonColourId,   p.btn);
    setColour (juce::TextButton::buttonOnColourId, p.btnOn);
    setColour (juce::TextButton::textColourOffId,  p.dim);
    setColour (juce::TextButton::textColourOnId,   p.btnOnText);
    setColour (juce::ComboBox::backgroundColourId, p.btn);
    setColour (juce::ComboBox::textColourId,       p.txt);
    setColour (juce::ComboBox::outlineColourId,    p.line);
    setColour (juce::ComboBox::arrowColourId,      p.dim);
    setColour (juce::PopupMenu::backgroundColourId,            p.panel);
    setColour (juce::PopupMenu::textColourId,                  p.txt);
    setColour (juce::PopupMenu::headerTextColourId,            p.faint);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, p.btnOn);
    setColour (juce::PopupMenu::highlightedTextColourId,       p.btnOnText);
    setColour (juce::TextEditor::backgroundColourId,     p.btn);
    setColour (juce::TextEditor::textColourId,           p.txt);
    setColour (juce::TextEditor::outlineColourId,        p.line);
    setColour (juce::TextEditor::focusedOutlineColourId, p.accent);
    setColour (juce::CaretComponent::caretColourId,      p.txt);
    setColour (juce::AlertWindow::backgroundColourId, p.panel);
    setColour (juce::AlertWindow::textColourId,       p.txt);
}

juce::Font StyledLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    // Small buttons (band selectors, solo) get a smaller face so their text fits.
    return font (buttonHeight <= sc (16) ? 8.5f : buttonHeight <= sc (23) ? 9.5f : 11.5f);
}

void StyledLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                              const juce::Colour& bg, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    auto c = bg;
    if (over || down)
        c = c.getBrightness() > 0.5f ? c.darker (0.06f) : c.brighter (0.12f);
    g.setColour (c);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (pal->line);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

//==============================================================================
MagicDrumDeBleedAudioProcessorEditor::MagicDrumDeBleedAudioProcessorEditor (MagicDrumDeBleedAudioProcessor& proc)
    : AudioProcessorEditor (&proc), processor (proc)
{
    ui::pal = &theme::get (processor.isDarkTheme());
    lookAndFeel.setPalette (*ui::pal);
    setLookAndFeel (&lookAndFeel);

    setConstrainer (&constrainer);
    setResizable (true, true);

    rebuildViews();

    const bool simpleOn = processor.isSimpleView();
    const auto saved = simpleOn ? processor.getSimpleSize() : processor.getAdvancedSize();
    if (simpleOn)
    {
        constrainer.setSizeLimits (SimpleView::kMinW, SimpleView::kMinH, 1000, 1200);
        setSize (saved.x > 0 ? saved.x : SimpleView::kDefaultW,
                 saved.y > 0 ? saved.y : SimpleView::kDefaultH);
    }
    else
    {
        constrainer.setSizeLimits (AdvancedView::kMinW, AdvancedView::kMinH, 3200, 2400);
        setSize (saved.x > 0 ? saved.x : AdvancedView::kDefaultW,
                 saved.y > 0 ? saved.y : AdvancedView::kDefaultH);
    }

   #if ENABLE_DONATION_NAG
    juce::MessageManager::callAsync ([] { license::NagDialog::launchIfNeeded(); });
   #endif
}

MagicDrumDeBleedAudioProcessorEditor::~MagicDrumDeBleedAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void MagicDrumDeBleedAudioProcessorEditor::rebuildViews()
{
    const bool simpleOn = processor.isSimpleView();
    advanced.reset(); simple.reset();

    advanced = std::make_unique<AdvancedView> (processor,
        [this]                                              // theme toggle
        {
            processor.setDarkTheme (! processor.isDarkTheme());
            ui::pal = &theme::get (processor.isDarkTheme());
            lookAndFeel.setPalette (*ui::pal);
            // Deferred: this lambda lives on a button that rebuildViews() destroys.
            juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<MagicDrumDeBleedAudioProcessorEditor> (this)]
            {
                if (safe != nullptr) safe->rebuildViews();
            });
        },
        [this] { setView (true); });
    simple = std::make_unique<SimpleView> (processor, [this] { setView (false); });

    addChildComponent (*advanced);
    addChildComponent (*simple);
    advanced->setVisible (! simpleOn);
    simple->setVisible (simpleOn);
    resized();
    repaint();
}

void MagicDrumDeBleedAudioProcessorEditor::setView (bool simpleOn)
{
    processor.setSimpleView (simpleOn);
    advanced->setVisible (! simpleOn);
    simple->setVisible (simpleOn);

    const auto saved = simpleOn ? processor.getSimpleSize() : processor.getAdvancedSize();
    if (simpleOn)
    {
        constrainer.setSizeLimits (SimpleView::kMinW, SimpleView::kMinH, 1000, 1200);
        setSize (saved.x > 0 ? saved.x : SimpleView::kDefaultW,
                 saved.y > 0 ? saved.y : SimpleView::kDefaultH);
    }
    else
    {
        constrainer.setSizeLimits (AdvancedView::kMinW, AdvancedView::kMinH, 3200, 2400);
        setSize (saved.x > 0 ? saved.x : AdvancedView::kDefaultW,
                 saved.y > 0 ? saved.y : AdvancedView::kDefaultH);
    }
}

void MagicDrumDeBleedAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::pal->bg);
}

void MagicDrumDeBleedAudioProcessorEditor::resized()
{
    const bool simpleOn = processor.isSimpleView();

    // Legibility scale — fonts, knob diameters, meter widths, button heights.
    // Regions stretch independently of this.
    ui::scale = simpleOn
        ? juce::jlimit (1.0f, 2.2f, juce::jmin (getWidth() / (float) SimpleView::kDefaultW,
                                                getHeight() / (float) SimpleView::kDefaultH))
        : juce::jlimit (1.0f, 2.2f, juce::jmin (getWidth() / (float) AdvancedView::kDefaultW,
                                                getHeight() / (float) AdvancedView::kDefaultH));

    if (advanced != nullptr) advanced->setBounds (getLocalBounds());
    if (simple != nullptr)   simple->setBounds (getLocalBounds());

    if (simpleOn) processor.setSimpleSize (getWidth(), getHeight());
    else          processor.setAdvancedSize (getWidth(), getHeight());
}
