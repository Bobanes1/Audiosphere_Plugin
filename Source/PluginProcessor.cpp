#include "PluginProcessor.h"

#if ! AUDIOSPHERE_HEADLESS
 #include "PluginEditor.h"
#endif

AudiosphereProcessor::AudiosphereProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AudiosphereState", Params::createLayout()),
      presetManager (apvts)
{
    WavetableBank::get();   // build the tables up front rather than on the audio thread
    cacheParameterPointers();
    presetManager.loadInitialPreset();
}

void AudiosphereProcessor::cacheParameterPointers()
{
    auto p = [this] (const juce::String& id)
    {
        auto* ptr = apvts.getRawParameterValue (id);
        jassert (ptr != nullptr);
        return ptr;
    };

    pp.masterVol = p ("master_vol");
    pp.voiceMode = p ("voice_mode");
    pp.polyphony = p ("polyphony");
    pp.glide     = p ("glide");
    pp.bendRange = p ("bend_range");
    pp.ampVel    = p ("amp_vel");
    for (int m = 0; m < Params::numMacros; ++m)
        pp.macros[(size_t) m] = p (Params::macro (m));

    for (int o = 0; o < 2; ++o)
    {
        auto& d = pp.osc[(size_t) o];
        d.on       = p (Params::osc (o, "on"));
        d.table    = p (Params::osc (o, "table"));
        d.pos      = p (Params::osc (o, "pos"));
        d.oct      = p (Params::osc (o, "oct"));
        d.semi     = p (Params::osc (o, "semi"));
        d.fine     = p (Params::osc (o, "fine"));
        d.unison   = p (Params::osc (o, "unison"));
        d.detune   = p (Params::osc (o, "detune"));
        d.spread   = p (Params::osc (o, "spread"));
        d.level    = p (Params::osc (o, "level"));
        d.pan      = p (Params::osc (o, "pan"));
        d.warpMode = p (Params::osc (o, "warp_mode"));
        d.warp     = p (Params::osc (o, "warp"));
        d.retrig   = p (Params::osc (o, "retrig"));
    }

    pp.subOn = p ("sub_on");       pp.subShape = p ("sub_shape");   pp.subOct = p ("sub_oct");   pp.subLevel = p ("sub_level");
    pp.noiseOn = p ("noise_on");   pp.noiseType = p ("noise_type"); pp.noiseLevel = p ("noise_level");

    pp.fltOn = p ("flt_on");       pp.fltType = p ("flt_type");     pp.fltCutoff = p ("flt_cutoff");
    pp.fltRes = p ("flt_res");     pp.fltDrive = p ("flt_drive");   pp.fltEnv = p ("flt_env");
    pp.fltKey = p ("flt_key");     pp.fltVel = p ("flt_vel");

    auto env = [&p] (const juce::String& prefix) -> ParamPointers::Env
    {
        return { p (prefix + "_a"), p (prefix + "_d"), p (prefix + "_s"), p (prefix + "_r") };
    };
    pp.amp = env ("amp");
    pp.fenv = env ("fenv");
    pp.menv = env ("menv");

    for (int l = 0; l < 2; ++l)
        pp.lfo[(size_t) l] = { p (Params::lfo (l, "shape")), p (Params::lfo (l, "rate")), p (Params::lfo (l, "sync")),
                               p (Params::lfo (l, "div")), p (Params::lfo (l, "retrig")), p (Params::lfo (l, "fade")) };

    for (int s = 0; s < Params::numModSlots; ++s)
        pp.mod[(size_t) s] = { p (Params::mod (s, "src")), p (Params::mod (s, "dst")), p (Params::mod (s, "amt")) };

    pp.distOn = p ("dist_on");   pp.distType = p ("dist_type");  pp.distDrive = p ("dist_drive");
    pp.distTone = p ("dist_tone"); pp.distMix = p ("dist_mix");

    pp.lofiOn = p ("lofi_on");   pp.lofiBits = p ("lofi_bits");  pp.lofiRate = p ("lofi_rate");
    pp.lofiWow = p ("lofi_wow"); pp.lofiNoise = p ("lofi_noise"); pp.lofiTone = p ("lofi_tone"); pp.lofiMix = p ("lofi_mix");

    pp.eqOn = p ("eq_on"); pp.eqLow = p ("eq_low"); pp.eqMid = p ("eq_mid"); pp.eqHigh = p ("eq_high");

    pp.chorusOn = p ("chorus_on"); pp.chorusRate = p ("chorus_rate"); pp.chorusDepth = p ("chorus_depth"); pp.chorusMix = p ("chorus_mix");

    pp.delayOn = p ("delay_on");     pp.delaySync = p ("delay_sync");         pp.delayTime = p ("delay_time");
    pp.delayDiv = p ("delay_div");   pp.delayFb = p ("delay_fb");             pp.delayPingPong = p ("delay_pingpong");
    pp.delayTone = p ("delay_tone"); pp.delayMix = p ("delay_mix");

    pp.reverbOn = p ("reverb_on");     pp.reverbSize = p ("reverb_size"); pp.reverbDamp = p ("reverb_damp");
    pp.reverbWidth = p ("reverb_width"); pp.reverbMix = p ("reverb_mix");

    pp.limiterOn = p ("limiter_on");
}

static inline int asInt (std::atomic<float>* p)   { return juce::roundToInt (p->load()); }
static inline bool asBool (std::atomic<float>* p) { return p->load() >= 0.5f; }

void AudiosphereProcessor::readSynthParams (SynthParams& s, double bpm) const
{
    for (int o = 0; o < 2; ++o)
    {
        const auto& d = pp.osc[(size_t) o];
        auto& out = s.osc[(size_t) o];
        out.on       = asBool (d.on);
        out.table    = asInt (d.table);
        out.pos      = d.pos->load();
        out.pitch    = (float) (asInt (d.oct) * 12 + asInt (d.semi)) + d.fine->load() / 100.0f;
        out.unison   = asInt (d.unison);
        out.detune   = d.detune->load();
        out.spread   = d.spread->load();
        out.level    = d.level->load();
        out.pan      = d.pan->load();
        out.warpMode = (WarpMode) asInt (d.warpMode);
        out.warp     = d.warp->load();
        out.retrig   = asBool (d.retrig);
    }

    s.subOn = asBool (pp.subOn);
    s.subShape = (SubShape) asInt (pp.subShape);
    s.subOct = asInt (pp.subOct);
    s.subLevel = pp.subLevel->load();

    s.noiseOn = asBool (pp.noiseOn);
    s.noiseType = (NoiseType) asInt (pp.noiseType);
    s.noiseLevel = pp.noiseLevel->load();

    s.filterOn = asBool (pp.fltOn);
    s.filterType = (FilterType) asInt (pp.fltType);
    s.cutoff = pp.fltCutoff->load();
    s.resonance = pp.fltRes->load();
    s.drive = pp.fltDrive->load();
    s.envAmount = pp.fltEnv->load();
    s.keyTrack = pp.fltKey->load();
    s.velToCutoff = pp.fltVel->load();

    auto env = [] (const ParamPointers::Env& e) -> SynthParams::Adsr
    {
        return { e.a->load(), e.d->load(), e.s->load(), e.r->load() };
    };
    s.ampEnv = env (pp.amp);
    s.filterEnv = env (pp.fenv);
    s.modEnv = env (pp.menv);

    for (int l = 0; l < 2; ++l)
    {
        const auto& d = pp.lfo[(size_t) l];
        auto& out = s.lfo[(size_t) l];
        out.shape = (LfoShape) asInt (d.shape);
        out.rateHz = asBool (d.sync) ? (float) (bpm / 60.0 / SyncDiv::beats (asInt (d.div))) : d.rate->load();
        out.retrig = asBool (d.retrig);
        out.fade = d.fade->load();
    }

    for (int m = 0; m < Params::numModSlots; ++m)
        s.mods[(size_t) m] = { asInt (pp.mod[(size_t) m].src), asInt (pp.mod[(size_t) m].dst), pp.mod[(size_t) m].amt->load() };

    for (int m = 0; m < Params::numMacros; ++m)
        s.macros[(size_t) m] = pp.macros[(size_t) m]->load();

    s.voiceMode = (VoiceMode) asInt (pp.voiceMode);
    s.polyphony = asInt (pp.polyphony);
    s.glide = pp.glide->load();
    s.bendRange = asInt (pp.bendRange);
    s.ampVelocity = pp.ampVel->load();
}

void AudiosphereProcessor::readFxParams (FxParams& f, double bpm) const
{
    f.distOn = asBool (pp.distOn);
    f.distType = (DistType) asInt (pp.distType);
    f.distDrive = pp.distDrive->load();
    f.distTone = pp.distTone->load();
    f.distMix = pp.distMix->load();

    f.lofiOn = asBool (pp.lofiOn);
    f.lofiBits = pp.lofiBits->load();
    f.lofiRate = pp.lofiRate->load();
    f.lofiWow = pp.lofiWow->load();
    f.lofiNoise = pp.lofiNoise->load();
    f.lofiTone = pp.lofiTone->load();
    f.lofiMix = pp.lofiMix->load();

    f.eqOn = asBool (pp.eqOn);
    f.eqLow = pp.eqLow->load();
    f.eqMid = pp.eqMid->load();
    f.eqHigh = pp.eqHigh->load();

    f.chorusOn = asBool (pp.chorusOn);
    f.chorusRate = pp.chorusRate->load();
    f.chorusDepth = pp.chorusDepth->load();
    f.chorusMix = pp.chorusMix->load();

    f.delayOn = asBool (pp.delayOn);
    f.delaySeconds = asBool (pp.delaySync) ? (float) (SyncDiv::beats (asInt (pp.delayDiv)) * 60.0 / bpm)
                                           : pp.delayTime->load() / 1000.0f;
    f.delayFeedback = pp.delayFb->load();
    f.delayPingPong = asBool (pp.delayPingPong);
    f.delayTone = pp.delayTone->load();
    f.delayMix = pp.delayMix->load();

    f.reverbOn = asBool (pp.reverbOn);
    f.reverbSize = pp.reverbSize->load();
    f.reverbDamp = pp.reverbDamp->load();
    f.reverbWidth = pp.reverbWidth->load();
    f.reverbMix = pp.reverbMix->load();

    f.masterGain = juce::Decibels::decibelsToGain (pp.masterVol->load(), -47.9f);
    f.limiterOn = asBool (pp.limiterOn);
}

//==============================================================================
void AudiosphereProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    const int maxBlock = std::max (samplesPerBlock, 16);
    workBuffer.setSize (2, maxBlock);
    engine.prepare (sr, maxBlock);
    fx.prepare (sr, maxBlock);
    setLatencySamples (fx.getLatencySamples());
    lfoPhase = {};
    keyboardState.reset();
}

bool AudiosphereProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet().isDisabled();
}

void AudiosphereProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    if (workBuffer.getNumSamples() < numSamples)
        workBuffer.setSize (2, numSamples, false, false, true);   // only if the host exceeds its promise

    // ---- Tempo & host position ------------------------------------------------
    double bpm = 120.0;
    juce::Optional<double> ppq;
    bool playing = false;
    if (auto* head = getPlayHead())
    {
        if (auto pos = head->getPosition())
        {
            if (auto b = pos->getBpm())
                bpm = juce::jlimit (20.0, 999.0, *b);
            playing = pos->getIsPlaying();
            ppq = pos->getPpqPosition();
        }
    }

    readSynthParams (synthParams, bpm);
    readFxParams (fxParams, bpm);

    // Free-running LFOs: lock to the song position when synced and playing.
    std::array<float, 2> phases {};
    for (int l = 0; l < 2; ++l)
    {
        const bool sync = asBool (pp.lfo[(size_t) l].sync);
        if (sync && playing && ppq.hasValue())
        {
            const double beats = SyncDiv::beats (asInt (pp.lfo[(size_t) l].div));
            lfoPhase[(size_t) l] = *ppq / beats - std::floor (*ppq / beats);
        }
        phases[(size_t) l] = (float) lfoPhase[(size_t) l];

        lfoPhase[(size_t) l] += synthParams.lfo[(size_t) l].rateHz * numSamples / sampleRate;
        lfoPhase[(size_t) l] -= std::floor (lfoPhase[(size_t) l]);
    }

    juce::AudioBuffer<float> block (workBuffer.getArrayOfWritePointers(), 2, numSamples);
    engine.process (block, midi, synthParams, phases);
    fx.process (block, fxParams);
    activeVoices.store (engine.getNumActiveVoices());

    // ---- Output -------------------------------------------------------------------
    const int outChannels = getTotalNumOutputChannels();
    if (outChannels >= 2)
    {
        buffer.copyFrom (0, 0, block, 0, 0, numSamples);
        buffer.copyFrom (1, 0, block, 1, 0, numSamples);
        for (int c = 2; c < outChannels; ++c)
            buffer.clear (c, 0, numSamples);
    }
    else if (outChannels == 1)
    {
        buffer.copyFrom (0, 0, block.getReadPointer (0), numSamples, 0.5f);
        buffer.addFrom (0, 0, block, 1, 0, numSamples, 0.5f);
    }

    int w = scopeWrite.load (std::memory_order_relaxed);
    const float* l = block.getReadPointer (0);
    const float* r = block.getReadPointer (1);
    for (int i = 0; i < numSamples; ++i)
    {
        scope[(size_t) w] = 0.5f * (l[i] + r[i]);
        w = (w + 1) % scopeSize;
    }
    scopeWrite.store (w, std::memory_order_release);
}

void AudiosphereProcessor::copyScope (float* dest, int numSamples) const
{
    numSamples = std::min (numSamples, scopeSize);
    const int end = scopeWrite.load (std::memory_order_acquire);
    for (int i = 0; i < numSamples; ++i)
        dest[i] = scope[(size_t) ((end - numSamples + i + scopeSize) % scopeSize)];
}

//==============================================================================
bool AudiosphereProcessor::hasEditor() const
{
   #if AUDIOSPHERE_HEADLESS
    return false;
   #else
    return true;
   #endif
}

juce::AudioProcessorEditor* AudiosphereProcessor::createEditor()
{
   #if AUDIOSPHERE_HEADLESS
    return nullptr;
   #else
    return new AudiosphereEditor (*this);
   #endif
}

void AudiosphereProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetManager.getCurrentName(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AudiosphereProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            const auto name = tree.getProperty ("presetName").toString();
            apvts.replaceState (tree);
            presetManager.setCurrentName (name.isNotEmpty() ? name : juce::String ("Init"));
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudiosphereProcessor();
}
