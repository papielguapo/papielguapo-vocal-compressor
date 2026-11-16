#pragma once
#include <chrono>
#include <algorithm>
#include <fstream>
#include <vector>

void beginAllocationMonitoring() noexcept;
unsigned long endAllocationMonitoring() noexcept;

std::vector<float> advancedRender(double rate,int hostBlock,int channels,bool automate,bool protect=true){
 FET67Processor p;set(p,"comp",-24);set(p,"gain",0);set(p,"gate",-80);set(p,"fix",protect?1:0);
 if(channels==1){auto l=p.getBusesLayout();l.inputBuses.set(0,juce::AudioChannelSet::mono());l.outputBuses.set(0,juce::AudioChannelSet::mono());require(p.setBusesLayout(l),"advanced mono");}
 p.prepareToPlay(rate,hostBlock);constexpr int total=32768;std::vector<float> output(size_t(total*channels));juce::AudioBuffer<float> b(channels,hostBlock);juce::MidiBuffer m;
 for(int pos=0;pos<total;){const int eventEnd=(pos/4096+1)*4096;const int count=juce::jmin(hostBlock,juce::jmin(total-pos,eventEnd-pos));
  if(automate&&pos%4096==0){const int step=pos/4096;set(p,"comp",step%2?-36:0);set(p,"lowCut",float((step*59)%401));set(p,"delta",float(step%3==0));set(p,"fix",float(step%2));}
  b.setSize(channels,count,false,false,true);
  for(int c=0;c<channels;++c)for(int i=0;i<count;++i){const int index=pos+i;const double t=index/rate;const float loud=protect&&index%809<3?4.f:1.f;float x=loud*(.4f*std::sin(float(2*juce::MathConstants<double>::pi*173*t))+.08f*std::cos(float(2*juce::MathConstants<double>::pi*701*t)));if(c)x*=.37f;b.setSample(c,i,x);}
  beginAllocationMonitoring();p.processBlock(b,m);const auto allocations=endAllocationMonitoring();require(allocations==0,"no C++ new allocations in audio callback");
  for(int c=0;c<channels;++c)for(int i=0;i<count;++i){const float x=b.getSample(c,i);require(std::isfinite(x),"advanced finite");require(std::abs(x)<=VocalPeakSafety::ceiling+1.e-6f,"post FIX/Delta peak ceiling");output[size_t((pos+i)*channels+c)]=x;}pos+=count;
 }
 return output;
}
void advancedValidation(){
 // Check raw-delay alignment with an impulse, not an RMS-only sine comparison.
 for(double rate:{44100.,48000.,88200.,96000.,192000.}){
  FET67Processor p;set(p,"comp",0);set(p,"gain",0);p.prepareToPlay(rate,1);juce::AudioBuffer<float>b(2,1);juce::MidiBuffer m;const int latency=p.getLatencySamples();
  for(int i=0;i<latency+20;++i){b.setSample(0,0,i==0?.5f:0);b.setSample(1,0,i==0?-.25f:0);p.processBlock(b,m);require(std::abs(b.getSample(0,0)-(i==latency?.495f:0))<1.e-6f,"impulse actual latency equals reported");}
  for(int channels:{1,2})for(int block:{1,7,64,128,512,4097})advancedRender(rate,block,channels,true);
 }
 const auto reference=advancedRender(48000,1,2,true);float maximum=0;
 for(int block:{7,64,128,512,4097}){const auto result=advancedRender(48000,block,2,true);for(size_t i=0;i<result.size();++i)maximum=juce::jmax(maximum,std::abs(result[i]-reference[i]));}
 require(maximum<2.e-5f,"automation independent of host block partition");std::cout<<"Automation partition maximum error: "<<maximum<<"\n";
 // Linked protection preserves the L/R ratio, even for supernominal input.
 VocalPeakSafety safety;safety.prepare(48000);
 for(int i=0;i<100000;++i){const float x=12*std::sin(float(i)*.071f);std::array<float,2>w{x,x*.37f},d=w;safety.tick(w,d,2);require(std::abs(w[0])<=VocalPeakSafety::ceiling&&std::abs(w[1])<=VocalPeakSafety::ceiling,"limiter absolute sample bound");require(std::abs(w[1]-.37f*w[0])<1.e-6f,"linked limiter stereo ratio");}
 // Validate the previously wrong lower edge: a genuine 1 Hz cutoff.
 {FET67Processor p;set(p,"comp",0);set(p,"gain",0);set(p,"lowCut",1);p.prepareToPlay(48000,512);juce::AudioBuffer<float>b(2,512);juce::MidiBuffer m;double energy=0;int samples=0;
  for(int pos=0;pos<48000*25;pos+=512){for(int i=0;i<512;++i){const float x=.1f*std::sin(float(2*juce::MathConstants<double>::pi*(pos+i)/48000));b.setSample(0,i,x);b.setSample(1,i,x);}p.processBlock(b,m);if(pos>48000*15){for(int i=0;i<512;++i){energy+=double(b.getSample(0,i))*b.getSample(0,i);++samples;}}}
  const double attenuation=10*std::log10(energy/samples)-20*std::log10(.099/std::sqrt(2.));require(std::abs(attenuation+3.0103)<.05,"actual 1 Hz cutoff");std::cout<<"Low Cut 1 Hz corner: "<<attenuation<<" dB\n";
 }
 std::cout<<"PASS advanced: ceiling after FIX/Delta, 5 rates, 6 block sizes, mono/stereo, automation partition, C++ allocation monitor, impulse latency, Low Cut 1 Hz\n";
}
void performanceValidation(const char* path){
 std::ofstream file(path);file<<"sample_rate,block_size,automation,mean_us,p99_us,max_us,budget_us\n";
 for(double rate:{48000.,96000.,192000.})for(int block:{64,512})for(bool automate:{false,true}){
  FET67Processor p;set(p,"comp",-24);set(p,"gain",0);set(p,"fix",1);p.prepareToPlay(rate,block);juce::AudioBuffer<float>b(2,block);juce::MidiBuffer m;std::vector<double> times;times.reserve(1000);double sum=0;
  for(int k=0;k<1200;++k){if(automate&&k%16==0)set(p,"comp",k%32?-36:0);for(int i=0;i<block;++i){const float x=.3f*std::sin(float(2*juce::MathConstants<double>::pi*173*(k*block+i)/rate));b.setSample(0,i,x);b.setSample(1,i,x*.7f);}
   const auto start=std::chrono::steady_clock::now();p.processBlock(b,m);const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();if(k>=200){times.push_back(us);sum+=us;}}
  std::sort(times.begin(),times.end());file<<rate<<","<<block<<","<<automate<<","<<sum/times.size()<<","<<times[size_t(times.size()*.99)]<<","<<times.back()<<","<<block/rate*1.e6<<"\n";
 }
 std::cout<<"Performance report: "<<path<<" (wall clock; OS scheduling included)\n";
}
