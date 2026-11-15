#include "Source/PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <limits>
void set(FET67Processor& p,const char* id,float v) {auto* a=p.apvts.getParameter(id);a->setValueNotifyingHost(a->convertTo0to1(v));}
void require(bool x,const char* m) {if(!x)throw std::runtime_error(m);}
#include "OfflineRender.h"
float render(float gate,float comp,float gain,float amplitude,double sr,int channels,bool bypass=false) {
 FET67Processor p; set(p,"gate",gate);set(p,"comp",comp);set(p,"gain",gain);
 if(channels==1) {auto l=p.getBusesLayout();l.inputBuses.set(0,juce::AudioChannelSet::mono());l.outputBuses.set(0,juce::AudioChannelSet::mono());require(p.setBusesLayout(l),"mono layout");}
 p.prepareToPlay(sr,257);juce::AudioBuffer<float>b(channels,257);juce::MidiBuffer m;double energy=0;int count=0;
 for(int k=0;k<350;++k){for(int c=0;c<channels;++c)for(int i=0;i<257;++i)b.setSample(c,i,amplitude*std::sin(float(2*juce::MathConstants<double>::pi*440*(k*257+i)/sr)));
 if(bypass)p.processBlockBypassed(b,m);else p.processBlock(b,m);
 for(int i=0;i<257;++i){float x=b.getSample(0,i);require(std::isfinite(x),"finite output");if(!bypass)require(std::abs(x)<64.f,"bounded output");
 if(channels==2)require(std::abs(x-b.getSample(1,i))<0.00001f,"stereo link");if(k>250){energy+=x*x;++count;}}
 }return float(std::sqrt(energy/count));
}
int main(int argc,char**argv){juce::ScopedJuceInitialiser_GUI init;try {
 if(argc>6 && juce::String(argv[1])=="--render") {FET67Processor p;set(p,"gate",juce::String(argv[4]).getFloatValue());set(p,"comp",juce::String(argv[5]).getFloatValue());set(p,"gain",juce::String(argv[6]).getFloatValue());return renderAudioFile(p,juce::File(juce::String(argv[2])),juce::File(juce::String(argv[3])));}

 for(double sr:{44100.,48000.,96000.})for(int n:{1,2}) {require(render(-80,-12,-3,0,sr,n)==0,"silence");render(-80,-36,-6,.8f,sr,n);}
 const auto open=render(-80,0,-3,.001f,48000,2);const auto closed=render(-30,0,-3,.001f,48000,2);require(closed<open*.1f,"gate attenuation");
 const auto dry=render(-80,0,0,.1f,48000,2,true);require(std::abs(dry-.0707107f)<.001f,"bypass unity");
 const auto a=render(-80,0,-12,.2f,48000,2);const auto b=render(-80,-24,-12,.2f,48000,2);require(std::abs(a-b)>.005f,"comp changes output");
 FET67Processor p;set(p,"gate",-40);set(p,"comp",-23);juce::MemoryBlock state;p.getStateInformation(state);FET67Processor q;q.setStateInformation(state.getData(),int(state.getSize()));require(std::abs(q.apvts.getRawParameterValue("comp")->load()+23)<.001f,"state comp");require(std::abs(q.apvts.getRawParameterValue("gate")->load()+40)<.001f,"state gate");
 FET67Processor stress;stress.prepareToPlay(48000,64);juce::AudioBuffer<float> buf(2,4097);juce::MidiBuffer midi;
 for(int k=0;k<30;++k){set(stress,"gate",k%2?-40:-80);set(stress,"comp",float(-k));
  for(int c=0;c<2;++c)for(int i=0;i<buf.getNumSamples();++i)buf.setSample(c,i,.2f*std::sin(float(i)*.1f));
  buf.setSample(0,4,std::numeric_limits<float>::quiet_NaN());
  if(k%3==0)stress.processBlockBypassed(buf,midi);else stress.processBlock(buf,midi);
  for(int c=0;c<2;++c)for(int i=0;i<buf.getNumSamples();++i)require(std::isfinite(buf.getSample(c,i)),"stress finite");
 }
 if(argc>1){std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());auto im=e->createComponentSnapshot(e->getLocalBounds());juce::File f(juce::String::fromUTF8(argv[1]));f.deleteFile();juce::FileOutputStream out(f);juce::PNGImageFormat png;require(png.writeImageToStream(im,out),"preview");}
 std::cout<<"PASS: mono/stereo at 44.1/48/96k, silence, finite, bounded, stereo link, gate, comp, bypass, state\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
