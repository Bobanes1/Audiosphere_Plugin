#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Theme
{
    const juce::Colour background   { 0xff0d0f15 };
    const juce::Colour header       { 0xff141823 };
    const juce::Colour panel        { 0xff161a24 };
    const juce::Colour panelLight   { 0xff1f2432 };
    const juce::Colour outline      { 0xff2a3142 };
    const juce::Colour knobBody     { 0xff262c3a };
    const juce::Colour track        { 0xff2c3344 };
    const juce::Colour text         { 0xffd8deea };
    const juce::Colour textDim      { 0xff8590a6 };

    const juce::Colour cyan         { 0xff38d6ec };
    const juce::Colour violet       { 0xffa47cff };
    const juce::Colour amber        { 0xffffb547 };
    const juce::Colour pink         { 0xffff5fa2 };
    const juce::Colour green        { 0xff5ee69b };

    inline juce::Font font (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
    }
}

class AudiosphereLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AudiosphereLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& background,
                               bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    juce::Font getLabelFont (juce::Label&) override;
};
