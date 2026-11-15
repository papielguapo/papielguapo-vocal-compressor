#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class FET67Processor final : public juce::AudioProcessor
{
public:
    FET67Processor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Papis Compressor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> currentGrDb { 0.0f }, currentOutDb { -100.0f };
    std::atomic<float> currentMatchGainDb { 0.0f };
    std::atomic<int> editorWidth { 1634 }, editorHeight { 252 };
private:
    struct HighPass
    {
        float previousInput = 0, previousOutput = 0, pole = 0;
        float tick(float x) noexcept { const float y = pole * (previousOutput + x - previousInput); previousInput = x; previousOutput = y; return y; }
        void clear() noexcept { previousInput = previousOutput = 0; }
    };
    struct Channel
    {
        HighPass inputHP, fetHP, detectorHP;
        float delayed = 0, fast = 0, slow = 0, lag = 0, gr = 0;
    };
    std::array<Channel, 2> channels;
    std::atomic<float> *power, *input, *output, *release, *gate, *hpf, *bypass, *fix, *scFilter;
    void process(juce::AudioBuffer<float>&, juce::MidiBuffer&, bool hostBypassed);
    void processEngine(juce::dsp::AudioBlock<float>&, const juce::dsp::AudioBlock<float>*, int channelsToProcess, int mode);
    juce::SmoothedValue<float> inputGain, outputGain, wet, attackPole, releasePole, slowReleasePole, linkMix, scFilterMix;
    juce::SmoothedValue<float> matchGain, matchMix, compThreshold, ceilingGain;
    float gateEnvelope = 0, gateReduction = 0, limiterGain = 1;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling, keyOversampling;
    juce::AudioBuffer<float> engineBuffer, keyBuffer, dryDelay;
    static constexpr int processingCapacity = 512;
    int delayPosition = 0;
    double rate = 44100;
    double engineRate = 352800;
    std::array<double, 2> rms {};
    double matchInputEnergy = 0, matchOutputEnergy = 0, matchEnergyPole = 0;
    int matchCounter = 0;
    float grMeter = 0, meterRelease = 0, rmsPole = 0, slowCharge = 0, lagPole = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FET67Processor)
};
