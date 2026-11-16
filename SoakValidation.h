#pragma once
#include <cstdint>

// Streaming stress: no multi-minute audio buffers and no disk I/O in processBlock.
void soakValidation(const char* path){
 std::ofstream report(path);require(report.good(),"soak report opens");
 report<<"sample_rate,audio_seconds,callbacks,peak,cpp_allocations,wall_seconds\n";
 uint32_t random=20261116;auto next=[&](){random=1664525u*random+1013904223u;return float(random>>8)/16777216.f;};
 const char* ids[]{"gate","comp","gain","power","bypass","lowCut","delta","meterMode","fix"};
 for(double rate:{48000.,96000.,192000.}){
  const int seconds=rate==48000?1800:rate==96000?600:300;
  FET67Processor p;p.prepareToPlay(rate,512);juce::AudioBuffer<float> b(2,4097);juce::MidiBuffer midi;
  const int sizes[]{1,7,64,128,512,4097};int64_t pos=0,callbacks=0;unsigned long allocations=0;float peak=0;
  const int64_t total=int64_t(rate)*seconds;int64_t nextEvent=0,nextState=int64_t(rate)*30,nextProgress=int64_t(rate)*60;double phase=0;
  const auto start=std::chrono::steady_clock::now();
  while(pos<total){
   if(pos>=nextEvent){set(p,"gate",-80+70*next());set(p,"comp",-36*next());set(p,"gain",-36*next());set(p,"lowCut",next()<.2f?0:400*next());
    for(const char* id:{"power","bypass","delta","meterMode","fix"})set(p,id,next()>.5f?1:0);
    nextEvent=pos+int64_t(rate*(.1+next()*1.9));}
   if(pos>=nextState){juce::MemoryBlock state;p.getStateInformation(state);FET67Processor restored;restored.setStateInformation(state.getData(),int(state.getSize()));
    for(const auto* id:ids)require(p.apvts.getRawParameterValue(id)->load()==restored.apvts.getRawParameterValue(id)->load(),"soak state equality");nextState+=int64_t(rate)*30;}
   const int count=int(juce::jmin<int64_t>(sizes[(callbacks/32)%6],total-pos));b.setSize(2,count,false,false,true);
   const float envelope=float(.02+.6*(.5+.5*std::sin(double(pos)/rate*2.3)));const bool silence=(int(pos/rate)%17)==0;
   for(int i=0;i<count;++i){phase+=2*juce::MathConstants<double>::pi*(110+60*std::sin(double(pos+i)/rate*.3))/rate;if(phase>2*juce::MathConstants<double>::pi)phase-=2*juce::MathConstants<double>::pi;
    const float x=silence?0:envelope*(float(std::sin(phase))+.25f*float(std::sin(3*phase)))+.005f*(next()-.5f);b.setSample(0,i,x);b.setSample(1,i,.37f*x);}
   if(callbacks%997==0){b.setSample(0,0,std::numeric_limits<float>::quiet_NaN());b.setSample(1,0,std::numeric_limits<float>::infinity());}
   if(callbacks%991==0){b.setSample(0,count-1,12);b.setSample(1,count-1,-3);}
   beginAllocationMonitoring();p.processBlock(b,midi);allocations+=endAllocationMonitoring();
   for(int c=0;c<2;++c)for(int i=0;i<count;++i){const float x=b.getSample(c,i);require(std::isfinite(x)&&std::abs(x)<=64,"soak finite bounded including bypass");peak=juce::jmax(peak,std::abs(x));}
   pos+=count;++callbacks;
   if(pos>=nextProgress){std::cout<<"Soak "<<rate<<" Hz: "<<int(pos/rate)<<" / "<<seconds<<" audio seconds\n"<<std::flush;nextProgress+=int64_t(rate)*60;}
  }
  require(allocations==0,"soak callback C++ allocations");const double wall=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  report<<rate<<","<<seconds<<","<<callbacks<<","<<peak<<","<<allocations<<","<<wall<<"\n";report.flush();
 }
 std::cout<<"PASS soak: 45 minutes of simulated audio across three rates; automation, malformed input, variable blocks, state cycles, finite output, zero callback C++ new allocations\n";
}
