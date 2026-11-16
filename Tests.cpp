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
double filterLevel(float cutoff,double frequency,double rate,bool bypass=false) {
 FET67Processor p;set(p,"comp",0);set(p,"gain",0);set(p,"gate",-80);set(p,"lowCut",cutoff);p.prepareToPlay(rate,257);
 juce::AudioBuffer<float> b(2,257);juce::MidiBuffer m;double energy=0;int samples=0;
 for(int block=0;block<600;++block){for(int i=0;i<257;++i){const float x=.2f*std::sin(float(2*juce::MathConstants<double>::pi*frequency*(block*257+i)/rate));b.setSample(0,i,x);b.setSample(1,i,x*.5f);}
  if(bypass)p.processBlockBypassed(b,m);else p.processBlock(b,m);
  for(int i=0;i<257;++i){const float x=b.getSample(0,i);require(std::isfinite(x),"filter finite");require(std::abs(x*.5f-b.getSample(1,i))<1.e-6f,"filter stereo ratio");if(block>400){energy+=double(x)*x;++samples;}}
 }return 10*std::log10(energy/samples);
}
void buttonTests(){
 FET67Processor normal,delta,matched;
 for(auto* p:{&normal,&delta,&matched}){set(*p,"gate",-80);set(*p,"comp",0);set(*p,"gain",-12);}
 set(delta,"delta",1);set(delta,"meterMode",1);set(matched,"fix",1);
 for(auto* p:{&normal,&delta,&matched})p->prepareToPlay(48000,257);
 juce::AudioBuffer<float> a(2,257),b(2,257),c(2,257);juce::MidiBuffer m;double energy=0;int count=0;
 for(int block=0;block<1800;++block){for(int i=0;i<257;++i){const float x=.1f*std::sin(float(2*juce::MathConstants<double>::pi*440*(block*257+i)/48000.));for(int ch=0;ch<2;++ch){a.setSample(ch,i,x);b.setSample(ch,i,x);c.setSample(ch,i,x);}}
  normal.processBlock(a,m);delta.processBlock(b,m);matched.processBlock(c,m);
  for(int i=0;i<257;++i){const int sample=block*257+i-normal.getLatencySamples();const float dry=sample<0?0:.1f*std::sin(float(2*juce::MathConstants<double>::pi*440*sample/48000.));require(std::abs(a.getSample(0,i)+b.getSample(0,i)-dry)<1.e-6f,"delta reconstructs latency aligned dry");if(block>1600){energy+=double(c.getSample(0,i))*c.getSample(0,i);++count;}}
 }
 require(std::abs(10*std::log10(energy/count)-20*std::log10(.1/std::sqrt(2.)))<.1,"FIX RMS matching");
 for(int k=0;k<300;++k){c.clear();matched.processBlock(c,m);}require(c.getMagnitude(0,c.getNumSamples())==0,"FIX silence remains silent");
 juce::MemoryBlock state;matched.getStateInformation(state);FET67Processor restored;restored.setStateInformation(state.getData(),int(state.getSize()));require(restored.apvts.getRawParameterValue("fix")->load()==1,"FIX state saved");
}
void harmonicTests(){
 for(double frequency:{74.,10000.}){
  VocalHarmonics colour;colour.prepare(48000);std::array<double,7> re{},im{};double dc=0;int count=0;
  for(int i=0;i<240000;++i){const double phase=juce::MathConstants<double>::twoPi*frequency*i/48000;std::array<float,2> wet{float(std::sin(phase)),float(std::sin(phase))},dry=wet;colour.tick(wet,dry,1);
   if(i>=144000){require(std::isfinite(wet[0]),"harmonic finite");dc+=wet[0];++count;
    for(size_t h=0;h<7;++h){const double f=frequency==74?74*(h+1):(h==0?10000:18000);const double p=juce::MathConstants<double>::twoPi*f*i/48000;re[h]+=wet[0]*std::cos(p);im[h]+=wet[0]*std::sin(p);}
   }
  }
  std::array<double,7> amplitude{};for(size_t h=0;h<7;++h)amplitude[h]=2*std::hypot(re[h],im[h])/count;
  require(std::abs(dc/count)<1.e-6,"harmonic DC removal");
  if(frequency==74){require(std::abs(amplitude[1]-.02)<.001,"second harmonic level");require(std::abs(amplitude[2]-.005)<.001,"third harmonic level");for(size_t h=3;h<7;++h)require(amplitude[h]<1.e-4,"higher harmonics suppressed");}
  else require(amplitude[1]/amplitude[0]<.00032,"8x alias suppression");
 }
}
#include "AdvancedValidation.h"
int main(int argc,char**argv){juce::ScopedJuceInitialiser_GUI init;try {
 if(argc>1&&juce::String(argv[1])=="--advanced"){advancedValidation();return 0;}
 if(argc>2&&juce::String(argv[1])=="--benchmark"){performanceValidation(argv[2]);return 0;}
 if(argc>6 && juce::String(argv[1])=="--render") {FET67Processor p;set(p,"gate",juce::String(argv[4]).getFloatValue());set(p,"comp",juce::String(argv[5]).getFloatValue());set(p,"gain",juce::String(argv[6]).getFloatValue());if(argc>7)set(p,"lowCut",juce::String(argv[7]).getFloatValue());return renderAudioFile(p,juce::File(juce::String(argv[2])),juce::File(juce::String(argv[3])));}

 harmonicTests();
 buttonTests();
 for(double sr:{44100.,48000.,96000.})for(int n:{1,2}) {require(render(-80,-12,-3,0,sr,n)==0,"silence");render(-80,-36,-6,.8f,sr,n);}
 const auto open=render(-80,0,-3,.001f,48000,2);const auto closed=render(-30,0,-3,.001f,48000,2);require(closed<open*.1f,"gate attenuation");
 const auto dry=render(-80,0,0,.1f,48000,2,true);require(std::abs(dry-.0707107f)<.001f,"bypass unity");
 const auto a=render(-80,0,-12,.2f,48000,2);const auto b=render(-80,-24,-12,.2f,48000,2);require(std::abs(a-b)>.005f,"comp changes output");
 for(double sr:{44100.,48000.,96000.}){
  const double low=filterLevel(100,50,sr)-filterLevel(0,50,sr);
  const double corner=filterLevel(100,100,sr)-filterLevel(0,100,sr);
  const double high=filterLevel(100,2000,sr)-filterLevel(0,2000,sr);
  require(std::abs(low+24.1)<.3,"24 dB/oct slope");require(std::abs(corner+3.0103)<.15,"Butterworth cutoff");require(std::abs(high)<.05,"high frequency transparency");
  require(std::abs(filterLevel(400,50,sr,true)-filterLevel(0,50,sr,true))<1.e-6,"unfiltered host bypass");
 }
 {FET67Processor old;juce::MemoryBlock blob;old.getStateInformation(blob);FET67Processor newer;set(newer,"lowCut",200);auto tree=old.apvts.copyState();for(const char* id:{"lowCut","fix","delta","meterMode"})tree.removeChild(tree.getChildWithProperty("id",id),nullptr);set(newer,"fix",1);set(newer,"delta",1);set(newer,"meterMode",1);juce::MemoryBlock legacy;{juce::MemoryOutputStream stream(legacy,false);stream.write(blob.getData(),4);stream.writeInt(0);tree.createXml()->writeTo(stream,juce::XmlElement::TextFormat().singleLine());stream.writeByte(0);}static_cast<juce::uint32*>(legacy.getData())[1]=juce::ByteOrder::swapIfBigEndian(juce::uint32(legacy.getSize()-9));newer.setStateInformation(legacy.getData(),int(legacy.getSize()));require(newer.apvts.getRawParameterValue("lowCut")->load()==0,"legacy sessions lowcut off");for(const char* id:{"fix","delta","meterMode"})require(newer.apvts.getRawParameterValue(id)->load()==0,"legacy buttons off");}
 FET67Processor p;set(p,"gate",-40);set(p,"comp",-23);juce::MemoryBlock state;p.getStateInformation(state);FET67Processor q;q.setStateInformation(state.getData(),int(state.getSize()));require(std::abs(q.apvts.getRawParameterValue("comp")->load()+23)<.001f,"state comp");require(std::abs(q.apvts.getRawParameterValue("gate")->load()+40)<.001f,"state gate");
 FET67Processor stress;stress.prepareToPlay(48000,64);juce::AudioBuffer<float> buf(2,4097);juce::MidiBuffer midi;
 for(int k=0;k<30;++k){set(stress,"gate",k%2?-40:-80);set(stress,"comp",float(-k));set(stress,"lowCut",k%2?400:0);
  for(int c=0;c<2;++c)for(int i=0;i<buf.getNumSamples();++i)buf.setSample(c,i,.2f*std::sin(float(i)*.1f));
  buf.setSample(0,4,std::numeric_limits<float>::quiet_NaN());
  if(k%3==0)stress.processBlockBypassed(buf,midi);else stress.processBlock(buf,midi);
  for(int c=0;c<2;++c)for(int i=0;i<buf.getNumSamples();++i)require(std::isfinite(buf.getSample(c,i)),"stress finite");
 }
 if(argc>1){std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());auto im=e->createComponentSnapshot(e->getLocalBounds());juce::File f(juce::String::fromUTF8(argv[1]));f.deleteFile();juce::FileOutputStream out(f);juce::PNGImageFormat png;require(png.writeImageToStream(im,out),"preview");}
 std::cout<<"PASS: H2/H3, higher-order rejection, DC removal, 8x alias suppression; FIX RMS match and silence, DELTA reconstruction and meter independence; lowcut 24 dB/oct, cutoff, stereo ratio, legacy state, unfiltered bypass;  mono/stereo at 44.1/48/96k, silence, finite, bounded, stereo link, gate, comp, bypass, state\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
