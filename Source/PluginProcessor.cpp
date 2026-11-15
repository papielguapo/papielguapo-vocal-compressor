#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
float gain(float db) { return std::pow(10.0f, db * 0.05f); }
float db(float x) { return 20.0f * std::log10(juce::jmax(1.0e-9f, x)); }
// Feedback topology needs (R - 1) controller gain to produce a settled R:1 slope.
float reduction(float level, float threshold, float knee, float r)
{
    const float over = level - threshold;
    if (over <= -knee * 0.5f) return 0;
    const float slope = r - 1.0f;
    if (over >= knee * 0.5f) return juce::jmin(60.0f, slope * over);
    const float v = over + knee * 0.5f;
    return juce::jmin(60.0f, slope * v * v / (2.0f * knee));
}
}

juce::AudioProcessorValueTreeState::ParameterLayout FET67Processor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    const auto addFloat = [&p](const char* id, const char* name, float lo, float hi, float def)
    { p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>(lo, hi), def)); };
    const auto addBool = [&p](const char* id, const char* name, bool def)
    { p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id, 1}, name, def)); };
    addBool("power", "Power", true);
    addFloat("gate", "Gate (dB, -96 = off)", -96, 0, -96);
    addFloat("comp", "Comp (dB)", -36, 0, -12);
    addFloat("gain", "Gain (dB)", -36, 0, -3);
    addFloat("release", "Release (ms, 1200 = auto)", 100, 1200, 600);
    addFloat("hpf", "SC HPF (Hz, 0 = off)", 0, 220, 0);
    addBool("scAuto", "External sidechain auto", true);
    addBool("meterMode", "Meter Output", false);
    addBool("bypass", "Bypass", false);
    addBool("fix", "FIX level match / LIM", false);
    return p;
}
FET67Processor::FET67Processor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PapisCompressor", makeParameters())
{
    power = apvts.getRawParameterValue("power"); input = apvts.getRawParameterValue("comp");
    output = apvts.getRawParameterValue("gain"); release = apvts.getRawParameterValue("release");
    bypass = apvts.getRawParameterValue("bypass"); fix = apvts.getRawParameterValue("fix");
    scFilter = apvts.getRawParameterValue("scAuto");
    gate = apvts.getRawParameterValue("gate"); hpf = apvts.getRawParameterValue("hpf");

}
bool FET67Processor::isBusesLayoutSupported(const BusesLayout& l) const
{
    if (l.inputBuses.size() != 2 || l.outputBuses.size() != 1) return false;
    const auto key = l.getChannelSet(true, 1);
    return (l.getMainInputChannelSet() == juce::AudioChannelSet::mono() || l.getMainInputChannelSet() == juce::AudioChannelSet::stereo())
        && l.getMainInputChannelSet() == l.getMainOutputChannelSet()
        && (key.isDisabled() || key == juce::AudioChannelSet::mono() || key == juce::AudioChannelSet::stereo());
}
void FET67Processor::prepareToPlay(double sr, int)
{
    rate = std::isfinite(sr) ? juce::jmax(8000.0, sr) : 44100.0;
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampling->initProcessing(processingCapacity);
    keyOversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    keyOversampling->initProcessing(processingCapacity);
    engineRate = rate * static_cast<double>(oversampling->getOversamplingFactor());
    setLatencySamples(juce::roundToInt(oversampling->getLatencyInSamples()));
    engineBuffer.setSize(2, processingCapacity);
    keyBuffer.setSize(2, processingCapacity);
    dryDelay.setSize(2, getLatencySamples() + 1);
    for (auto& c : channels)
    {
        c.inputHP.pole = static_cast<float>(std::exp(-juce::MathConstants<double>::twoPi * 18.0 / engineRate));
        c.fetHP.pole = static_cast<float>(std::exp(-juce::MathConstants<double>::twoPi * 20.0 / engineRate));
        c.detectorHP.pole = static_cast<float>(std::exp(-juce::MathConstants<double>::twoPi * 80.0 / engineRate));
    }
    for (auto* smooth : { &inputGain, &outputGain, &attackPole, &releasePole, &slowReleasePole, &linkMix, &scFilterMix, &compThreshold })
        smooth->reset(engineRate, 0.02);
    wet.reset(rate, 0.02); ceilingGain.reset(rate, 0.02);
    matchMix.reset(rate, 0.02); matchGain.reset(rate, 1.0);
    matchEnergyPole = std::exp(-1.0 / (rate * 0.75));
    meterRelease = static_cast<float>(std::exp(-1.0 / (engineRate * 0.25)));
    rmsPole = static_cast<float>(std::exp(-1.0 / (rate * 0.3)));
    slowCharge = static_cast<float>(std::exp(-1.0 / (engineRate * 0.15)));
    lagPole = static_cast<float>(std::exp(-1.0 / (engineRate * 0.00018)));
    reset();
}

void FET67Processor::reset()
{
    for (auto& c : channels)
    {
        c.inputHP.clear(); c.fetHP.clear(); c.detectorHP.clear();
        c.delayed = c.fast = c.slow = c.lag = c.gr = 0;
    }
    if (oversampling != nullptr) oversampling->reset();
    if (keyOversampling != nullptr) keyOversampling->reset();
    keyBuffer.clear();
    engineBuffer.clear(); dryDelay.clear(); delayPosition = 0;
    inputGain.setCurrentAndTargetValue(1.0f);
    outputGain.setCurrentAndTargetValue(gain(output->load() - input->load() * 0.75f));
    wet.setCurrentAndTargetValue(power->load() > 0.5f && bypass->load() < 0.5f ? 1.0f : 0.0f);
    matchMix.setCurrentAndTargetValue(fix->load() > 0.5f ? 1.0f : 0.0f);
    matchGain.setCurrentAndTargetValue(1.0f);
    matchInputEnergy = matchOutputEnergy = 0; matchCounter = 0;
    currentMatchGainDb.store(0);
    const bool all = false;
    const double a = all ? 0.00045 : 0.0002;
    const double r = release->load() * 0.001;
    attackPole.setCurrentAndTargetValue(static_cast<float>(std::exp(-1.0 / (engineRate * a))));
    releasePole.setCurrentAndTargetValue(static_cast<float>(std::exp(-1.0 / (engineRate * r))));
    slowReleasePole.setCurrentAndTargetValue(static_cast<float>(std::exp(-1.0 / (engineRate * r * (all ? 5 : 3)))));
    linkMix.setCurrentAndTargetValue(1.0f);
    scFilterMix.setCurrentAndTargetValue(hpf->load() > 1 ? 1.0f : 0.0f);
    compThreshold.setCurrentAndTargetValue(input->load());
    ceilingGain.setCurrentAndTargetValue(gain(output->load()));
    gateEnvelope = gateReduction = 0; limiterGain = 1;
    rms.fill(0); grMeter = 0;
    currentGrDb.store(0); currentOutDb.store(-100);
}

void FET67Processor::processEngine(juce::dsp::AudioBlock<float>& block, const juce::dsp::AudioBlock<float>* key, int n, int mode)
{
    constexpr float ratios[] {4,8,12,20,24};
    constexpr float knees[] {4.5f,3.2f,2.4f,1.5f,7.5f};
    const bool all = mode == 4;
    const float gateAttack = static_cast<float>(std::exp(-1.0 / (engineRate * 0.001)));
    const float gateRelease = static_cast<float>(std::exp(-1.0 / (engineRate * 0.05)));
    const float gateOpen = static_cast<float>(std::exp(-1.0 / (engineRate * 0.002)));
    const float gateClose = static_cast<float>(std::exp(-1.0 / (engineRate * 0.08)));
    std::array<float*, 2> data { block.getChannelPointer(0), block.getChannelPointer(1) };
    for (size_t i = 0; i < block.getNumSamples(); ++i)
    {
        const float inG = inputGain.getNextValue(), outG = outputGain.getNextValue();
        const float threshold = compThreshold.getNextValue();
        float gatePeak = 0;
        for (int ch = 0; ch < n; ++ch) gatePeak = juce::jmax(gatePeak, std::abs(data[static_cast<size_t>(ch)][i]));
        const float envPole = (gatePeak > gateEnvelope ? gateAttack : gateRelease);
        gateEnvelope = envPole * gateEnvelope + (1-envPole) * gatePeak;
        const float gateTarget = gate->load() <= -95.9f ? 0.0f : juce::jlimit(0.0f, 80.0f, (gate->load() - db(gateEnvelope)) * 1.5f);
        const float gatePole = (gateTarget < gateReduction ? gateOpen : gateClose);
        gateReduction = gatePole * gateReduction + (1-gatePole) * gateTarget;
        const float gateGain = gain(-gateReduction);

        const float ac = attackPole.getNextValue(), rc = releasePole.getNextValue();
        const float src = slowReleasePole.getNextValue(), linked = linkMix.getNextValue();
        const float filteredMix = scFilterMix.getNextValue();
        std::array<float, 2> detection {};
        const int detectorChannels = key != nullptr ? 2 : n;
        for (int ch = 0; ch < detectorChannels; ++ch)
        {
            auto& state = channels[static_cast<size_t>(ch)];
            // Apply the previous gain-cell attenuation to the external key:
            // preserve feedback controller calibration rather than using R-1 feed-forward.
            const float signal = key != nullptr ? key->getSample(ch, i) * inG
                * gain(-channels[static_cast<size_t>(n == 1 ? 0 : ch)].gr) : state.delayed;
            const float filtered = state.detectorHP.tick(signal);
            const float raw = std::abs(signal);
            detection[static_cast<size_t>(ch)] = raw + filteredMix * (std::abs(filtered) - raw);
        }
        const float maxDetector = juce::jmax(detection[0], detection[1]);
        float maxGr = 0;
        for (int ch = 0; ch < n; ++ch)
        {
            auto& c = channels[static_cast<size_t>(ch)];
            float x = juce::jlimit(-64.0f, 64.0f, data[static_cast<size_t>(ch)][i] * inG * gateGain);
            // Clean voicing: more headroom before the feedback gain cell.
            constexpr float ironDrive = 0.015f;
            x = c.inputHP.tick(std::tanh(x * ironDrive) / ironDrive);
            float detector = key != nullptr && n == 1 ? maxDetector
                : detection[static_cast<size_t>(ch)] + linked * (maxDetector - detection[static_cast<size_t>(ch)]);
            if (all) { c.lag = lagPole * c.lag + (1 - lagPole) * detector; detector = c.lag; }
            const float target = reduction(db(detector), threshold, knees[mode], ratios[mode]);
            const float fastPole = target > c.fast ? ac : rc;
            c.fast = fastPole * c.fast + (1 - fastPole) * target;
            const float slowTarget = c.fast * juce::jlimit(0.0f, 0.75f, (c.fast - 3.0f) / 24.0f);
            const float sp = slowTarget > c.slow ? slowCharge : src;
            c.slow = sp * c.slow + (1 - sp) * slowTarget;
            c.gr = juce::jlimit(0.0f, 60.0f, juce::jmax(c.fast, c.slow));
            x *= gain(-c.gr); c.delayed = x;
            const float drive = juce::jmin(1.0f, c.gr / 24.0f);
            // Retain a trace of bounded FET colour without the previous heavy H2.
            const float even = 0.015f * drive * x * x / (1 + x * x);
            const float fet = x + even - 0.001f * drive * x * x * x / (1 + x * x);
            x = c.fetHP.tick(fet) * outG;
            // Output safety saturation starts at 0 dBFS, rather than -4 dBFS.
            // This is not a true-peak limiter; leave output headroom in the DAW.
            constexpr float knee = 1.0f;
            if (std::abs(x) > knee)
                x = std::copysign(knee + 0.25f * std::tanh((std::abs(x) - knee) / 0.25f), x);
            data[static_cast<size_t>(ch)][i] = x;
            maxGr = juce::jmax(maxGr, c.gr);
        }
        grMeter = juce::jmax(maxGr + gateReduction, grMeter * meterRelease);
    }
}

void FET67Processor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
{
    process(b, midi, false);
}

void FET67Processor::processBlockBypassed(juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
{
    // Keep the same engine and delay histories running in both callbacks.
    // Host bypass and the panel switch share the same latency-aligned fade.
    process(b, midi, true);
}

void FET67Processor::process(juce::AudioBuffer<float>& b, juce::MidiBuffer& midi, bool hostBypassed)
{
    juce::ScopedNoDenormals guard;
    midi.clear();
    auto main = getBusBuffer(b, false, 0);
    const int n = juce::jmin(2, main.getNumChannels());
    if (n == 0 || oversampling == nullptr) return;
    const bool external = scFilter->load() > 0.5f && getBus(true, 1)->isEnabled();
    for (auto& c : channels) c.detectorHP.pole = static_cast<float>(std::exp(-juce::MathConstants<double>::twoPi * juce::jmax(1.0f, hpf->load()) / engineRate));
    compThreshold.setTargetValue(input->load());
    ceilingGain.setTargetValue(gain(output->load()));
    auto key = getBusBuffer(b, true, 1);
    inputGain.setTargetValue(1.0f);
    outputGain.setTargetValue(gain(output->load() - input->load() * 0.75f));
    linkMix.setTargetValue(1.0f);
    scFilterMix.setTargetValue(hpf->load() > 1 ? 1.0f : 0.0f);
    const bool active = !hostBypassed && power->load() > 0.5f && bypass->load() < 0.5f;
    wet.setTargetValue(active ? 1.0f : 0.0f);
    matchMix.setTargetValue(fix->load() > 0.5f ? 1.0f : 0.0f);
    const int mode = 0;
    const bool all = mode == 4;
    const double at = all ? 0.00045 : 0.0002;
    const double rt = release->load() >= 1199 ? 0.12 + 0.045 * juce::jmin(20.0f, grMeter) : release->load() * 0.001;
    attackPole.setTargetValue(static_cast<float>(std::exp(-1.0 / (engineRate * at))));
    releasePole.setTargetValue(static_cast<float>(std::exp(-1.0 / (engineRate * rt))));
    slowReleasePole.setTargetValue(static_cast<float>(std::exp(-1.0 / (engineRate * rt * (all ? 5 : 3)))));
    const int delayLength = dryDelay.getNumSamples();
    for (int offset = 0; offset < b.getNumSamples(); offset += processingCapacity)
    {
        const int count = juce::jmin(processingCapacity, b.getNumSamples() - offset);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* dest = engineBuffer.getWritePointer(ch);
            if (ch >= n) { juce::FloatVectorOperations::clear(dest, count); continue; }
            const auto* source = main.getReadPointer(ch, offset);
            for (int i = 0; i < count; ++i)
                dest[i] = std::isfinite(source[i]) ? juce::jlimit(-64.0f, 64.0f, source[i]) : 0.0f;
        }
        auto baseBlock = juce::dsp::AudioBlock<float>(engineBuffer).getSubBlock(0, static_cast<size_t>(count));
        auto upBlock = oversampling->processSamplesUp(juce::dsp::AudioBlock<const float>(baseBlock));
        if (external && key.getNumChannels() > 0)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto* source = key.getReadPointer(juce::jmin(ch, key.getNumChannels() - 1), offset);
                auto* dest = keyBuffer.getWritePointer(ch);
                for (int i = 0; i < count; ++i)
                    dest[i] = std::isfinite(source[i]) ? juce::jlimit(-64.0f, 64.0f, source[i]) : 0.0f;
            }
            auto keyBase = juce::dsp::AudioBlock<float>(keyBuffer).getSubBlock(0, static_cast<size_t>(count));
            auto keyUp = keyOversampling->processSamplesUp(juce::dsp::AudioBlock<const float>(keyBase));
            processEngine(upBlock, &keyUp, n, mode);
        }
        else processEngine(upBlock, nullptr, n, mode);
        oversampling->processSamplesDown(baseBlock);
        for (int i = 0; i < count; ++i)
        {
            const float mix = wet.getNextValue();
            const float matching = matchMix.getNextValue();
            const float gainMatch = 1.0f + matching * (matchGain.getNextValue() - 1.0f);
            float peak = 0;
            for (int ch = 0; ch < n; ++ch) peak = juce::jmax(peak, std::abs(engineBuffer.getSample(ch, i) * gainMatch));
            const float ceiling = ceilingGain.getNextValue();
            const float required = peak > ceiling ? ceiling / peak : 1.0f;
            const float recovery = static_cast<float>(std::exp(-1.0 / (rate * 0.05)));
            limiterGain = required < limiterGain ? required : recovery * limiterGain + (1-recovery) * required;
            double inputEnergy = 0, outputEnergy = 0;
            const int readPosition = (delayPosition + 1) % delayLength;
            for (int ch = 0; ch < n; ++ch)
            {
                auto* target = main.getWritePointer(ch, offset);
                const float dry = std::isfinite(target[i]) ? target[i] : 0.0f;
                dryDelay.setSample(ch, delayPosition, dry);
                const float delayedDry = dryDelay.getSample(ch, readPosition);
                const float processed = engineBuffer.getSample(ch, i);
                inputEnergy += static_cast<double>(delayedDry) * delayedDry;
                outputEnergy += static_cast<double>(processed) * processed;
                const float matched = processed * gainMatch * limiterGain;
                target[i] = mix <= 0 ? delayedDry : mix >= 1 ? matched : delayedDry + mix * (matched - delayedDry);
                auto& channelRms = rms[static_cast<size_t>(ch)];
                const double level = target[i];
                channelRms = rmsPole * channelRms + (1 - rmsPole) * level * level;
            }
            matchInputEnergy = matchEnergyPole * matchInputEnergy + (1 - matchEnergyPole) * (inputEnergy / n);
            matchOutputEnergy = matchEnergyPole * matchOutputEnergy + (1 - matchEnergyPole) * (outputEnergy / n);
            if (++matchCounter >= 64)
            {
                matchCounter = 0;
                // Learn before compensation; use one gain for both channels. Hold
                // through silence and clamp to +/-24 dB to avoid runaway boosts.
                if (matchInputEnergy > 1.0e-6 && matchOutputEnergy > 1.0e-8)
                {
                    const double desiredDb = juce::jlimit(-24.0, 24.0, 10.0 * std::log10(matchInputEnergy / matchOutputEnergy));
                    matchGain.setTargetValue(gain(static_cast<float>(desiredDb)));
                }
            }
            delayPosition = readPosition;
        }
    }
    currentGrDb.store(grMeter * wet.getCurrentValue(), std::memory_order_relaxed);
    currentMatchGainDb.store(db(1.0f + matchMix.getCurrentValue() * (matchGain.getCurrentValue() - 1.0f)) * wet.getCurrentValue(), std::memory_order_relaxed);
    const double maxRms = n == 1 ? rms[0] : juce::jmax(rms[0], rms[1]);
    currentOutDb.store(static_cast<float>(juce::jmax(-100.0, 10.0 * std::log10(juce::jmax(1.0e-10, maxRms)))), std::memory_order_relaxed);
}
juce::AudioProcessorParameter* FET67Processor::getBypassParameter() const { return apvts.getParameter("bypass"); }
void FET67Processor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    // APVTS snaps boolean values, while a VST3 controller may retain fractional
    // normalized automation. Preserve that host representation as well.
    if (auto oldValues = state.getChildWithName("HOST_NORMALIZED"); oldValues.isValid())
        state.removeChild(oldValues, nullptr);
    juce::ValueTree normalized("HOST_NORMALIZED");
    for (auto* parameter : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter))
        {
            juce::ValueTree value("VALUE");
            value.setProperty("id", ranged->getParameterID(), nullptr);
            value.setProperty("value", ranged->getValue(), nullptr);
            normalized.addChild(value, -1, nullptr);
        }
    state.addChild(normalized, -1, nullptr);
    state.setProperty("editorWidth", editorWidth.load(), nullptr);
    state.setProperty("editorHeight", editorHeight.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}
void FET67Processor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml(*xml);
            const auto normalized = state.getChildWithName("HOST_NORMALIZED");
            if (normalized.isValid()) state.removeChild(normalized, nullptr);
            editorWidth.store(juce::jlimit(817, 4085, static_cast<int>(state.getProperty("editorWidth", 1634))));
            editorHeight.store(juce::jlimit(126, 630, static_cast<int>(state.getProperty("editorHeight", 252))));
            apvts.replaceState(state);
            for (int i = 0; i < normalized.getNumChildren(); ++i)
            {
                const auto value = normalized.getChild(i);
                if (auto* parameter = apvts.getParameter(value.getProperty("id").toString()))
                {
                    const float v = static_cast<float>(value.getProperty("value", parameter->getDefaultValue()));
                    if (std::isfinite(v)) parameter->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, v));
                }
            }
        }
}
juce::AudioProcessorEditor* FET67Processor::createEditor() { return new FET67Editor(*this); }



juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FET67Processor(); }
