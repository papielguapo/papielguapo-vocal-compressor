#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "VocalHarmonics.h"
class FET67Processor final : public juce::AudioProcessor {
public:
 FET67Processor();
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 void reset() override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorParameter* getBypassParameter() const override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "PAPIELGUAPO - VOCAL Compressor";}
 bool acceptsMidi() const override {return false;}
 bool producesMidi() const override {return false;}
 bool isMidiEffect() const override {return false;}
 double getTailLengthSeconds() const override {return .25;}
 int getNumPrograms() override {return 1;}
 int getCurrentProgram() override {return 0;}
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override {return "Default";}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
 juce::AudioProcessorValueTreeState apvts;
 std::atomic<float> currentGrDb{0},currentOutDb{-100};
 std::atomic<int> editorWidth{1634},editorHeight{252};
private:
 void process(juce::AudioBuffer<float>&,juce::MidiBuffer&,bool);
 std::atomic<float> *gate,*comp,*output,*power,*bypass,*lowCut,*fix,*delta;
 CleanGainMemory cleanGain;
 VocalHarmonics harmonics;
 std::array<std::vector<float>,2> cleanDryDelay,cleanWetDelay;
 int cleanPosition=0,cleanLatency=0;
 double rate=48000;
 static constexpr int maximumDelay=512;
 int delay=64;
 std::array<std::array<float,maximumDelay+1>,2> audioDelay{},filteredDelay{};
 std::array<std::array<float,2>,2> cutState1{},cutState2{};
 std::array<float,maximumDelay> gainHistory{},kernel{},peakHistory{};
 int audioPosition=0,gainPosition=0,overshootHold=0;
 float overloadMemory=0;
 float memoryGain=1,primaryGain=1,recoveryPole=0,releaseStartGain=1,releaseFraction=0;
 bool recovering=false;
 float gateEnvelope=0,previousPeak=0,fastReduction=0,slowReduction=0,overshootReduction=0,limiterGain=1;
 float gatePole=0,limiterPole=0,meterPole=0,shownReduction=0;
 juce::SmoothedValue<float> compression,gateThreshold,outputGain,wet,cutFrequency,cutMix,fixMix,deltaMix,matchGain;
 double matchInput=0,matchOutput=0;
 int matchCounter=0;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FET67Processor)
};
