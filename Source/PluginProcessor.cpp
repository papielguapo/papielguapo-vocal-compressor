#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
float gain(float db) {return std::pow(10.0f,juce::jlimit(-240.f,120.f,db)*.05f);}
float parameter(std::atomic<float>* p,float fallback,float lo,float hi){const float v=p->load();return std::isfinite(v)?juce::jlimit(lo,hi,v):fallback;}
float dotGain(const float* a,const float* b,int count) noexcept {float s0=0,s1=0,s2=0,s3=0;int i=0;for(;i+3<count;i+=4){s0+=a[i]*b[i];s1+=a[i+1]*b[i+1];s2+=a[i+2]*b[i+2];s3+=a[i+3]*b[i+3];}float sum=(s0+s1)+(s2+s3);for(;i<count;++i)sum+=a[i]*b[i];return sum;}
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
 rate=std::isfinite(sr)&&sr>0?sr:48000.;
 delay=64;while(rate>48000.0*(delay/64)&&delay<maximumDelay)delay*=2;cleanGain.prepare(rate);harmonics.prepare(rate,processingCapacity);peakSafety.prepare(rate);cleanLatency=cleanGain.latency();
 for(auto& c:cleanDryDelay)c.assign(size_t(cleanLatency+1),0);for(auto& c:cleanWetDelay)c.assign(size_t(cleanLatency+1),0);
 wetScratch.setSize(2,processingCapacity);dryScratch.setSize(2,processingCapacity);
 colourMeterDelay.assign(size_t(harmonics.latency()+1),0);limiterMeterDelay.assign(size_t(peakSafety.latency()+1),0);
 detectorPeak.prepare(delay);setLatencySamples(delay+cleanLatency+harmonics.latency()+peakSafety.latency());
 gatePole=std::exp(-1./(rate*.13));limiterPole=std::exp(-1./(rate*.05));meterPole=std::exp(-1./(rate*.25));
 for(auto* s:{&compression,&gateThreshold,&outputGain,&wet,&cutFrequency,&cutMix,&fixMix,&deltaMix})s->reset(rate,.02);
 matchGain.reset(rate,1.0);kernel.fill(0);float sum=0;for(int i=0;i<delay;++i){kernel[size_t(i)]=std::sin(juce::MathConstants<float>::pi*(i+.5f)/delay);sum+=kernel[size_t(i)];}for(auto& k:kernel)k/=sum;for(int k=0;k<delay;++k)reverseKernel[size_t(k)]=kernel[size_t(delay-1-k)];reset();
}
void FET67Processor::reset() {
 peakSafety.reset();detectorPeak.reset();colourMeterPosition=limiterMeterPosition=0;std::fill(colourMeterDelay.begin(),colourMeterDelay.end(),0.f);std::fill(limiterMeterDelay.begin(),limiterMeterDelay.end(),0.f);
 coefficientComp=100;releaseTauCache=-1;updateCoefficients(parameter(comp,-12,-36,0));
 cleanPosition=0;for(auto& c:cleanDryDelay)std::fill(c.begin(),c.end(),0.f);for(auto& c:cleanWetDelay)std::fill(c.begin(),c.end(),0.f);cleanGain.reset(-parameter(comp,-12,-36,0));harmonics.reset();
 for(auto& c:filteredDelay)c.fill(0);for(auto& c:cutState1)c.fill(0);for(auto& c:cutState2)c.fill(0);
 matchInput=matchOutput=0;matchCounter=0;matchGain.setCurrentAndTargetValue(1);fixMix.setCurrentAndTargetValue(fix->load()>.5f?1.f:0.f);deltaMix.setCurrentAndTargetValue(delta->load()>.5f?1.f:0.f);
 cutFrequency.setCurrentAndTargetValue(parameter(lowCut,0,0,400));cutMix.setCurrentAndTargetValue(parameter(lowCut,0,0,400)>0?1.f:0.f);
 for(auto& channel:audioDelay)channel.fill(0);audioPosition=gainPosition=overshootHold=0;
 gateEnvelope=previousPeak=fastReduction=slowReduction=overshootReduction=shownReduction=0;limiterGain=1;
 compression.setCurrentAndTargetValue(parameter(comp,-12,-36,0));gateThreshold.setCurrentAndTargetValue(parameter(gate,-80,-80,0));outputGain.setCurrentAndTargetValue(gain(parameter(output,-3,-36,0)));wet.setCurrentAndTargetValue(power->load()>.5f&&bypass->load()<.5f?1.f:0.f);
 // A fresh instance has no preceding audio. Initialize the control history to unity
 // gate and automatic makeup, matching its steady gain below threshold.
 overloadMemory=0;
 memoryGain=primaryGain=releaseStartGain=gain(-parameter(comp,-12,-36,0));releaseFraction=0;recovering=false;peakHistory.fill(0);
 gainHistory.fill(gain(-parameter(comp,-12,-36,0)));currentGrDb.store(0);currentOutDb.store(-100);
}
void FET67Processor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m){process(b,m,false);}
void FET67Processor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer& m){process(b,m,true);}
void FET67Processor::updateCoefficients(float compDb){
 if(compDb==coefficientComp)return;coefficientComp=compDb;const auto r=recovery(compDb);
 coefficients.base=r.base;coefficients.power=r.power;coefficients.chargePole=float(std::exp(-1/(rate*r.charge)));coefficients.memoryReleasePole=float(std::exp(-1/(rate*r.release)));
 constexpr float table[7][3]={{.175151f,.003198f,.464809f},{.175151f,.003198f,.464809f},{.256299f,.041601f,.442511f},{.411638f,.091361f,.453809f},{.562297f,.118417f,.477935f},{.65f,.132f,.49f},{.700354f,.138953f,.496946f}};
 const float p=juce::jlimit(0.f,6.f,-compDb/6);const int i=juce::jmin(5,int(p));const float f=p-i;const auto interpolate=[&](int k){return table[i][k]+f*(table[i+1][k]-table[i][k]);};
 coefficients.rippleWeight=interpolate(0);coefficients.rippleFast=float(std::exp(-1/(rate*interpolate(1))));coefficients.rippleSlow=float(std::exp(-1/(rate*interpolate(2))));
}
void FET67Processor::process(juce::AudioBuffer<float>& b,juce::MidiBuffer& m,bool hostBypassed){
 juce::ScopedNoDenormals denormals;m.clear();const int n=juce::jmin(2,b.getNumChannels());if(n<1||wetScratch.getNumSamples()==0)return;
 compression.setTargetValue(parameter(comp,-12,-36,0));gateThreshold.setTargetValue(parameter(gate,-80,-80,0));outputGain.setTargetValue(gain(parameter(output,-3,-36,0)));
 wet.setTargetValue(!hostBypassed&&power->load()>.5f&&bypass->load()<.5f?1.f:0.f);fixMix.setTargetValue(fix->load()>.5f?1.f:0.f);deltaMix.setTargetValue(delta->load()>.5f?1.f:0.f);
 const float cut=parameter(lowCut,0,0,400);cutFrequency.setTargetValue(cut);cutMix.setTargetValue(cut>0?1.f:0.f);
 float outPeak=0;for(int offset=0;offset<b.getNumSamples();offset+=processingCapacity)processChunk(b,offset,juce::jmin(processingCapacity,b.getNumSamples()-offset),n,outPeak);
 currentGrDb.store(shownReduction*wet.getCurrentValue(),std::memory_order_relaxed);currentOutDb.store(db(outPeak),std::memory_order_relaxed);
}
void FET67Processor::processChunk(juce::AudioBuffer<float>& b,int offset,int count,int n,float& outPeak){
 const float overloadPole=float(std::exp(-1/rate));const double matchPole=std::exp(-1/(rate*.4));
 for(int i=0;i<count;++i){
  const float compDb=compression.getNextValue();updateCoefficients(compDb);const auto& r=coefficients;
  const float chargePole=r.chargePole,memoryReleasePole=r.memoryReleasePole,rippleWeight=r.rippleWeight,rippleFast=r.rippleFast,rippleSlow=r.rippleSlow;
  std::array<float,2> in{};float peak=0;
  for(int c=0;c<n;++c){float x=b.getSample(c,offset+i);in[size_t(c)]=std::isfinite(x)?juce::jlimit(-64.f,64.f,x):0;peak=juce::jmax(peak,std::abs(in[size_t(c)]));}
  const auto rawInput=in;const float frequency=cutFrequency.getNextValue(),cutWet=cutMix.getNextValue();
  const double g=std::tan(juce::MathConstants<double>::pi*juce::jmin(double(frequency),rate*.45)/rate);
  constexpr double damping[]{1.8477590650225735,.7653668647301795};peak=0;
  for(int c=0;c<n;++c){double x=in[size_t(c)];
   for(int s=0;s<2;++s){const double k=damping[s],a=1/(1+g*(g+k));auto& z1=cutState1[size_t(c)][size_t(s)];auto& z2=cutState2[size_t(c)][size_t(s)];const double v1=(z1+g*(x-z2))*a,v2=z2+g*v1;z1=2*v1-z1;z2=2*v2-z2;x=x-k*v1-v2;}
   in[size_t(c)]=float(rawInput[size_t(c)]+cutWet*(x-rawInput[size_t(c)]));peak=juce::jmax(peak,std::abs(in[size_t(c)]));
  }
  overloadMemory=juce::jmax(peak,overloadMemory*overloadPole);
  const float transient=peak+.234375f*juce::jmax(0.f,peak-previousPeak);
  gateEnvelope=juce::jmax(transient,gatePole*gateEnvelope+(1-gatePole)*peak);
  const float threshold=gateThreshold.getNextValue();const float gateG=threshold<=-79.99f?1.f:juce::jmin(1.f,gateEnvelope/gain(threshold));
  const float makeup=gain(-compDb);const float keyed=peak; // Measured detector is pre-gate; audio gate precedes gain reduction.
  const float heldPeak=detectorPeak.tick(keyed);
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
    if(tau!=releaseTauCache){recoveryPole=std::exp(-1./(rate*tau));releaseTauCache=tau;}
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
  float smoothed=dotGain(gainHistory.data(),reverseKernel.data()+delay-1-gainPosition,gainPosition+1);
  const int remaining=delay-gainPosition-1;if(remaining>0)smoothed+=dotGain(gainHistory.data()+gainPosition+1,reverseKernel.data(),remaining);gainPosition=(gainPosition+1)%delay;
  smoothed=gain(cleanGain.tick(db(juce::jmax(1.e-12f,smoothed))));
  std::array<float,2> dry{};const int read=(audioPosition+1)%(delay+1);float processedPeak=0;
  for(int c=0;c<n;++c){audioDelay[size_t(c)][size_t(audioPosition)]=rawInput[size_t(c)];filteredDelay[size_t(c)][size_t(audioPosition)]=in[size_t(c)];dry[size_t(c)]=audioDelay[size_t(c)][size_t(read)];processedPeak=juce::jmax(processedPeak,std::abs(dry[size_t(c)]*smoothed));}audioPosition=read;
  // Gain-cell output is protected after colour, Gain and FIX in the second pass.
  const int cleanRead=(cleanPosition+1)%(cleanLatency+1);std::array<float,2> processedAudio{};
  for(int c=0;c<n;++c){cleanDryDelay[size_t(c)][size_t(cleanPosition)]=dry[size_t(c)];cleanWetDelay[size_t(c)][size_t(cleanPosition)]=filteredDelay[size_t(c)][size_t(read)];dry[size_t(c)]=cleanDryDelay[size_t(c)][size_t(cleanRead)];processedAudio[size_t(c)]=cleanWetDelay[size_t(c)][size_t(cleanRead)]*smoothed*.99f;}cleanPosition=cleanRead;
  const float macro=juce::jlimit(0.f,1.f,-compression.getCurrentValue()/36);const float depth=juce::jlimit(0.f,1.f,reduction/18);
  learningValues[size_t(i)]=gateG;colourTargets[size_t(i)]=macro*(.35f+.65f*depth);headroomValues[size_t(i)]=3*std::sqrt(macro);
  gainValues[size_t(i)]=outputGain.getNextValue()*gain(-headroomValues[size_t(i)]);wetValues[size_t(i)]=wet.getNextValue();fixValues[size_t(i)]=fixMix.getNextValue();deltaValues[size_t(i)]=deltaMix.getNextValue();
  cellMeter[size_t(i)]=juce::jmax(0.f,db(makeup)-db(juce::jmax(1.e-12f,smoothed)));
  for(int c=0;c<2;++c){wetScratch.setSample(c,i,c<n?processedAudio[size_t(c)]:0);dryScratch.setSample(c,i,c<n?dry[size_t(c)]:0);}previousPeak=peak;
 }
 auto wetBlock=juce::dsp::AudioBlock<float>(wetScratch).getSubBlock(0,size_t(count));auto dryBlock=juce::dsp::AudioBlock<float>(dryScratch).getSubBlock(0,size_t(count));harmonics.process(wetBlock,dryBlock,colourTargets.data());
 for(int i=0;i<count;++i){
  const float correction=1+fixValues[size_t(i)]*(matchGain.getNextValue()-1);std::array<float,2> dry{},matched{};double inputEnergy=0,processedEnergy=0;
  for(int c=0;c<n;++c){dry[size_t(c)]=dryScratch.getSample(c,i);const float processed=wetScratch.getSample(c,i)*gainValues[size_t(i)];inputEnergy+=double(dry[size_t(c)])*dry[size_t(c)];processedEnergy+=double(processed)*processed;matched[size_t(c)]=processed*correction;}
  matchInput=matchPole*matchInput+(1-matchPole)*inputEnergy/n;matchOutput=matchPole*matchOutput+(1-matchPole)*processedEnergy/n;
  // A closed gain cell and silence do not train a runaway makeup boost.
  if(++matchCounter>=64){matchCounter=0;if(learningValues[size_t(i)]>.5f&&matchInput>1.e-6&&matchOutput>1.e-9){matchGain.setTargetValue(gain(float(juce::jlimit(-24.,24.,10*std::log10(matchInput/matchOutput)))));}}
  const int colourRead=(colourMeterPosition+1)%int(colourMeterDelay.size());colourMeterDelay[size_t(colourMeterPosition)]=cellMeter[size_t(i)];const float appliedCellGr=colourMeterDelay[size_t(colourRead)];colourMeterPosition=colourRead;
  const int limiterRead=(limiterMeterPosition+1)%int(limiterMeterDelay.size());limiterMeterDelay[size_t(limiterMeterPosition)]=appliedCellGr+headroomValues[size_t(i)];const float alignedGr=limiterMeterDelay[size_t(limiterRead)];limiterMeterPosition=limiterRead;
  const float safetyGr=peakSafety.tick(matched,dry,n);std::array<float,2> difference{};float deltaPeak=0;
  for(int c=0;c<n;++c){difference[size_t(c)]=dry[size_t(c)]-matched[size_t(c)];deltaPeak=juce::jmax(deltaPeak,std::abs(difference[size_t(c)]));}
  const float deltaSafety=deltaPeak>VocalPeakSafety::ceiling?VocalPeakSafety::ceiling/deltaPeak:1.f;
  for(int c=0;c<n;++c){const float listened=juce::jlimit(-VocalPeakSafety::ceiling,VocalPeakSafety::ceiling,matched[size_t(c)]+deltaValues[size_t(i)]*(difference[size_t(c)]*deltaSafety-matched[size_t(c)]));const float mix=wetValues[size_t(i)];const float y=mix<=0?dry[size_t(c)]:mix>=1?listened:dry[size_t(c)]+mix*(listened-dry[size_t(c)]);b.setSample(c,offset+i,y);outPeak=juce::jmax(outPeak,std::abs(y));}
  shownReduction=juce::jmax(alignedGr+safetyGr,shownReduction*meterPole);
 }
}
juce::AudioProcessorParameter* FET67Processor::getBypassParameter() const {return apvts.getParameter("bypass");}
void FET67Processor::getStateInformation(juce::MemoryBlock& dest){auto state=apvts.copyState();state.setProperty("editorWidth",editorWidth.load(),nullptr);if(auto xml=state.createXml())copyXmlToBinary(*xml,dest);}
void FET67Processor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName(apvts.state.getType())){auto state=juce::ValueTree::fromXml(*xml);for(const char* id:{"fix","delta","meterMode"})if(!state.getChildWithProperty("id",id).isValid())apvts.getParameter(id)->setValueNotifyingHost(0);if(!state.getChildWithProperty("id","lowCut").isValid())apvts.getParameter("lowCut")->setValueNotifyingHost(0);editorWidth.store(juce::jlimit(817,4085,int(state.getProperty("editorWidth",1634))));apvts.replaceState(state);}}
juce::AudioProcessorEditor* FET67Processor::createEditor(){return new FET67Editor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new FET67Processor();}
