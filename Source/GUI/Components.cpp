#include "Components.h"
#include "../DSP/Wavetables.h"
#include "../Parameters.h"

//==============================================================================
Knob::Knob (APVTS& state, const juce::String& paramId, const juce::String& caption, juce::Colour accent)
    : param (state.getParameter (paramId)), captionText (caption)
{
    jassert (param != nullptr);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setMouseDragSensitivity (220);
    slider.addMouseListener (this, false);
    addAndMakeVisible (slider);

    const auto& range = param->getNormalisableRange();
    if (range.start < 0.0f && range.end > 0.0f)
        slider.getProperties().set ("bipolar", true);

    attachment = std::make_unique<APVTS::SliderAttachment> (state, paramId, slider);
    slider.setDoubleClickReturnValue (true, (double) param->convertFrom0to1 (param->getDefaultValue()));
    slider.setTooltip (param->getName (64));

    slider.onDragStart = [this] { dragging = true; updateCaption(); };
    slider.onDragEnd = [this] { dragging = false; updateCaption(); };
    slider.onValueChange = [this] { if (hovering || dragging) updateCaption(); };

    label.setJustificationType (juce::Justification::centred);
    label.setFont (Theme::font (11.0f, true));
    label.setInterceptsMouseClicks (false, false);
    label.setMinimumHorizontalScale (0.7f);
    addAndMakeVisible (label);
    updateCaption();
}

void Knob::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromBottom (14));
    slider.setBounds (b);
}

void Knob::mouseEnter (const juce::MouseEvent&) { hovering = true; updateCaption(); }
void Knob::mouseExit (const juce::MouseEvent&)  { hovering = false; updateCaption(); }

void Knob::updateCaption()
{
    const bool showValue = hovering || dragging;
    label.setText (showValue ? param->getCurrentValueAsText() : captionText, juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, showValue ? Theme::text : Theme::textDim);
}

//==============================================================================
ChoiceBox::ChoiceBox (APVTS& state, const juce::String& paramId, const juce::String& caption)
{
    auto* choice = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (paramId));
    jassert (choice != nullptr);

    box.addItemList (choice->choices, 1);
    box.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (box);
    attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramId, box);

    if (caption.isNotEmpty())
    {
        label.setText (caption, juce::dontSendNotification);
        label.setFont (Theme::font (10.5f, true));
        label.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (label);
    }
}

void ChoiceBox::resized()
{
    auto b = getLocalBounds();
    if (label.isVisible())
        label.setBounds (b.removeFromTop (14));
    box.setBounds (b);
}

//==============================================================================
ParamToggle::ParamToggle (APVTS& state, const juce::String& paramId, const juce::String& buttonText, juce::Colour accent)
    : juce::ToggleButton (buttonText)
{
    setColour (juce::ToggleButton::tickColourId, accent);
    if (auto* p = state.getParameter (paramId))
        setTooltip (p->getName (64));
    attachment = std::make_unique<APVTS::ButtonAttachment> (state, paramId, *this);
}

//==============================================================================
Section::Section (const juce::String& t, juce::Colour a, bool power)
    : title (t), accent (a), hasPower (power)
{
    setInterceptsMouseClicks (false, false);
}

void Section::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (Theme::outline);
    g.drawRoundedRectangle (b, 6.0f, 1.0f);

    auto strip = getLocalBounds().removeFromTop (titleHeight).toFloat().reduced (1.0f);
    g.setColour (Theme::panelLight);
    g.fillRoundedRectangle (strip.withHeight (strip.getHeight() + 6.0f), 6.0f);
    g.setColour (Theme::panel);
    g.fillRect (strip.withTop (strip.getBottom()).withHeight (6.0f));

    g.setColour (accent);
    g.fillRoundedRectangle (juce::Rectangle<float> (3.0f, 12.0f).withCentre ({ 9.0f, (float) titleHeight * 0.5f }), 1.5f);

    g.setColour (Theme::text);
    g.setFont (Theme::font (12.0f, true));
    g.drawText (title, (hasPower ? 38 : 16), 0, getWidth() - 40, titleHeight, juce::Justification::centredLeft);
}

juce::Rectangle<int> Section::getContentBounds() const
{
    return getBounds().withTrimmedTop (titleHeight + 4).reduced (8, 4);
}

juce::Rectangle<int> Section::getPowerButtonBounds() const
{
    return { getX() + 16, getY() + 3, 18, 18 };
}

juce::Rectangle<int> Section::getTitleRightArea (int width) const
{
    return { getRight() - width - 6, getY() + 3, width, titleHeight - 6 };
}

//==============================================================================
ParamWatcherView::ParamWatcherView (APVTS& state, const juce::StringArray& ids)
{
    for (auto& id : ids)
    {
        values.push_back (state.getRawParameterValue (id));
        jassert (values.back() != nullptr);
    }
    lastValues.assign (values.size(), -12345.0f);
    startTimerHz (30);
}

void ParamWatcherView::timerCallback()
{
    bool changed = false;
    for (size_t i = 0; i < values.size(); ++i)
    {
        const float v = values[i]->load();
        if (std::abs (v - lastValues[i]) > 1.0e-6f)
        {
            lastValues[i] = v;
            changed = true;
        }
    }
    if (changed)
        repaint();
}

static void fillDisplayBackground (juce::Graphics& g, juce::Rectangle<float> b)
{
    g.setColour (Theme::background);
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (Theme::outline.withAlpha (0.5f));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine ((int) (b.getY() + b.getHeight() * (float) i / 4.0f), b.getX() + 4.0f, b.getRight() - 4.0f);
}

//==============================================================================
WavetableView::WavetableView (APVTS& state, int osc, juce::Colour a)
    : ParamWatcherView (state, { Params::osc (osc, "table"), Params::osc (osc, "pos"), Params::osc (osc, "warp_mode"),
                                 Params::osc (osc, "warp"), Params::osc (osc, "on") }),
      accent (a)
{
}

static float warpPhaseForDisplay (WarpMode mode, float warp, float ph)
{
    switch (mode)
    {
        case WarpMode::Bend:     { const float a = warp * 8.0f; return ph * (1.0f + a) / (1.0f + a * ph); }
        case WarpMode::Squeeze:  { const float d = 0.5f - 0.48f * warp; return ph < d ? ph * 0.5f / d : 0.5f + (ph - d) * 0.5f / (1.0f - d); }
        case WarpMode::Sync:     { const float x = ph * (1.0f + warp * 7.0f); return x - std::floor (x); }
        case WarpMode::Quantize: { const float steps = std::exp2 (10.0f - warp * 8.0f); return std::floor (ph * steps) / steps; }
        case WarpMode::Off:
        case WarpMode::FM:
        case WarpMode::AM:       break;
    }
    return ph;
}

void WavetableView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    fillDisplayBackground (g, bounds);

    const auto& bank = WavetableBank::get();
    const int table = juce::jlimit (0, bank.getNumTables() - 1, juce::roundToInt (value (0)));
    const float pos = value (1);
    const auto mode = (WarpMode) juce::roundToInt (value (2));
    const float warp = value (3);
    const bool on = value (4) >= 0.5f;
    const int frames = WavetableBank::numFrames;
    const int level = 4;

    const auto area = bounds.reduced (10.0f, 8.0f);
    const float depthX = area.getWidth() * 0.16f, depthY = area.getHeight() * 0.32f;
    const float waveW = area.getWidth() - depthX, waveH = area.getHeight() - depthY;
    const int points = 120;

    auto framePath = [&] (const float* data, const float* dataB, float mix, float offset)
    {
        juce::Path p;
        const float x0 = area.getX() + depthX * (1.0f - offset);
        const float yc = area.getY() + depthY * offset + waveH * 0.5f;
        for (int i = 0; i <= points; ++i)
        {
            const float ph = warpPhaseForDisplay (mode, warp, std::min ((float) i / (float) points, 0.9999f));
            const int idx = (int) (ph * WavetableBank::tableSize);
            const float s = data[idx] + mix * (dataB[idx] - data[idx]);
            const auto pt = juce::Point<float> (x0 + waveW * (float) i / (float) points, yc - s * waveH * 0.42f);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        return p;
    };

    // Back frames
    for (int f = frames - 1; f >= 0; --f)
    {
        const float offset = (float) f / (float) (frames - 1);
        const auto* data = bank.getFrame (table, level, f);
        g.setColour (Theme::textDim.withAlpha (0.12f + 0.1f * (1.0f - offset)));
        g.strokePath (framePath (data, data, 0.0f, 1.0f - offset), juce::PathStrokeType (1.0f));
    }

    // Current frame
    const float framePos = pos * (float) (frames - 1);
    const int f0 = std::min ((int) framePos, frames - 2);
    const auto current = framePath (bank.getFrame (table, level, f0), bank.getFrame (table, level, f0 + 1),
                                    framePos - (float) f0, 1.0f - pos);
    const auto colour = on ? accent : Theme::textDim;
    g.setColour (colour.withAlpha (0.25f));
    g.strokePath (current, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colour);
    g.strokePath (current, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (Theme::textDim);
    g.setFont (Theme::font (10.5f, true));
    g.drawText (juce::String (juce::roundToInt (pos * 100.0f)) + "%", bounds.reduced (8.0f, 4.0f), juce::Justification::topRight);
}

//==============================================================================
EnvelopeView::EnvelopeView (APVTS& state, const juce::String& prefix, juce::Colour a)
    : ParamWatcherView (state, { prefix + "_a", prefix + "_d", prefix + "_s", prefix + "_r" }), accent (a)
{
}

void EnvelopeView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    fillDisplayBackground (g, bounds);

    const auto area = bounds.reduced (8.0f, 8.0f);
    const float a = std::sqrt (value (0)), d = std::sqrt (value (1)), s = value (2), r = std::sqrt (value (3));
    const float hold = 0.9f;
    const float total = std::max (0.001f, a + d + r + hold);
    const float scale = area.getWidth() / std::max (total, 3.0f);

    const float x0 = area.getX(), bottom = area.getBottom(), top = area.getY(), h = area.getHeight();
    const float xa = x0 + a * scale;
    const float xd = xa + d * scale;
    const float xs = xd + hold * scale;
    const float xr = xs + r * scale;
    const float ys = bottom - s * h;

    juce::Path p;
    p.startNewSubPath (x0, bottom);
    p.lineTo (xa, top);
    p.quadraticTo (xa + (xd - xa) * 0.2f, ys, xd, ys);
    p.lineTo (xs, ys);
    p.quadraticTo (xs + (xr - xs) * 0.2f, bottom, xr, bottom);

    auto fill = p;
    fill.lineTo (x0, bottom);
    fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.35f), 0.0f, top, accent.withAlpha (0.02f), 0.0f, bottom, false));
    g.fillPath (fill);
    g.setColour (accent);
    g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    for (auto pt : { juce::Point<float> (xa, top), juce::Point<float> (xd, ys), juce::Point<float> (xs, ys) })
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (pt));
}

//==============================================================================
LfoView::LfoView (APVTS& state, int lfo, juce::Colour a)
    : ParamWatcherView (state, { Params::lfo (lfo, "shape") }), accent (a)
{
}

void LfoView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    fillDisplayBackground (g, bounds);

    const auto area = bounds.reduced (8.0f, 8.0f);
    const auto shape = (LfoShape) juce::roundToInt (value (0));
    juce::Random rng (7);
    float held = 0.0f, prev = 0.0f;
    std::array<float, 9> randoms {};
    for (auto& v : randoms)
        v = rng.nextFloat() * 2.0f - 1.0f;

    juce::Path p;
    const int points = 200;
    for (int i = 0; i <= points; ++i)
    {
        const float t = 2.0f * (float) i / (float) points;     // two cycles
        const float ph = t - std::floor (t);
        const int cycle = std::min (7, (int) (t * 4.0f));
        float v = 0.0f;
        switch (shape)
        {
            case LfoShape::Sine:         v = std::sin (juce::MathConstants<float>::twoPi * ph); break;
            case LfoShape::Triangle:     v = ph < 0.25f ? 4.0f * ph : (ph < 0.75f ? 2.0f - 4.0f * ph : 4.0f * ph - 4.0f); break;
            case LfoShape::SawUp:        v = 2.0f * ph - 1.0f; break;
            case LfoShape::SawDown:      v = 1.0f - 2.0f * ph; break;
            case LfoShape::Square:       v = ph < 0.5f ? 1.0f : -1.0f; break;
            case LfoShape::SampleHold:   held = randoms[(size_t) cycle]; v = held; break;
            case LfoShape::SmoothRandom:
            {
                const float local = t * 4.0f - (float) cycle;
                prev = randoms[(size_t) cycle];
                v = prev + (randoms[(size_t) cycle + 1] - prev) * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * local));
                break;
            }
        }
        const auto pt = juce::Point<float> (area.getX() + area.getWidth() * (float) i / (float) points,
                                            area.getCentreY() - v * area.getHeight() * 0.45f);
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }

    g.setColour (accent.withAlpha (0.25f));
    g.strokePath (p, juce::PathStrokeType (4.0f));
    g.setColour (accent);
    g.strokePath (p, juce::PathStrokeType (1.8f));
}

//==============================================================================
ScopeView::ScopeView (Source s) : source (std::move (s))
{
    samples.resize (1024);
    startTimerHz (30);
}

void ScopeView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (Theme::background);
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (Theme::outline);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

    source (samples.data(), (int) samples.size());

    // Trigger on a rising zero crossing for a stable picture.
    const int shown = 512;
    int start = 0;
    for (int i = 1; i < (int) samples.size() - shown; ++i)
    {
        if (samples[(size_t) i - 1] <= 0.0f && samples[(size_t) i] > 0.0f)
        {
            start = i;
            break;
        }
    }

    const auto area = bounds.reduced (4.0f, 4.0f);
    juce::Path p;
    for (int i = 0; i < shown; ++i)
    {
        const float v = juce::jlimit (-1.0f, 1.0f, samples[(size_t) (start + i)]);
        const auto pt = juce::Point<float> (area.getX() + area.getWidth() * (float) i / (float) (shown - 1),
                                            area.getCentreY() - v * area.getHeight() * 0.48f);
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    g.setColour (Theme::cyan.withAlpha (0.25f));
    g.strokePath (p, juce::PathStrokeType (3.0f));
    g.setColour (Theme::cyan);
    g.strokePath (p, juce::PathStrokeType (1.2f));
}

//==============================================================================
void StarButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (highlighted ? Theme::panelLight.brighter (0.08f) : Theme::panelLight);
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (getToggleState() ? Theme::amber : Theme::outline);
    g.drawRoundedRectangle (b, 4.0f, 1.0f);

    juce::Path star;
    const auto c = b.getCentre();
    const float outer = std::min (b.getWidth(), b.getHeight()) * (down ? 0.3f : 0.34f), inner = outer * 0.45f;
    for (int i = 0; i < 10; ++i)
    {
        const float angle = juce::MathConstants<float>::pi * (float) i / 5.0f;
        const auto p = c.getPointOnCircumference (i % 2 == 0 ? outer : inner, angle);
        if (i == 0) star.startNewSubPath (p); else star.lineTo (p);
    }
    star.closeSubPath();

    if (getToggleState())
    {
        g.setColour (Theme::amber);
        g.fillPath (star);
    }
    else
    {
        g.setColour (isEnabled() ? (highlighted ? Theme::text : Theme::textDim) : Theme::outline);
        g.strokePath (star, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

//==============================================================================
PresetBar::PresetBar (PresetManager& manager) : presets (manager)
{
    for (auto* b : std::initializer_list<juce::Component*> { &prevButton, &nextButton, &nameButton, &saveButton, &starButton })
        addAndMakeVisible (b);

    starButton.setTooltip ("Like this preset (adds it to Favourites)");
    starButton.onClick = [this]
    {
        presets.toggleFavourite (presets.getCurrentIndex());
        lastChange = -1;
        timerCallback();
    };

    prevButton.onClick = [this] { presets.loadPrevious(); };
    nextButton.onClick = [this] { presets.loadNext(); };
    nameButton.onClick = [this] { showMenu(); };
    saveButton.onClick = [this] { showSaveDialog(); };
    prevButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");
    nameButton.setTooltip ("Browse presets");
    saveButton.setTooltip ("Save the current sound as a user preset");

    categoryLabel.setFont (Theme::font (10.5f, true));
    categoryLabel.setJustificationType (juce::Justification::centred);
    categoryLabel.setColour (juce::Label::textColourId, Theme::cyan);
    categoryLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (categoryLabel);

    startTimerHz (10);
    timerCallback();
}

void PresetBar::paint (juce::Graphics&) {}

void PresetBar::resized()
{
    auto b = getLocalBounds();
    categoryLabel.setBounds (b.removeFromTop (14));
    b.removeFromTop (2);
    saveButton.setBounds (b.removeFromRight (56));
    b.removeFromRight (6);
    starButton.setBounds (b.removeFromRight (30));
    b.removeFromRight (6);
    prevButton.setBounds (b.removeFromLeft (30));
    nextButton.setBounds (b.removeFromRight (30));
    nameButton.setBounds (b.reduced (4, 0));
}

void PresetBar::timerCallback()
{
    // Favourites can change in another plugin window, so the star is refreshed every tick.
    const int index = presets.getCurrentIndex();
    starButton.setEnabled (index >= 0);
    starButton.setToggleState (index >= 0 && presets.isFavourite (index), juce::dontSendNotification);

    const int change = presets.getChangeCounter();
    if (change == lastChange)
        return;

    lastChange = change;
    nameButton.setButtonText (presets.getCurrentName());

    const auto& entries = presets.getEntries();
    categoryLabel.setText (juce::isPositiveAndBelow (index, (int) entries.size()) ? entries[(size_t) index].category.toUpperCase()
                                                                                    : juce::String ("INIT / MODIFIED"),
                           juce::dontSendNotification);
}

namespace PresetMenuIds
{
    enum { initItem = 100000, saveItem, deleteItem, folderItem, refreshItem };
}

void PresetBar::showMenu()
{
    using namespace PresetMenuIds;

    const auto& entries = presets.getEntries();
    const int current = presets.getCurrentIndex();

    juce::PopupMenu menu;
    std::map<juce::String, juce::PopupMenu> byCategory;
    juce::StringArray order;

    const auto favourites = presets.getFavouriteIndices();
    const juce::String star (juce::CharPointer_UTF8 ("\xe2\x98\x85 "));

    for (int i = 0; i < (int) entries.size(); ++i)
    {
        const auto& e = entries[(size_t) i];
        if (! order.contains (e.category))
            order.add (e.category);
        const bool liked = std::find (favourites.begin(), favourites.end(), i) != favourites.end();
        byCategory[e.category].addItem (i + 1, (liked ? star : juce::String()) + e.name, true, i == current);
    }

    juce::PopupMenu favouritesMenu;
    for (int i : favourites)
        favouritesMenu.addItem (i + 1, entries[(size_t) i].name, true, i == current);
    if (favourites.empty())
        favouritesMenu.addItem (-1, "Click the star next to a preset to add it here", false, false);
    menu.addSubMenu (star + "Favourites (" + juce::String ((int) favourites.size()) + ")", favouritesMenu);
    menu.addSeparator();

    for (auto& category : order)
        menu.addSubMenu (category, byCategory[category], true, nullptr,
                         juce::isPositiveAndBelow (current, (int) entries.size()) && entries[(size_t) current].category == category);

    menu.addSeparator();
    menu.addItem (initItem, "Init Patch");
    menu.addItem (saveItem, "Save Preset...");
    const bool isUser = juce::isPositiveAndBelow (current, (int) entries.size()) && entries[(size_t) current].factoryIndex < 0;
    menu.addItem (deleteItem, "Delete User Preset", isUser);
    menu.addItem (folderItem, "Show User Preset Folder");
    menu.addItem (refreshItem, "Rescan User Presets");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameButton).withMinimumWidth (nameButton.getWidth()),
                        [safe = juce::Component::SafePointer<PresetBar> (this), current] (int result)
                        {
                            if (safe == nullptr || result == 0)
                                return;

                            auto& pm = safe->presets;
                            if (result == initItem)         pm.loadInit();
                            else if (result == saveItem)    safe->showSaveDialog();
                            else if (result == deleteItem)  pm.deleteUserPreset (current);
                            else if (result == refreshItem) pm.refresh();
                            else if (result == folderItem)
                            {
                                auto folder = PresetManager::getUserPresetFolder();
                                folder.createDirectory();
                                folder.startAsProcess();
                            }
                            else
                                pm.load (result - 1);
                        });
}

void PresetBar::showSaveDialog()
{
    auto* window = new juce::AlertWindow ("Save Preset", "Name your sound:", juce::MessageBoxIconType::NoIcon, this);
    window->setLookAndFeel (&getLookAndFeel());
    window->addTextEditor ("name", presets.getCurrentName());
    window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true, juce::ModalCallbackFunction::create (
        [safe = juce::Component::SafePointer<PresetBar> (this), window] (int result)
        {
            if (safe != nullptr && result == 1)
                safe->presets.saveUserPreset (window->getTextEditorContents ("name"));
        }), true);
}
