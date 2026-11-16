#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <cmath>

// Three cascaded boxcar averages in the log-gain domain. Positive FIR weights
// suppress periodic gain modulation without ringing or negative linear gains.
class CleanGainMemory {
public:
 void prepare(double rate) { size=juce::jmax(2,juce::roundToInt(rate*.016/3)+1); }
 int latency() const noexcept { return 3*(size-1); }
 void reset(float value) {position=0;for(size_t s=0;s<3;++s){history[s].assign(size_t(size),value);sums[s]=double(size)*value;}}
 float tick(float value) noexcept {double v=value;for(size_t s=0;s<3;++s){auto& old=history[s][size_t(position)];sums[s]+=v-old;old=v;v=sums[s]/size;}position=(position+1)%size;return float(v);}
private:
 int size=2,position=0;
 std::array<std::vector<double>,3> history;
 std::array<double,3> sums{};
};

// Oversample only the nonlinear correction. The main audio is delayed but never
// filtered by the oversampler: with zero colour, the linear signal stays exact.
class VocalHarmonics {
public:
 void prepare(double rate) {
  oversampler=std::make_unique<juce::dsp::Oversampling<float>>(2,3,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true);
  oversampler->initProcessing(1);delay=juce::roundToInt(oversampler->getLatencyInSamples());
  for(auto& channel:linearDelay)channel.assign(size_t(delay+1),0);
  for(auto& channel:dryDelay)channel.assign(size_t(delay+1),0);
  amount.reset(rate*8,.02);dcG=std::tan(juce::MathConstants<double>::pi*20/(rate*8));reset();
 }
 int latency() const noexcept {return delay;}
 void reset() {if(oversampler)oversampler->reset();for(auto& c:linearDelay)std::fill(c.begin(),c.end(),0.f);for(auto& c:dryDelay)std::fill(c.begin(),c.end(),0.f);position=0;for(auto& c:dcState1)c.fill(0);for(auto& c:dcState2)c.fill(0);amount.setCurrentAndTargetValue(0);}
 void tick(std::array<float,2>& wet,std::array<float,2>& dry,float target) noexcept {
  float left=wet[0],right=wet[1];float* channels[]{&left,&right};juce::dsp::AudioBlock<float> base(channels,2,1);
  auto up=oversampler->processSamplesUp(juce::dsp::AudioBlock<const float>(base));amount.setTargetValue(juce::jlimit(0.f,1.f,target));
  for(size_t i=0;i<up.getNumSamples();++i){const float colour=amount.getNextValue();
   for(size_t c=0;c<2;++c){const float x=up.getSample(int(c),int(i)),a=std::abs(x);const float squared=a<=2?x*x:4+4*(a-2);const float cubed=a<=2?x*x*x:std::copysign(8+12*(a-2),x);
    // At unit sine amplitude and full colour: H2 about -34 dBc, H3 about -46 dBc.
    const float correction=colour*(.04f*squared-.02f*cubed);
    double ac=correction;constexpr double damping[]{1.8477590650225735,.7653668647301795};
    for(size_t s=0;s<2;++s){const double k=damping[s],a1=1/(1+dcG*(dcG+k));auto& z1=dcState1[c][s];auto& z2=dcState2[c][s];const double v1=(z1+dcG*(ac-z2))*a1,v2=z2+dcG*v1;z1=2*v1-z1;z2=2*v2-z2;ac=ac-k*v1-v2;}
    up.setSample(int(c),int(i),float(ac));
   }
  }
  oversampler->processSamplesDown(base);const int read=(position+1)%(delay+1);
  for(size_t c=0;c<2;++c){linearDelay[c][size_t(position)]=wet[c];dryDelay[c][size_t(position)]=dry[c];wet[c]=linearDelay[c][size_t(read)]+(c==0?left:right);dry[c]=dryDelay[c][size_t(read)];}position=read;
 }
private:
 std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
 std::array<std::vector<float>,2> linearDelay,dryDelay;
 std::array<std::array<double,2>,2> dcState1{},dcState2{};
 juce::SmoothedValue<float> amount;
 double dcG=0;
 int delay=0,position=0;
};
