#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
float gain(float db) {return std::pow(10.0f,db*.05f);}
float db(float x) {return 20*std::log10(juce::jmax(1.e-12f,x));}
// Independent gain-domain recovery model fitted to measured 20/50/100/200/500/1500 ms
// DC bursts (.01 -> .5 -> .01) at 48 kHz. No Waves code is embedded.
struct Recovery {float base,power,charge,release;};
Recovery recovery(float compression) {
 constexpr Recovery table[]={{0.004034180633635883f,7.0280093041312f,0.24511765456719972f,0.4959159723894418f},{0.004034180633635883f,7.0280093041312f,0.24511765456719972f,0.4959159723894418f},{0.027606434783344194f,4.526350737389791f,0.24143505740344678f,0.47730148971969033f},{0.05677329123017288f,4.868006942538037f,0.23336251002428027f,0.4571293161489534f},{0.08210115453235783f,4.923619112630056f,0.22285411990287096f,0.4396038264741332f},{0.08879017856974407f,3.6222144384578048f,0.21803881038180142f,0.4436710569885099f},{0.11057982710398628f,3.4358258385335887f,0.18621155209185922f,0.41834815752611293f}};
 const float p=juce::jlimit(0.f,6.f,-compression/6);const int i=juce::jmin(5,int(p));const float f=p-i;
 const auto a=table[i],b=table[i+1];return {a.base+f*(b.base-a.base),a.power+f*(b.power-a.power),a.charge+f*(b.charge-a.charge),a.release+f*(b.release-a.release)};
}
}
juce::AudioProcessorValueTreeState::ParameterLayout FET67Processor::makeParameters() {
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 auto add=[&](const char* id,const char* name,float lo,float hi,float value){p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi),value));};
 add("gate","Gate (dB, -80 = off)",-80,0,-80);add("comp","Comp (dB)",-36,0,-12);add("gain","Gain (dB)",-36,0,-3);
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"power",1},"Power",true));
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));add("lowCut","Low Cut (Hz, 0 = off)",0,400,0);
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"delta",1},"Delta",false));
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"meterMode",1},"Meter Output",false));
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"fix",1},"FIX Auto Gain",false));return p;
}
FET67Processor::FET67Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),apvts(*this,nullptr,"PapisCompressor",makeParameters()) {
 gate=apvts.getRawParameterValue("gate");comp=apvts.getRawParameterValue("comp");output=apvts.getRawParameterValue("gain");power=apvts.getRawParameterValue("power");bypass=apvts.getRawParameterValue("bypass");lowCut=apvts.getRawParameterValue("lowCut");fix=apvts.getRawParameterValue("fix");delta=apvts.getRawParameterValue("delta");
}
bool FET67Processor::isBusesLayoutSupported(const BusesLayout& l) const {return l.inputBuses.size()==1&&l.outputBuses.size()==1&&(l.getMainInputChannelSet()==juce::AudioChannelSet::mono()||l.getMainInputChannelSet()==juce::AudioChannelSet::stereo())&&l.getMainInputChannelSet()==l.getMainOutputChannelSet();}
void FET67Processor::prepareToPlay(double sr,int) {
 rate=std::isfinite(sr)?juce::jmax(8000.,sr):48000.;
 delay=64;while(rate>48000.0*(delay/64)&&delay<maximumDelay)delay*=2;setLatencySamples(delay);
 gatePole=std::exp(-1./(rate*.13));limiterPole=std::exp(-1./(rate*.05));meterPole=std::exp(-1./(rate*.25));
 for(auto* s:{&compression,&gateThreshold,&outputGain,&wet,&cutFrequency,&cutMix,&fixMix,&deltaMix})s->reset(rate,.02);
 matchGain.reset(rate,1.0);kernel.fill(0);float sum=0;for(int i=0;i<delay;++i){kernel[size_t(i)]=std::sin(juce::MathConstants<float>::pi*(i+.5f)/delay);sum+=kernel[size_t(i)];}for(auto& k:kernel)k/=sum;reset();
}
void FET67Processor::reset() {
 for(auto& c:filteredDelay)c.fill(0);for(auto& c:cutState1)c.fill(0);for(auto& c:cutState2)c.fill(0);
 matchInput=matchOutput=0;matchCounter=0;matchGain.setCurrentAndTargetValue(1);fixMix.setCurrentAndTargetValue(fix->load()>.5f?1.f:0.f);deltaMix.setCurrentAndTargetValue(delta->load()>.5f?1.f:0.f);
 cutFrequency.setCurrentAndTargetValue(juce::jmax(5.f,lowCut->load()));cutMix.setCurrentAndTargetValue(lowCut->load()>0?1.f:0.f);
 for(auto& channel:audioDelay)channel.fill(0);audioPosition=gainPosition=overshootHold=0;
 gateEnvelope=previousPeak=fastReduction=slowReduction=overshootReduction=shownReduction=0;limiterGain=1;
 compression.setCurrentAndTargetValue(comp->load());gateThreshold.setCurrentAndTargetValue(gate->load());outputGain.setCurrentAndTargetValue(gain(output->load()));wet.setCurrentAndTargetValue(power->load()>.5f&&bypass->load()<.5f?1.f:0.f);
 // A fresh instance has no preceding audio. Initialize the control history to unity
 // gate and automatic makeup, matching its steady gain below threshold.
 overloadMemory=0;
 memoryGain=primaryGain=releaseStartGain=gain(-comp->load());releaseFraction=0;recovering=false;peakHistory.fill(0);
 gainHistory.fill(gain(-comp->load()));currentGrDb.store(0);currentOutDb.store(-100);
}
void FET67Processor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m){process(b,m,false);}
void FET67Processor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer& m){process(b,m,true);}
void FET67Processor::process(juce::AudioBuffer<float>& b,juce::MidiBuffer& m,bool hostBypassed) {
 juce::ScopedNoDenormals denormals;m.clear();const int n=juce::jmin(2,b.getNumChannels());if(n<1)return;
 compression.setTargetValue(comp->load());gateThreshold.setTargetValue(gate->load());outputGain.setTargetValue(gain(output->load()));wet.setTargetValue(!hostBypassed&&power->load()>.5f&&bypass->load()<.5f?1.f:0.f);
 const auto r=recovery(comp->load());const float chargePole=std::exp(-1./(rate*r.charge)),memoryReleasePole=std::exp(-1./(rate*r.release));
 constexpr float rippleTable[7][3]={{.175151f,.003198f,.464809f},{.175151f,.003198f,.464809f},{.256299f,.041601f,.442511f},{.411638f,.091361f,.453809f},{.562297f,.118417f,.477935f},{.65f,.132f,.49f},{.700354f,.138953f,.496946f}};
 const float rp=juce::jlimit(0.f,6.f,-comp->load()/6);const int ri=juce::jmin(5,int(rp));const float rf=rp-ri;
 const auto interp=[&](int k){return rippleTable[ri][k]+rf*(rippleTable[ri+1][k]-rippleTable[ri][k]);};
 const float rippleWeight=interp(0),rippleFast=std::exp(-1./(rate*interp(1))),rippleSlow=std::exp(-1./(rate*interp(2)));

 const float overloadPole=std::exp(-1./rate);
 fixMix.setTargetValue(fix->load()>.5f?1.f:0.f);deltaMix.setTargetValue(delta->load()>.5f?1.f:0.f);
 const double matchPole=std::exp(-1./(rate*.4));
 const float cut=lowCut->load();cutFrequency.setTargetValue(std::isfinite(cut)?juce::jlimit(5.f,400.f,cut):5.f);cutMix.setTargetValue(std::isfinite(cut)&&cut>0?1.f:0.f);
 float outPeak=0;
 for(int i=0;i<b.getNumSamples();++i) {
  std::array<float,2> in{};float peak=0;
  for(int c=0;c<n;++c){float x=b.getSample(c,i);in[size_t(c)]=std::isfinite(x)?juce::jlimit(-64.f,64.f,x):0;peak=juce::jmax(peak,std::abs(in[size_t(c)]));}
  const auto rawInput=in;const float frequency=cutFrequency.getNextValue(),cutWet=cutMix.getNextValue();
  const float g=std::tan(juce::MathConstants<float>::pi*frequency/float(rate));
  constexpr float damping[]{1.847759065f,.765366865f};peak=0;
  for(int c=0;c<n;++c){float x=in[size_t(c)];
   for(int s=0;s<2;++s){const float k=damping[s],a=1/(1+g*(g+k));auto& z1=cutState1[size_t(c)][size_t(s)];auto& z2=cutState2[size_t(c)][size_t(s)];const float v1=(z1+g*(x-z2))*a,v2=z2+g*v1;z1=2*v1-z1;z2=2*v2-z2;x=x-k*v1-v2;}
   in[size_t(c)]=rawInput[size_t(c)]+cutWet*(x-rawInput[size_t(c)]);peak=juce::jmax(peak,std::abs(in[size_t(c)]));
  }
  overloadMemory=juce::jmax(peak,overloadMemory*overloadPole);
  const float transient=peak+.234375f*juce::jmax(0.f,peak-previousPeak);
  gateEnvelope=juce::jmax(transient,gatePole*gateEnvelope+(1-gatePole)*peak);
  const float threshold=gateThreshold.getNextValue();const float gateG=threshold<=-79.99f?1.f:juce::jmin(1.f,gateEnvelope/gain(threshold));
  const float makeup=gain(-compression.getNextValue());const float keyed=peak; // Measured detector is pre-gate; audio gate precedes gain reduction.
  peakHistory[size_t(gainPosition)]=keyed;float heldPeak=0;
  for(int k=0;k<delay;++k)heldPeak=juce::jmax(heldPeak,peakHistory[size_t(k)]);
  const float targetGain=makeup/(1+(makeup-1)*heldPeak);
  const float memoryPole=targetGain<memoryGain?chargePole:memoryReleasePole;
  memoryGain=memoryPole*memoryGain+(1-memoryPole)*targetGain;
  if(targetGain<primaryGain){primaryGain=targetGain;recovering=false;}
  else {
   if(!recovering && targetGain>primaryGain*1.00001f){releaseStartGain=primaryGain;releaseFraction=0;recovering=true;}
   if(recovering){
    const float fraction=juce::jlimit(0.f,1.f,(targetGain-memoryGain)/juce::jmax(1.e-9f,targetGain-releaseStartGain));
    releaseFraction=juce::jmax(releaseFraction,fraction);
    const float tau=.0001f+r.base*std::pow(releaseFraction,r.power);
    recoveryPole=std::exp(-1./(rate*tau));
   }
   if(recovering)primaryGain=juce::jmin(targetGain,recoveryPole*primaryGain+(1-recoveryPole)*memoryGain);
  }
  const float target=20*std::log10(makeup/targetGain);
  const float transientReduction=20*std::log10(1+(makeup-1)*transient);
  if(transientReduction>overshootReduction){overshootReduction=transientReduction;overshootHold=delay;}
  else if(overshootHold>0)--overshootHold;else overshootReduction=target;
  fastReduction=juce::jmax(target,rippleFast*fastReduction+(1-rippleFast)*target);
  slowReduction=juce::jmax(target,rippleSlow*slowReduction+(1-rippleSlow)*target);
  const float heldGain=makeup*gain(-(rippleWeight*fastReduction+(1-rippleWeight)*slowReduction));
  const float charged=makeup>1.00001f?juce::jlimit(0.f,1.f,(makeup-memoryGain)/(makeup-1)):0.f;
  // The sustained correction was fitted within nominal input headroom. Preserve
  // the separate measured overload recovery after supernominal peaks.
  const float nominalWeight=juce::jlimit(0.f,1.f,(1-overloadMemory)*2);
  const float sustainedGain=juce::jmin(primaryGain,primaryGain+nominalWeight*charged*charged*(heldGain-primaryGain));
  const float compGain=juce::jmin(sustainedGain,makeup*gain(-overshootReduction));
  const float reduction=20*std::log10(makeup/juce::jmax(1.e-12f,compGain));
  gainHistory[size_t(gainPosition)]=gateG*compGain;
  float smoothed=0;for(int k=0;k<delay;++k)smoothed+=kernel[size_t(k)]*gainHistory[size_t((gainPosition-k+delay)%delay)];gainPosition=(gainPosition+1)%delay;
  std::array<float,2> dry{};const int read=(audioPosition+1)%(delay+1);float processedPeak=0;
  for(int c=0;c<n;++c){audioDelay[size_t(c)][size_t(audioPosition)]=rawInput[size_t(c)];filteredDelay[size_t(c)][size_t(audioPosition)]=in[size_t(c)];dry[size_t(c)]=audioDelay[size_t(c)][size_t(read)];processedPeak=juce::jmax(processedPeak,std::abs(dry[size_t(c)]*smoothed));}audioPosition=read;
  // Measured R-Vox limiting is part of the rational compression curve.
  // There is no additional hard ceiling at Comp = 0: supernominal input passes through.
  const float outG=outputGain.getNextValue(),mix=wet.getNextValue();
  const float correction=1+fixMix.getNextValue()*(matchGain.getNextValue()-1),audition=deltaMix.getNextValue();double inputEnergy=0,processedEnergy=0;
  for(int c=0;c<n;++c){const float processed=filteredDelay[size_t(c)][size_t(read)]*smoothed*.99f*outG;inputEnergy+=double(dry[size_t(c)])*dry[size_t(c)];processedEnergy+=double(processed)*processed;
   const float matched=processed*correction;const float listened=matched+audition*(dry[size_t(c)]-2*matched);const float y=dry[size_t(c)]+mix*(listened-dry[size_t(c)]);b.setSample(c,i,y);outPeak=juce::jmax(outPeak,std::abs(y));}
  matchInput=matchPole*matchInput+(1-matchPole)*inputEnergy/n;matchOutput=matchPole*matchOutput+(1-matchPole)*processedEnergy/n;
  if(++matchCounter>=64){matchCounter=0;if(gateG>.5f&&matchInput>1.e-6&&matchOutput>1.e-9){const double correctionDb=juce::jlimit(-24.,24.,10*std::log10(matchInput/matchOutput));matchGain.setTargetValue(gain(float(correctionDb)));}}

  const float total=reduction-db(juce::jmax(1.e-10f,gateG));shownReduction=juce::jmax(total,shownReduction*meterPole);previousPeak=peak;
 }
 currentGrDb.store(shownReduction*wet.getCurrentValue(),std::memory_order_relaxed);currentOutDb.store(db(outPeak),std::memory_order_relaxed);
}
juce::AudioProcessorParameter* FET67Processor::getBypassParameter() const {return apvts.getParameter("bypass");}
void FET67Processor::getStateInformation(juce::MemoryBlock& dest){auto state=apvts.copyState();state.setProperty("editorWidth",editorWidth.load(),nullptr);if(auto xml=state.createXml())copyXmlToBinary(*xml,dest);}
void FET67Processor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName(apvts.state.getType())){auto state=juce::ValueTree::fromXml(*xml);for(const char* id:{"fix","delta","meterMode"})if(!state.getChildWithProperty("id",id).isValid())apvts.getParameter(id)->setValueNotifyingHost(0);if(!state.getChildWithProperty("id","lowCut").isValid())apvts.getParameter("lowCut")->setValueNotifyingHost(0);editorWidth.store(juce::jlimit(817,4085,int(state.getProperty("editorWidth",1634))));apvts.replaceState(state);}}
juce::AudioProcessorEditor* FET67Processor::createEditor(){return new FET67Editor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new FET67Processor();}
