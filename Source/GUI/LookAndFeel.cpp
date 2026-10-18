#include "LookAndFeel.h"

AudiosphereLookAndFeel::AudiosphereLookAndFeel()
{
    setColourScheme ({ Theme::panel, Theme::background, Theme::panelLight, Theme::outline, Theme::text,
                       Theme::panelLight, Theme::text, Theme::cyan.withAlpha (0.25f), Theme::text });

    setColour (juce::ResizableWindow::backgroundColourId, Theme::background);
    setColour (juce::Slider::rotarySliderFillColourId, Theme::cyan);
    setColour (juce::Slider::rotarySliderOutlineColourId, Theme::track);
    setColour (juce::Slider::thumbColourId, Theme::text);
    setColour (juce::Label::textColourId, Theme::textDim);
    setColour (juce::ComboBox::backgroundColourId, Theme::panelLight);
    setColour (juce::ComboBox::outlineColourId, Theme::outline);
    setColour (juce::ComboBox::textColourId, Theme::text);
    setColour (juce::ComboBox::arrowColourId, Theme::textDim);
    setColour (juce::PopupMenu::backgroundColourId, Theme::panelLight);
    setColour (juce::PopupMenu::textColourId, Theme::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::cyan.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::PopupMenu::headerTextColourId, Theme::cyan);
    setColour (juce::TextButton::buttonColourId, Theme::panelLight);
    setColour (juce::TextButton::buttonOnColourId, Theme::cyan.withAlpha (0.3f));
    setColour (juce::TextButton::textColourOffId, Theme::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::TooltipWindow::backgroundColourId, Theme::panelLight);
    setColour (juce::TooltipWindow::textColourId, Theme::text);
    setColour (juce::TooltipWindow::outlineColourId, Theme::outline);
    setColour (juce::BubbleComponent::backgroundColourId, Theme::panelLight);
    setColour (juce::BubbleComponent::outlineColourId, Theme::outline);
    setColour (juce::TextEditor::backgroundColourId, Theme::background);
    setColour (juce::TextEditor::textColourId, Theme::text);
    setColour (juce::TextEditor::outlineColourId, Theme::outline);
    setColour (juce::TextEditor::focusedOutlineColourId, Theme::cyan);
    setColour (juce::AlertWindow::backgroundColourId, Theme::panel);
    setColour (juce::AlertWindow::textColourId, Theme::text);
    setColour (juce::AlertWindow::outlineColourId, Theme::outline);
}

void AudiosphereLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                               float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (2.0f);
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float arcRadius = radius - 2.0f;
    const float lineW = std::max (2.0f, radius * 0.14f);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const bool enabled = slider.isEnabled();

    // Track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (Theme::track);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc (from the centre for bipolar parameters)
    const bool bipolar = (bool) slider.getProperties().getWithDefault ("bipolar", false);
    const float valueAngle = startAngle + sliderPos * (endAngle - startAngle);
    const float fromAngle = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

    if (std::abs (valueAngle - fromAngle) > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             std::min (fromAngle, valueAngle), std::max (fromAngle, valueAngle), true);
        g.setColour (enabled ? accent : Theme::textDim);
        g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Body
    const float bodyRadius = arcRadius - lineW * 1.4f;
    juce::ColourGradient grad (Theme::knobBody.brighter (0.25f), centre.x, centre.y - bodyRadius,
                               Theme::knobBody.darker (0.3f), centre.x, centre.y + bodyRadius, false);
    g.setGradientFill (grad);
    g.fillEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre));
    g.setColour (Theme::outline);
    g.drawEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre), 1.0f);

    // Pointer
    const auto tip = centre.getPointOnCircumference (bodyRadius * 0.85f, valueAngle);
    const auto inner = centre.getPointOnCircumference (bodyRadius * 0.3f, valueAngle);
    g.setColour (enabled ? Theme::text : Theme::textDim);
    g.drawLine ({ inner, tip }, std::max (1.5f, lineW * 0.6f));
}

void AudiosphereLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                               float, float, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
        return;
    }

    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto trackArea = bounds.withSizeKeepingCentre (bounds.getWidth(), 6.0f);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    g.setColour (Theme::track);
    g.fillRoundedRectangle (trackArea, 3.0f);

    const bool bipolar = (bool) slider.getProperties().getWithDefault ("bipolar", false);
    const float from = bipolar ? trackArea.getCentreX() : trackArea.getX();
    const auto fill = juce::Rectangle<float>::leftTopRightBottom (std::min (from, sliderPos), trackArea.getY(),
                                                                  std::max (from, sliderPos), trackArea.getBottom());
    g.setColour (accent);
    g.fillRoundedRectangle (fill, 3.0f);

    g.setColour (Theme::text);
    g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ sliderPos, bounds.getCentreY() }));
}

void AudiosphereLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto accent = button.findColour (juce::ToggleButton::tickColourId);
    const bool on = button.getToggleState();

    if (button.getButtonText().isEmpty())
    {
        // Power button: a small round LED-style switch.
        const float d = std::min (bounds.getWidth(), bounds.getHeight());
        const auto circle = juce::Rectangle<float> (d, d).withCentre (bounds.getCentre()).reduced (1.0f);
        g.setColour (on ? accent.withAlpha (0.25f) : Theme::panelLight);
        g.fillEllipse (circle);
        g.setColour (on ? accent : (highlighted ? Theme::text : Theme::textDim));
        g.drawEllipse (circle, 1.2f);

        const auto c = circle.getCentre();
        const float r = circle.getWidth() * 0.26f;
        juce::Path icon;
        icon.addCentredArc (c.x, c.y, r, r, 0.0f, 0.7f, juce::MathConstants<float>::twoPi - 0.7f, true);
        g.strokePath (icon, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (c.x, c.y - r * 1.25f, c.x, c.y - r * 0.2f, 1.4f);
        return;
    }

    // Text chip
    g.setColour (on ? accent.withAlpha (0.22f) : Theme::panelLight);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (on ? accent : (highlighted ? Theme::text : Theme::outline));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    g.setColour (on ? Theme::text : Theme::textDim);
    g.setFont (Theme::font (std::min (11.0f, bounds.getHeight() * 0.6f), true));
    g.drawText (button.getButtonText(), bounds, juce::Justification::centred);
}

void AudiosphereLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& background,
                                                   bool highlighted, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto colour = button.getToggleState() ? button.findColour (juce::TextButton::buttonOnColourId) : background;
    if (down)
        colour = colour.brighter (0.2f);
    else if (highlighted)
        colour = colour.brighter (0.08f);

    g.setColour (colour);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (button.getToggleState() ? Theme::cyan : Theme::outline);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

juce::Font AudiosphereLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return Theme::font (std::min (13.0f, (float) buttonHeight * 0.5f), true);
}

void AudiosphereLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int> (width, height).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (box.hasKeyboardFocus (true) ? Theme::cyan : box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    const float arrowSize = std::min (8.0f, (float) height * 0.3f);
    const auto arrowCentre = juce::Point<float> ((float) width - 10.0f, (float) height * 0.5f);
    juce::Path arrow;
    arrow.addTriangle (arrowCentre.x - arrowSize * 0.5f, arrowCentre.y - arrowSize * 0.25f,
                       arrowCentre.x + arrowSize * 0.5f, arrowCentre.y - arrowSize * 0.25f,
                       arrowCentre.x, arrowCentre.y + arrowSize * 0.35f);
    g.setColour (box.isEnabled() ? box.findColour (juce::ComboBox::arrowColourId) : Theme::outline);
    g.fillPath (arrow);
}

juce::Font AudiosphereLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return Theme::font (std::min (12.5f, (float) box.getHeight() * 0.55f));
}

void AudiosphereLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 1, box.getWidth() - 20, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font AudiosphereLookAndFeel::getPopupMenuFont()
{
    return Theme::font (14.0f);
}

void AudiosphereLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (Theme::panelLight);
    g.setColour (Theme::outline);
    g.drawRect (0, 0, width, height);
}

juce::Font AudiosphereLookAndFeel::getLabelFont (juce::Label& label)
{
    return label.getFont();
}
