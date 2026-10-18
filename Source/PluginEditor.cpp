#include "PluginEditor.h"

namespace
{
constexpr int pageY = 104;
constexpr int pageHeight = 590;
constexpr int keyboardHeight = 84;

// Lays components out left-to-right, sharing the width equally.
void placeRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> comps, int gap = 0)
{
    const int n = (int) comps.size();
    if (n == 0)
        return;
    const int w = (area.getWidth() - gap * (n - 1)) / n;
    for (auto* c : comps)
    {
        c->setBounds (area.removeFromLeft (w));
        area.removeFromLeft (gap);
    }
}

juce::Label* styleSubtitle (juce::Label* l, const juce::String& text, juce::Colour colour)
{
    l->setText (text, juce::dontSendNotification);
    l->setFont (Theme::font (11.0f, true));
    l->setColour (juce::Label::textColourId, colour);
    l->setJustificationType (juce::Justification::centredLeft);
    return l;
}
}

//==============================================================================
// Base for pages: owns its child components and offers small factory helpers.
struct PageBase : public juce::Component
{
    explicit PageBase (APVTS& s) : state (s) {}

    template <typename T, typename... Args>
    T* make (Args&&... args)
    {
        auto c = std::make_unique<T> (std::forward<Args> (args)...);
        auto* raw = c.get();
        addAndMakeVisible (raw);
        owned.push_back (std::move (c));
        return raw;
    }

    Knob* knob (const juce::String& id, const juce::String& caption, juce::Colour accent) { return make<Knob> (state, id, caption, accent); }
    ChoiceBox* choice (const juce::String& id, const juce::String& caption = {})        { return make<ChoiceBox> (state, id, caption); }
    ParamToggle* toggle (const juce::String& id, const juce::String& text, juce::Colour accent) { return make<ParamToggle> (state, id, text, accent); }
    Section* section (const juce::String& title, juce::Colour accent, bool power = false) { return make<Section> (title, accent, power); }

    APVTS& state;
    std::vector<std::unique_ptr<juce::Component>> owned;
};

//==============================================================================
struct AudiosphereEditor::Header : public PageBase
{
    Header (AudiosphereProcessor& p)
        : PageBase (p.apvts),
          presetBar (p.presetManager),
          scope ([&p] (float* d, int n) { p.copyScope (d, n); })
    {
        addAndMakeVisible (presetBar);
        addAndMakeVisible (scope);

        for (int m = 0; m < Params::numMacros; ++m)
            macros[(size_t) m] = knob (Params::macro (m), "MACRO " + juce::String (m + 1), Theme::violet);
        master = knob ("master_vol", "MASTER", Theme::cyan);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Theme::header);
        g.fillRect (getLocalBounds());
        g.setColour (Theme::outline);
        g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());

        // Logo: a small sphere mark + wordmark
        const auto mark = juce::Rectangle<float> (16.0f, 15.0f, 34.0f, 34.0f);
        g.setGradientFill (juce::ColourGradient (Theme::cyan, mark.getX(), mark.getY(), Theme::violet, mark.getRight(), mark.getBottom(), false));
        g.fillEllipse (mark);
        g.setColour (Theme::header.withAlpha (0.85f));
        g.drawEllipse (mark.reduced (7.0f, 2.0f), 1.6f);
        g.drawLine (mark.getX() + 2.0f, mark.getCentreY(), mark.getRight() - 2.0f, mark.getCentreY(), 1.6f);

        g.setColour (Theme::text);
        g.setFont (Theme::font (21.0f, true));
        g.drawText ("AUDIOSPHERE", 60, 12, 180, 24, juce::Justification::centredLeft);
        g.setColour (Theme::textDim);
        g.setFont (Theme::font (10.0f, true));
        g.drawText ("WAVETABLE SYNTH  //  HIP HOP EDITION", 61, 36, 200, 14, juce::Justification::centredLeft);
    }

    void resized() override
    {
        presetBar.setBounds (270, 8, 400, 50);
        for (int m = 0; m < Params::numMacros; ++m)
            macros[(size_t) m]->setBounds (690 + m * 56, 4, 56, 58);
        master->setBounds (918, 4, 60, 58);
        scope.setBounds (990, 10, 196, 46);
    }

    PresetBar presetBar;
    ScopeView scope;
    std::array<Knob*, Params::numMacros> macros {};
    Knob* master = nullptr;
};

//==============================================================================
struct AudiosphereEditor::SynthPage : public PageBase
{
    struct OscUI
    {
        Section* section; ParamToggle* power; ChoiceBox* table; WavetableView* view;
        Knob *pos, *level, *pan, *oct, *semi, *fine, *unison, *detune, *spread, *warp;
        ChoiceBox* warpMode; ParamToggle* retrig;
    };

    struct EnvUI { juce::Label* title; EnvelopeView* view; Knob *a, *d, *s, *r; };

    struct LfoUI
    {
        Section* section; LfoView* view; ChoiceBox* shape; Knob *rate, *fade;
        ParamToggle *sync, *retrig; ChoiceBox* div;
    };

    explicit SynthPage (APVTS& s) : PageBase (s)
    {
        for (int o = 0; o < 2; ++o)
        {
            const auto accent = o == 0 ? Theme::cyan : Theme::violet;
            auto& ui = osc[(size_t) o];
            ui.section = section (o == 0 ? "OSC A" : "OSC B", accent, true);
            ui.power = toggle (Params::osc (o, "on"), {}, accent);
            ui.table = choice (Params::osc (o, "table"));
            ui.view = make<WavetableView> (state, o, accent);
            ui.pos = knob (Params::osc (o, "pos"), "WT POS", accent);
            ui.level = knob (Params::osc (o, "level"), "LEVEL", accent);
            ui.pan = knob (Params::osc (o, "pan"), "PAN", accent);
            ui.oct = knob (Params::osc (o, "oct"), "OCT", accent);
            ui.semi = knob (Params::osc (o, "semi"), "SEMI", accent);
            ui.fine = knob (Params::osc (o, "fine"), "FINE", accent);
            ui.unison = knob (Params::osc (o, "unison"), "UNISON", accent);
            ui.detune = knob (Params::osc (o, "detune"), "DETUNE", accent);
            ui.spread = knob (Params::osc (o, "spread"), "SPREAD", accent);
            ui.warp = knob (Params::osc (o, "warp"), "WARP", accent);
            ui.warpMode = choice (Params::osc (o, "warp_mode"), "WARP MODE");
            ui.retrig = toggle (Params::osc (o, "retrig"), "RETRIG", accent);
        }

        subSection = section ("SUB", Theme::cyan, true);
        subPower = toggle ("sub_on", {}, Theme::cyan);
        subShape = choice ("sub_shape");
        subOct = knob ("sub_oct", "OCT", Theme::cyan);
        subLevel = knob ("sub_level", "LEVEL", Theme::cyan);

        noiseSection = section ("NOISE", Theme::cyan, true);
        noisePower = toggle ("noise_on", {}, Theme::cyan);
        noiseType = choice ("noise_type");
        noiseLevel = knob ("noise_level", "LEVEL", Theme::cyan);

        filterSection = section ("FILTER", Theme::amber, true);
        filterPower = toggle ("flt_on", {}, Theme::amber);
        filterType = choice ("flt_type");
        cutoff = knob ("flt_cutoff", "CUTOFF", Theme::amber);
        res = knob ("flt_res", "RESONANCE", Theme::amber);
        drive = knob ("flt_drive", "DRIVE", Theme::amber);
        envAmt = knob ("flt_env", "ENV AMT", Theme::amber);
        keyTrack = knob ("flt_key", "KEY TRACK", Theme::amber);
        velAmt = knob ("flt_vel", "VELOCITY", Theme::amber);

        envSection = section ("ENVELOPES", Theme::green);
        const char* prefixes[] = { "amp", "fenv", "menv" };
        const char* titles[] = { "AMP", "FILTER", "MOD" };
        const juce::Colour envColours[] = { Theme::green, Theme::amber, Theme::pink };
        for (int e = 0; e < 3; ++e)
        {
            auto& ui = env[(size_t) e];
            const juce::String p (prefixes[e]);
            ui.title = styleSubtitle (make<juce::Label>(), titles[e], envColours[e]);
            ui.view = make<EnvelopeView> (state, p, envColours[e]);
            ui.a = knob (p + "_a", "ATTACK", envColours[e]);
            ui.d = knob (p + "_d", "DECAY", envColours[e]);
            ui.s = knob (p + "_s", "SUSTAIN", envColours[e]);
            ui.r = knob (p + "_r", "RELEASE", envColours[e]);
        }

        for (int l = 0; l < 2; ++l)
        {
            auto& ui = lfo[(size_t) l];
            ui.section = section ("LFO " + juce::String (l + 1), Theme::pink);
            ui.view = make<LfoView> (state, l, Theme::pink);
            ui.shape = choice (Params::lfo (l, "shape"));
            ui.rate = knob (Params::lfo (l, "rate"), "RATE", Theme::pink);
            ui.fade = knob (Params::lfo (l, "fade"), "FADE IN", Theme::pink);
            ui.sync = toggle (Params::lfo (l, "sync"), "SYNC", Theme::pink);
            ui.retrig = toggle (Params::lfo (l, "retrig"), "RETRIG", Theme::pink);
            ui.div = choice (Params::lfo (l, "div"));
        }

        voiceSection = section ("VOICE", Theme::cyan);
        voiceMode = choice ("voice_mode", "MODE");
        polyphony = knob ("polyphony", "VOICES", Theme::cyan);
        glide = knob ("glide", "GLIDE", Theme::cyan);
        bend = knob ("bend_range", "BEND", Theme::cyan);
        velocity = knob ("amp_vel", "VELOCITY", Theme::cyan);
    }

    void refresh()
    {
        for (int l = 0; l < 2; ++l)
        {
            const bool synced = state.getRawParameterValue (Params::lfo (l, "sync"))->load() >= 0.5f;
            lfo[(size_t) l].rate->setEnabled (! synced);
            lfo[(size_t) l].div->setEnabled (synced);
        }
    }

    void resized() override
    {
        // ---- Row 1: oscillators, sub/noise, filter -------------------------------
        for (int o = 0; o < 2; ++o)
        {
            auto& ui = osc[(size_t) o];
            ui.section->setBounds (10 + o * 360, 0, 350, 300);
            ui.power->setBounds (ui.section->getPowerButtonBounds());
            ui.table->setBounds (ui.section->getTitleRightArea (150));

            auto c = ui.section->getContentBounds();
            ui.view->setBounds (c.removeFromTop (106));
            c.removeFromTop (6);
            placeRow (c.removeFromTop (62), { ui.pos, ui.level, ui.pan, ui.oct, ui.semi, ui.fine });
            c.removeFromTop (6);
            auto row = c.removeFromTop (62);
            placeRow (row.removeFromLeft (row.getWidth() * 4 / 6), { ui.unison, ui.detune, ui.spread, ui.warp });
            row.removeFromLeft (4);
            ui.warpMode->setBounds (row.removeFromTop (36));
            row.removeFromTop (4);
            ui.retrig->setBounds (row.removeFromTop (20));
        }

        subSection->setBounds (730, 0, 150, 146);
        subPower->setBounds (subSection->getPowerButtonBounds());
        {
            auto c = subSection->getContentBounds();
            subShape->setBounds (c.removeFromTop (22));
            c.removeFromTop (6);
            placeRow (c.removeFromTop (62), { subOct, subLevel });
        }

        noiseSection->setBounds (730, 154, 150, 146);
        noisePower->setBounds (noiseSection->getPowerButtonBounds());
        {
            auto c = noiseSection->getContentBounds();
            noiseType->setBounds (c.removeFromTop (22));
            c.removeFromTop (6);
            noiseLevel->setBounds (c.removeFromTop (62).withSizeKeepingCentre (64, 62));
        }

        filterSection->setBounds (890, 0, 300, 300);
        filterPower->setBounds (filterSection->getPowerButtonBounds());
        filterType->setBounds (filterSection->getTitleRightArea (110));
        {
            auto c = filterSection->getContentBounds();
            c.removeFromTop (8);
            auto top = c.removeFromTop (120);
            cutoff->setBounds (top.removeFromLeft (top.getWidth() / 2).reduced (6, 0));
            placeRow (top.withTrimmedTop (30), { res, drive });
            c.removeFromTop (20);
            placeRow (c.removeFromTop (72), { envAmt, keyTrack, velAmt });
        }

        // ---- Row 2: envelopes, LFOs, voice ------------------------------------------
        envSection->setBounds (10, 310, 670, 280);
        {
            auto c = envSection->getContentBounds();
            const int colW = c.getWidth() / 3;
            for (int e = 0; e < 3; ++e)
            {
                auto col = c.removeFromLeft (colW).reduced (6, 0);
                auto& ui = env[(size_t) e];
                ui.title->setBounds (col.removeFromTop (18));
                ui.view->setBounds (col.removeFromTop (130));
                col.removeFromTop (8);
                placeRow (col.removeFromTop (64), { ui.a, ui.d, ui.s, ui.r });
            }
        }

        for (int l = 0; l < 2; ++l)
        {
            auto& ui = lfo[(size_t) l];
            ui.section->setBounds (690 + l * 175, 310, 165, 280);
            auto c = ui.section->getContentBounds();
            ui.view->setBounds (c.removeFromTop (64));
            c.removeFromTop (6);
            ui.shape->setBounds (c.removeFromTop (22));
            c.removeFromTop (6);
            placeRow (c.removeFromTop (62), { ui.rate, ui.fade });
            c.removeFromTop (6);
            placeRow (c.removeFromTop (20), { ui.sync, ui.retrig }, 6);
            c.removeFromTop (6);
            ui.div->setBounds (c.removeFromTop (22));
        }

        voiceSection->setBounds (1040, 310, 150, 280);
        {
            auto c = voiceSection->getContentBounds();
            voiceMode->setBounds (c.removeFromTop (38));
            c.removeFromTop (10);
            placeRow (c.removeFromTop (64), { polyphony, glide });
            c.removeFromTop (10);
            placeRow (c.removeFromTop (64), { bend, velocity });
        }
    }

    std::array<OscUI, 2> osc {};
    Section *subSection, *noiseSection, *filterSection, *envSection, *voiceSection;
    ParamToggle *subPower, *noisePower, *filterPower;
    ChoiceBox *subShape, *noiseType, *filterType, *voiceMode;
    Knob *subOct, *subLevel, *noiseLevel, *cutoff, *res, *drive, *envAmt, *keyTrack, *velAmt;
    Knob *polyphony, *glide, *bend, *velocity;
    std::array<EnvUI, 3> env {};
    std::array<LfoUI, 2> lfo {};
};

//==============================================================================
struct AudiosphereEditor::ModPage : public PageBase
{
    struct Row
    {
        juce::Label* number; ChoiceBox *src, *dst; juce::Slider* amount; juce::Label* value;
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };

    explicit ModPage (APVTS& s) : PageBase (s)
    {
        matrixSection = section ("MODULATION MATRIX", Theme::violet);
        for (auto* h : { &srcHeader, &dstHeader, &amtHeader })
            addAndMakeVisible (h);
        styleSubtitle (&srcHeader, "SOURCE", Theme::textDim);
        styleSubtitle (&dstHeader, "DESTINATION", Theme::textDim);
        styleSubtitle (&amtHeader, "AMOUNT", Theme::textDim);

        for (int i = 0; i < Params::numModSlots; ++i)
        {
            auto& row = rows[(size_t) i];
            row.number = styleSubtitle (make<juce::Label>(), juce::String (i + 1), Theme::violet);
            row.number->setJustificationType (juce::Justification::centred);
            row.src = choice (Params::mod (i, "src"));
            row.dst = choice (Params::mod (i, "dst"));
            row.amount = make<juce::Slider> (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
            row.amount->setColour (juce::Slider::rotarySliderFillColourId, Theme::violet);
            row.amount->getProperties().set ("bipolar", true);
            row.attachment = std::make_unique<APVTS::SliderAttachment> (state, Params::mod (i, "amt"), *row.amount);
            row.amount->setDoubleClickReturnValue (true, 0.0);
            row.value = make<juce::Label>();
            row.value->setFont (Theme::font (12.0f, true));
            row.value->setColour (juce::Label::textColourId, Theme::text);
            row.value->setJustificationType (juce::Justification::centredRight);

            auto* param = state.getParameter (Params::mod (i, "amt"));
            auto* label = row.value;
            row.amount->onValueChange = [param, label] { label->setText (param->getCurrentValueAsText(), juce::dontSendNotification); };
            row.amount->onValueChange();
        }

        macroSection = section ("MACROS", Theme::violet);
        for (int m = 0; m < Params::numMacros; ++m)
            macros[(size_t) m] = knob (Params::macro (m), "MACRO " + juce::String (m + 1), Theme::violet);

        helpSection = section ("HOW TO MODULATE", Theme::violet);
        help = make<juce::Label>();
        help->setFont (Theme::font (12.5f));
        help->setColour (juce::Label::textColourId, Theme::textDim);
        help->setJustificationType (juce::Justification::topLeft);
        help->setText ("Pick a source and a destination, then dial in the amount (double-click resets it).\n\n"
                       "Sources: LFO 1/2, the three envelopes, velocity, mod wheel, aftertouch, key track, "
                       "a per-note random value, pitch bend and the four macros.\n\n"
                       "Macros are also on the top bar. In Ableton, click Configure on the device and move a "
                       "macro to expose it, then map it to an Instrument Rack macro.\n\n"
                       "808 tip: Mod Env -> Pitch with a short decay gives the classic punch.",
                       juce::dontSendNotification);
    }

    void resized() override
    {
        matrixSection->setBounds (10, 0, 780, 590);
        auto c = matrixSection->getContentBounds().reduced (8, 4);
        auto headerRow = c.removeFromTop (20);
        headerRow.removeFromLeft (36);
        srcHeader.setBounds (headerRow.removeFromLeft (220));
        headerRow.removeFromLeft (30);
        dstHeader.setBounds (headerRow.removeFromLeft (220));
        headerRow.removeFromLeft (16);
        amtHeader.setBounds (headerRow);
        c.removeFromTop (8);

        const int rowH = c.getHeight() / Params::numModSlots;
        for (auto& row : rows)
        {
            auto r = c.removeFromTop (rowH).withSizeKeepingCentre (c.getWidth(), 28);
            row.number->setBounds (r.removeFromLeft (30));
            r.removeFromLeft (6);
            row.src->setBounds (r.removeFromLeft (220));
            r.removeFromLeft (30);
            row.dst->setBounds (r.removeFromLeft (220));
            r.removeFromLeft (16);
            row.value->setBounds (r.removeFromRight (60));
            row.amount->setBounds (r);
        }

        macroSection->setBounds (800, 0, 390, 200);
        placeRow (macroSection->getContentBounds().withSizeKeepingCentre (360, 90), { macros[0], macros[1], macros[2], macros[3] });

        helpSection->setBounds (800, 210, 390, 380);
        help->setBounds (helpSection->getContentBounds().reduced (4, 6));
    }

    // Draw the arrows between source and destination.
    void paintOverChildren (juce::Graphics& g) override
    {
        g.setColour (Theme::textDim);
        g.setFont (Theme::font (14.0f, true));
        for (auto& row : rows)
        {
            const auto b = row.src->getBounds();
            g.drawText (juce::CharPointer_UTF8 ("\xe2\x86\x92"), b.getRight(), b.getY(), 30, b.getHeight(), juce::Justification::centred);
        }
    }

    Section *matrixSection, *macroSection, *helpSection;
    juce::Label srcHeader, dstHeader, amtHeader;
    juce::Label* help;
    std::array<Row, Params::numModSlots> rows {};
    std::array<Knob*, Params::numMacros> macros {};
};

//==============================================================================
struct AudiosphereEditor::FxPage : public PageBase
{
    explicit FxPage (APVTS& s) : PageBase (s)
    {
        distSection = section ("DISTORTION", Theme::pink, true);
        distPower = toggle ("dist_on", {}, Theme::pink);
        distType = choice ("dist_type");
        distDrive = knob ("dist_drive", "DRIVE", Theme::pink);
        distTone = knob ("dist_tone", "TONE", Theme::pink);
        distMix = knob ("dist_mix", "MIX", Theme::pink);

        lofiSection = section ("LO-FI", Theme::amber, true);
        lofiPower = toggle ("lofi_on", {}, Theme::amber);
        lofiBits = knob ("lofi_bits", "BITS", Theme::amber);
        lofiRate = knob ("lofi_rate", "RATE", Theme::amber);
        lofiWow = knob ("lofi_wow", "WOW", Theme::amber);
        lofiNoise = knob ("lofi_noise", "VINYL", Theme::amber);
        lofiTone = knob ("lofi_tone", "TONE", Theme::amber);
        lofiMix = knob ("lofi_mix", "MIX", Theme::amber);

        eqSection = section ("EQ", Theme::green, true);
        eqPower = toggle ("eq_on", {}, Theme::green);
        eqLow = knob ("eq_low", "LOW", Theme::green);
        eqMid = knob ("eq_mid", "MID", Theme::green);
        eqHigh = knob ("eq_high", "HIGH", Theme::green);

        outSection = section ("OUTPUT", Theme::cyan);
        master = knob ("master_vol", "MASTER", Theme::cyan);
        limiter = toggle ("limiter_on", "LIMITER", Theme::cyan);

        chorusSection = section ("CHORUS", Theme::violet, true);
        chorusPower = toggle ("chorus_on", {}, Theme::violet);
        chorusRate = knob ("chorus_rate", "RATE", Theme::violet);
        chorusDepth = knob ("chorus_depth", "DEPTH", Theme::violet);
        chorusMix = knob ("chorus_mix", "MIX", Theme::violet);

        delaySection = section ("DELAY", Theme::cyan, true);
        delayPower = toggle ("delay_on", {}, Theme::cyan);
        delaySync = toggle ("delay_sync", "SYNC", Theme::cyan);
        delayPing = toggle ("delay_pingpong", "PING-PONG", Theme::cyan);
        delayDiv = choice ("delay_div", "TIME (SYNC)");
        delayTime = knob ("delay_time", "TIME", Theme::cyan);
        delayFb = knob ("delay_fb", "FEEDBACK", Theme::cyan);
        delayTone = knob ("delay_tone", "TONE", Theme::cyan);
        delayMix = knob ("delay_mix", "MIX", Theme::cyan);

        reverbSection = section ("REVERB", Theme::green, true);
        reverbPower = toggle ("reverb_on", {}, Theme::green);
        reverbSize = knob ("reverb_size", "SIZE", Theme::green);
        reverbDamp = knob ("reverb_damp", "DAMP", Theme::green);
        reverbWidth = knob ("reverb_width", "WIDTH", Theme::green);
        reverbMix = knob ("reverb_mix", "MIX", Theme::green);
    }

    void refresh()
    {
        const bool synced = state.getRawParameterValue ("delay_sync")->load() >= 0.5f;
        delayTime->setEnabled (! synced);
        delayDiv->setEnabled (synced);
    }

    void resized() override
    {
        const int bigKnobH = 92;

        distSection->setBounds (10, 0, 290, 285);
        distPower->setBounds (distSection->getPowerButtonBounds());
        {
            auto c = distSection->getContentBounds();
            distType->setBounds (c.removeFromTop (24).reduced (20, 0));
            c.removeFromTop (30);
            placeRow (c.removeFromTop (bigKnobH), { distDrive, distTone, distMix });
        }

        lofiSection->setBounds (310, 0, 410, 285);
        lofiPower->setBounds (lofiSection->getPowerButtonBounds());
        {
            auto c = lofiSection->getContentBounds();
            c.removeFromTop (8);
            placeRow (c.removeFromTop (bigKnobH), { lofiBits, lofiRate, lofiWow });
            c.removeFromTop (16);
            placeRow (c.removeFromTop (bigKnobH), { lofiNoise, lofiTone, lofiMix });
        }

        eqSection->setBounds (730, 0, 250, 285);
        eqPower->setBounds (eqSection->getPowerButtonBounds());
        placeRow (eqSection->getContentBounds().withTrimmedTop (54).withHeight (bigKnobH), { eqLow, eqMid, eqHigh });

        outSection->setBounds (990, 0, 200, 285);
        {
            auto c = outSection->getContentBounds();
            c.removeFromTop (30);
            master->setBounds (c.removeFromTop (110).withSizeKeepingCentre (100, 110));
            c.removeFromTop (20);
            limiter->setBounds (c.removeFromTop (24).withSizeKeepingCentre (110, 24));
        }

        chorusSection->setBounds (10, 295, 290, 295);
        chorusPower->setBounds (chorusSection->getPowerButtonBounds());
        placeRow (chorusSection->getContentBounds().withTrimmedTop (60).withHeight (bigKnobH), { chorusRate, chorusDepth, chorusMix });

        delaySection->setBounds (310, 295, 470, 295);
        delayPower->setBounds (delaySection->getPowerButtonBounds());
        {
            auto c = delaySection->getContentBounds();
            c.removeFromTop (6);
            auto top = c.removeFromTop (38);
            delaySync->setBounds (top.removeFromLeft (70).withTrimmedTop (14));
            top.removeFromLeft (8);
            delayPing->setBounds (top.removeFromLeft (100).withTrimmedTop (14));
            top.removeFromLeft (16);
            delayDiv->setBounds (top.removeFromLeft (130));
            c.removeFromTop (30);
            placeRow (c.removeFromTop (bigKnobH), { delayTime, delayFb, delayTone, delayMix });
        }

        reverbSection->setBounds (790, 295, 400, 295);
        reverbPower->setBounds (reverbSection->getPowerButtonBounds());
        placeRow (reverbSection->getContentBounds().withTrimmedTop (60).withHeight (bigKnobH), { reverbSize, reverbDamp, reverbWidth, reverbMix });
    }

    Section *distSection, *lofiSection, *eqSection, *outSection, *chorusSection, *delaySection, *reverbSection;
    ParamToggle *distPower, *lofiPower, *eqPower, *chorusPower, *delayPower, *reverbPower, *limiter, *delaySync, *delayPing;
    ChoiceBox *distType, *delayDiv;
    Knob *distDrive, *distTone, *distMix, *lofiBits, *lofiRate, *lofiWow, *lofiNoise, *lofiTone, *lofiMix;
    Knob *eqLow, *eqMid, *eqHigh, *master, *chorusRate, *chorusDepth, *chorusMix;
    Knob *delayTime, *delayFb, *delayTone, *delayMix, *reverbSize, *reverbDamp, *reverbWidth, *reverbMix;
};

//==============================================================================
AudiosphereEditor::AudiosphereEditor (AudiosphereProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (content);

    header = std::make_unique<Header> (p);
    synthPage = std::make_unique<SynthPage> (p.apvts);
    modPage = std::make_unique<ModPage> (p.apvts);
    fxPage = std::make_unique<FxPage> (p.apvts);

    for (auto* c : std::initializer_list<juce::Component*> { header.get(), synthPage.get(), modPage.get(), fxPage.get() })
        content.addAndMakeVisible (c);

    int tabIndex = 0;
    for (auto* tab : { &synthTab, &modTab, &fxTab })
    {
        tab->setClickingTogglesState (false);
        tab->onClick = [this, tabIndex] { showPage (tabIndex); };
        content.addAndMakeVisible (tab);
        ++tabIndex;
    }

    voiceLabel.setFont (Theme::font (11.0f, true));
    voiceLabel.setColour (juce::Label::textColourId, Theme::textDim);
    voiceLabel.setJustificationType (juce::Justification::centredRight);
    content.addAndMakeVisible (voiceLabel);
    content.addAndMakeVisible (updateButton);

    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (24);
    keyboard.setWantsKeyboardFocus (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffe6e9f0));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, Theme::panel);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, Theme::outline);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Theme::cyan.withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Theme::cyan.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, Theme::panel);
    content.addAndMakeVisible (keyboard);

    showPage (0);

    setResizable (true, true);
    setResizeLimits (baseWidth * 6 / 10, baseHeight * 6 / 10, baseWidth * 2, baseHeight * 2);
    if (auto* sizeConstrainer = getConstrainer())
        sizeConstrainer->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (10);
    updates->checkIfDue();
}

AudiosphereEditor::~AudiosphereEditor()
{
    setLookAndFeel (nullptr);
}

void AudiosphereEditor::showPage (int index)
{
    synthPage->setVisible (index == 0);
    modPage->setVisible (index == 1);
    fxPage->setVisible (index == 2);
    synthTab.setToggleState (index == 0, juce::dontSendNotification);
    modTab.setToggleState (index == 1, juce::dontSendNotification);
    fxTab.setToggleState (index == 2, juce::dontSendNotification);
}

void AudiosphereEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);
}

void AudiosphereEditor::resized()
{
    const float scale = (float) getWidth() / (float) baseWidth;
    content.setBounds (0, 0, baseWidth, baseHeight);
    content.setTransform (juce::AffineTransform::scale (scale));

    header->setBounds (0, 0, baseWidth, 66);
    synthTab.setBounds (10, 72, 110, 26);
    modTab.setBounds (126, 72, 130, 26);
    fxTab.setBounds (262, 72, 110, 26);
    voiceLabel.setBounds (baseWidth - 110, 72, 100, 26);
    updateButton.setBounds (baseWidth - 330, 72, 210, 26);

    for (auto* page : std::initializer_list<juce::Component*> { synthPage.get(), modPage.get(), fxPage.get() })
        page->setBounds (0, pageY, baseWidth, pageHeight);

    const auto keyArea = juce::Rectangle<int> (10, baseHeight - keyboardHeight - 6, baseWidth - 20, keyboardHeight);
    keyboard.setBounds (keyArea);
    keyboard.setKeyWidth ((float) keyArea.getWidth() / 50.0f);   // 50 white keys from C1 to C8
}

void AudiosphereEditor::timerCallback()
{
    synthPage->refresh();
    fxPage->refresh();
    voiceLabel.setText ("VOICES " + juce::String (processor.getActiveVoiceCount()), juce::dontSendNotification);
}
