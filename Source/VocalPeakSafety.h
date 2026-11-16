#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <cstdint>

// Stereo-linked, 1 ms lookahead sample-peak protection. This is not true peak.
class VocalPeakSafety {
public:
 static constexpr float ceiling=.966050879f; // -0.3 dBFS
 void prepare(double rate){lookahead=juce::jmax(1,juce::roundToInt(rate*.001));capacity=lookahead+2;
  for(auto& c:wetDelay)c.assign(size_t(lookahead+1),0);for(auto& c:dryDelay)c.assign(size_t(lookahead+1),0);
  queueValues.resize(size_t(capacity));queueTimes.resize(size_t(capacity));attack=std::exp(-1/(rate*.0001));release=std::exp(-1/(rate*.05));reset();}
 int latency() const noexcept{return lookahead;}
 void reset(){for(auto& c:wetDelay)std::fill(c.begin(),c.end(),0.f);for(auto& c:dryDelay)std::fill(c.begin(),c.end(),0.f);position=head=tail=0;frame=0;envelope=1;}
 float tick(std::array<float,2>& wet,std::array<float,2>& dry,int channels) noexcept {
  float peak=0;for(int c=0;c<channels;++c)peak=juce::jmax(peak,std::abs(wet[size_t(c)]));
  while(head!=tail&&queueTimes[size_t(head)]+uint64_t(lookahead)<frame)head=(head+1)%capacity;
  while(head!=tail){const int last=(tail+capacity-1)%capacity;if(queueValues[size_t(last)]>peak)break;tail=last;}
  queueValues[size_t(tail)]=peak;queueTimes[size_t(tail)]=frame;tail=(tail+1)%capacity;
  const float maximum=queueValues[size_t(head)],desired=maximum>ceiling?ceiling/maximum:1.f;
  const float pole=desired<envelope?attack:release;envelope=pole*envelope+(1-pole)*desired;
  const int read=(position+1)%(lookahead+1);float delayedPeak=0;
  for(int c=0;c<channels;++c){wetDelay[size_t(c)][size_t(position)]=wet[size_t(c)];dryDelay[size_t(c)][size_t(position)]=dry[size_t(c)];wet[size_t(c)]=wetDelay[size_t(c)][size_t(read)];dry[size_t(c)]=dryDelay[size_t(c)][size_t(read)];delayedPeak=juce::jmax(delayedPeak,std::abs(wet[size_t(c)]));}
  const float applied=juce::jmin(envelope,delayedPeak>ceiling?ceiling/delayedPeak:1.f);
  for(int c=0;c<channels;++c)wet[size_t(c)]=juce::jlimit(-ceiling,ceiling,wet[size_t(c)]*applied);
  position=read;++frame;return -20*std::log10(juce::jmax(1.e-12f,applied));
 }
private:
 std::array<std::vector<float>,2> wetDelay,dryDelay;
 std::vector<float> queueValues;std::vector<uint64_t> queueTimes;
 int lookahead=1,capacity=3,position=0,head=0,tail=0;uint64_t frame=0;
 float envelope=1,attack=0,release=0;
};

class VocalRollingPeak {
public:
 void prepare(int length){window=length;capacity=length+1;values.resize(size_t(capacity));times.resize(size_t(capacity));reset();}
 void reset() noexcept{head=tail=0;frame=0;}
 float tick(float x) noexcept{while(head!=tail&&times[size_t(head)]+uint64_t(window)<=frame)head=(head+1)%capacity;
  while(head!=tail){const int last=(tail+capacity-1)%capacity;if(values[size_t(last)]>x)break;tail=last;}
  values[size_t(tail)]=x;times[size_t(tail)]=frame++;tail=(tail+1)%capacity;return values[size_t(head)];}
private:
 std::vector<float> values;std::vector<uint64_t> times;int window=1,capacity=2,head=0,tail=0;uint64_t frame=0;
};
