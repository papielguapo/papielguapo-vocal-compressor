#include "PluginEditor.h"
#include <charconv>
#include <cmath>

namespace
{
juce::Colour colour(juce::uint32 hex) { return juce::Colour(hex); }
juce::Font font(const juce::String& name,float size,int style)
{
#if JUCE_MAJOR_VERSION >= 8
    return juce::Font(juce::FontOptions(name,size,style));
#else
    return juce::Font(name,size,style);
#endif
}
void text(juce::Graphics& g, const juce::String& s, float x, float y, float w, float h, float size = 10, bool bold = true)
{
    g.setFont(font("Arial", size, bold ? juce::Font::bold : juce::Font::plain));
    g.drawText(s, juce::Rectangle<float>(x,y,w,h), juce::Justification::centred, false);
}
void screw(juce::Graphics& g, float x, float y)
{
    g.setColour(juce::Colours::white.withAlpha(0.42f)); g.fillEllipse(x-15,y-14,31,31);
    g.setGradientFill(juce::ColourGradient(colour(0xff080b0c),x-8,y-12,colour(0xff353b3c),x+9,y+12,false));
    g.fillEllipse(x-15,y-15,30,30);
    g.setGradientFill(juce::ColourGradient(colour(0xff191f20),x-3,y-6,colour(0xff020404),x+4,y+5,true));
    g.fillEllipse(x-6,y-6,12,12);
}
void lamp(juce::Graphics& g, juce::Rectangle<float> r, bool on)
{
    const auto c=r.getCentre();
    if(on) { g.setGradientFill(juce::ColourGradient(colour(0xff9b26ff).withAlpha(0.35f),c.x,c.y,juce::Colours::transparentBlack,c.x+r.getWidth()*0.85f,c.y,true));g.fillEllipse(r.expanded(r.getWidth()*0.4f)); }
    g.setColour(juce::Colours::black.withAlpha(0.3f));g.fillEllipse(r.translated(2,3));
    g.setGradientFill(juce::ColourGradient(colour(0xff73349d),r.getX(),r.getY(),colour(0xff260438),r.getRight(),r.getBottom(),false));g.fillEllipse(r);
    auto inner=r.reduced(5);
    if(on) { auto jewel=juce::ColourGradient(colour(0xffdf9aff),c.x-10,c.y-13,colour(0xff490078),c.x+24,c.y+20,true);jewel.addColour(0.36,colour(0xffb839ff));jewel.addColour(0.74,colour(0xff7900ce));g.setGradientFill(jewel);g.fillEllipse(inner);g.setColour(colour(0xffdd8fff).withAlpha(0.23f));g.drawEllipse(inner.reduced(1),2); }
    else { g.setColour(colour(0xff240046));g.fillEllipse(inner); }
    g.setColour(juce::Colours::white.withAlpha(on?0.83f:0.12f));g.fillEllipse(r.getX()+13,r.getY()+12,r.getWidth()*0.30f,r.getHeight()*0.19f);
    if(on){g.setColour(colour(0xffdc8fff).withAlpha(0.36f));g.fillEllipse(c.x+4,c.y+11,13,7);}
}
}

void FET67LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float pos, float start, float end, juce::Slider& slider)
{
    const float r = static_cast<float>(juce::jmin(width, height)) * 0.46f;
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    g.setColour(juce::Colours::black.withAlpha(0.38f));
    g.fillEllipse(cx-r+2,cy-r+5,r*2+2,r*2+2);
    g.setColour(colour(0xff24202c));g.fillEllipse(cx-r-1,cy-r-1,r*2+2,r*2+2);
    auto resin=juce::ColourGradient(colour(0xffaf40ff),cx-r*0.55f,cy-r*0.75f,
                                   colour(0xff200034),cx+r*0.8f,cy+r*0.9f,false);
    resin.addColour(0.34,colour(0xff7e12d5));resin.addColour(0.65,colour(0xff51008e));
    g.setGradientFill(resin);g.fillEllipse(cx-r,cy-r,r*2,r*2);
    g.setColour(colour(0xffdd9cff).withAlpha(0.8f));g.drawEllipse(cx-r+1.4f,cy-r+1.4f,r*2-2.8f,r*2-2.8f,1.1f);
    g.setColour(colour(0xff26003f).withAlpha(0.75f));g.drawEllipse(cx-r+4,cy-r+4,r*2-8,r*2-8,1.4f);
    juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::rotation(start+pos*(end-start)).translated(cx,cy));
    // Narrow concave channels follow the fin; the stationary resin skirt catches the light.
    for(float side : {-1.0f,1.0f})
    {
        juce::Path flute;
        flute.startNewSubPath(side*r*0.25f,-r*0.88f);
        flute.cubicTo(side*r*0.73f,-r*0.63f,side*r*0.77f,r*0.46f,side*r*0.30f,r*0.85f);
        flute.cubicTo(side*r*0.47f,r*0.25f,side*r*0.47f,-r*0.32f,side*r*0.25f,-r*0.88f);
        flute.closeSubPath();g.setColour(colour(0xff25003e));g.fillPath(flute);
        juce::Path rib;rib.startNewSubPath(side*r*0.55f,-r*0.67f);
        rib.cubicTo(side*r*0.77f,-r*0.27f,side*r*0.73f,r*0.32f,side*r*0.48f,r*0.67f);
        g.setColour(colour(0xffc45cff).withAlpha(side<0?0.56f:0.22f));g.strokePath(rib,juce::PathStrokeType(1.0f));
    }
    juce::Path fin;
    fin.startNewSubPath(-r*0.07f,-r*0.94f);
    fin.cubicTo(r*0.12f,-r*0.97f,r*0.18f,-r*0.70f,r*0.19f,-r*0.25f);
    fin.lineTo(r*0.21f,r*0.58f);
    fin.quadraticTo(r*0.24f,r*0.86f,r*0.04f,r*0.91f);
    fin.quadraticTo(-r*0.18f,r*0.94f,-r*0.20f,r*0.68f);
    fin.lineTo(-r*0.19f,-r*0.60f);
    fin.quadraticTo(-r*0.19f,-r*0.89f,-r*0.07f,-r*0.94f);fin.closeSubPath();
    auto ridge=juce::ColourGradient(colour(0xffe3a2ff),-r*0.20f,0,colour(0xff3c0065),r*0.23f,0,false);
    ridge.addColour(0.23,colour(0xffb14cff));ridge.addColour(0.40,colour(0xff7b13c0));
    ridge.addColour(0.63,colour(0xff6500a8));g.setGradientFill(ridge);g.fillPath(fin);
    g.setColour(colour(0xffcc82ff).withAlpha(0.72f));g.strokePath(fin,juce::PathStrokeType(0.8f));
    g.setColour(colour(0xfff8efff));g.fillRoundedRectangle(-r*0.09f,-r*0.87f,r*0.075f,r*0.49f,0.7f);
    if(slider.hasKeyboardFocus(true)) {g.setColour(colour(0xffefd4ff));g.drawEllipse(-r-3,-r-3,r*2+6,r*2+6,1);}
}

void FET67LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hover, bool down)
{
    const auto c=b.getLocalBounds().toFloat().getCentre();
    const bool right=b.getToggleState()!=static_cast<bool>(b.getProperties().getWithDefault("reverse",false));
    const float bx=c.x+3, tip=bx+(right?12.0f:-12.0f);
    g.setColour(juce::Colours::black.withAlpha(0.28f));g.fillEllipse(bx-12,c.y-11,27,28);
    g.setGradientFill(juce::ColourGradient(colour(0xff555b5e),bx-8,c.y-12,colour(0xff020506),bx+9,c.y+13,false));
    g.fillEllipse(bx-13,c.y-13,26,26);
    g.setColour(colour(0xff090c0d));g.fillEllipse(bx-10,c.y-10,20,20);
    g.setGradientFill(juce::ColourGradient(colour(0xff4c555b),tip,c.y-7,colour(0xff080b0d),tip,c.y+7,false));
    g.fillRoundedRectangle(juce::jmin(tip,bx)-5,c.y-7,std::abs(tip-bx)+10,14,6);
    g.setColour(colour(0xff030607));g.fillEllipse(tip-8,c.y-8,16,16);
    g.setGradientFill(juce::ColourGradient(colour(0xff515a60),tip-3,c.y-5,colour(0xff101416),tip+4,c.y+5,false));
    g.fillEllipse(tip-5,c.y-5.5f,10,11);
    if(hover||down||b.hasKeyboardFocus(true)){g.setColour(colour(0xffb84dff).withAlpha(0.65f));g.drawEllipse(bx-15,c.y-15,30,30,1);}
}
FaceplateComponent::FaceplateComponent(FET67Processor& p):processor(p)
{
    setLookAndFeel(&look);
    const char* ids[]{"lowCut","gate","comp","gain"};
    const char* names[]{"Low Cut","Gate","Comp","Gain"};
    for(size_t i=0;i<knobs.size();++i)
    {
        auto& s=knobs[i];s.inverted=i==2;addAndMakeVisible(s);s.setName(names[i]);s.setTitle(names[i]);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
        s.setRotaryParameters(juce::MathConstants<float>::pi*7/6,juce::MathConstants<float>::pi*17/6,true);
        s.setPopupDisplayEnabled(true,true,this);s.setMouseDragSensitivity(180);s.setScrollWheelEnabled(true);
        knobAttachments[i]=std::make_unique<SA>(processor.apvts,ids[i],s);
        s.setDoubleClickReturnValue(false,0);
        s.onValueEntry=[this,i]{beginValueEntry(i);};
        s.textFromValueFunction=[i](double v)
        {
            if(i==0) return v<=0 ? juce::String("OFF") : juce::String(v,0)+" Hz";
            if(i==1 && v<=-79.99) return juce::String("OFF");
            return juce::String(v,1)+" dB";
        };
    }
    addChildComponent(valueEntry);
    valueEntry.setMultiLine(false);valueEntry.setJustification(juce::Justification::centred);
    valueEntry.setSelectAllWhenFocused(true);valueEntry.setFont(juce::Font(juce::FontOptions(16.0f)));
    valueEntry.setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff161019));
    valueEntry.setColour(juce::TextEditor::textColourId,juce::Colours::white);
    valueEntry.setColour(juce::TextEditor::focusedOutlineColourId,juce::Colour(0xffb84dff));
    valueEntry.onReturnKey=[this]{commitValueEntry();};
    valueEntry.onEscapeKey=[this]{cancelValueEntry();};
    valueEntry.onFocusLost=[this]{cancelValueEntry();};
    const char* switchIds[]{"power","delta","meterMode","fix"};
    const char* tips[]{"Power: on / off","DELTA: hear dry minus processed signal (including FIX)","GR / OUT: meter gain reduction or output peak; OUT shows dBFS","FIX: adaptive RMS auto gain, +/-24 dB; holds through silence and closed gate"};
    for(size_t i=0;i<switches.size();++i) {auto& b=switches[i];addAndMakeVisible(b);b.setName(switchIds[i]);b.setTitle(tips[i]);b.setTooltip(tips[i]);b.setClickingTogglesState(true);if(i==0)b.getProperties().set("reverse",true);switchAttachments[i]=std::make_unique<BA>(processor.apvts,switchIds[i],b);}
    startTimerHz(60);
}
FaceplateComponent::~FaceplateComponent()
{
    stopTimer();valueEntry.onFocusLost=nullptr;valueEntry.onReturnKey=nullptr;valueEntry.onEscapeKey=nullptr;
    setLookAndFeel(nullptr);
}
void FaceplateComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    for(size_t i=0;i<knobs.size();++i){const int cx=knobs[i].getBounds().getCentreX();
        if(juce::Rectangle<int>(cx-53,170,106,23).contains(event.getPosition())){beginValueEntry(i);return;}}
}
void FaceplateComponent::beginValueEntry(size_t index)
{
    editingKnob=static_cast<int>(index);
    auto& knob=knobs[index];
    valueEntry.setName(knob.getName()+" value");
    valueEntry.setTooltip(knob.getName()+(index==0?" (Hz)":" (dB)")+". Enter: apply; Escape: cancel.");
    valueEntry.setColour(juce::TextEditor::focusedOutlineColourId,juce::Colour(0xffb84dff));
    valueEntry.setBounds(knob.getBounds().withSizeKeepingCentre(116,30));
    valueEntry.setText(juce::String(knob.getValue(),3),false);
    valueEntry.setVisible(true);valueEntry.toFront(true);valueEntry.grabKeyboardFocus();valueEntry.selectAll();
}
void FaceplateComponent::cancelValueEntry()
{
    editingKnob=-1;valueEntry.setVisible(false);
}
void FaceplateComponent::commitValueEntry()
{
    if(editingKnob<0)return;
    const int index=editingKnob;
    auto textValue=valueEntry.getText().trim().replaceCharacter(',','.');
    if(index==0&&textValue.endsWithIgnoreCase("Hz"))textValue=textValue.dropLastCharacters(2).trim();
    if(index!=0&&textValue.endsWithIgnoreCase("dB"))textValue=textValue.dropLastCharacters(2).trim();
    double value=0;bool valid=false;
    if(textValue.equalsIgnoreCase("OFF") && (index==0 || index==1)) {value=index==1?-80:0;valid=true;}
    else {
        if(textValue.startsWithChar('+')) textValue=textValue.substring(1);
        const auto raw=textValue.toStdString();
        const auto parsed=std::from_chars(raw.data(),raw.data()+raw.size(),value);
        valid=parsed.ec==std::errc{}&&parsed.ptr==raw.data()+raw.size()&&std::isfinite(value);
    }
    if(!valid)
    {
        valueEntry.setColour(juce::TextEditor::focusedOutlineColourId,juce::Colours::red);
        valueEntry.selectAll();return;
    }
    auto& knob=knobs[static_cast<size_t>(index)];
    const char* ids[]{"lowCut","gate","comp","gain"};
    auto* parameter=processor.apvts.getParameter(ids[index]);
    parameter->beginChangeGesture();
    knob.setValue(juce::jlimit(knob.getMinimum(),knob.getMaximum(),value),juce::sendNotificationSync);
    parameter->endChangeGesture();
    cancelValueEntry();knob.grabKeyboardFocus();
}
void FaceplateComponent::resized()
{
    constexpr int centres[]{374,557,740,924};
    for(size_t i=0;i<knobs.size();++i)knobs[i].setBounds(centres[i]-38,80,76,76);
    switches[0].setBounds(204,137,50,36);
    for(int i=1;i<4;++i)switches[size_t(i)].setBounds(1039+130*(i-1),146,50,36);

}
void FaceplateComponent::timerCallback()
{
    shownGr=processor.currentGrDb.load(std::memory_order_relaxed);
    shownOut=processor.currentOutDb.load(std::memory_order_relaxed);
    repaint();
}
void FaceplateComponent::paintMeter(juce::Graphics& g)
{
    const juce::Rectangle<float> bezel(1003,51,380,61);
    g.setColour(juce::Colours::white.withAlpha(0.72f));g.fillRoundedRectangle(bezel.expanded(1),3);
    g.setColour(colour(0xff08090a));g.fillRoundedRectangle(bezel,2);
    g.setColour(colour(0xff73777b));g.drawRoundedRectangle(bezel.reduced(1),2,1);
    constexpr float values[]{-21,-18,-16,-14,-12,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,-0.5f,0,0.5f,1,2,3,4,6};
    const char* labels[]{"-21","-18","-16","-14","-12","-10","-9","-8","-7","-6","-5","-4","-3","-2","-1","-.5","0",".5","1","2","3","4","CLIP"};
    constexpr float outValues[]{-60,-54,-48,-42,-36,-30,-27,-24,-21,-18,-15,-12,-9,-6,-5,-4,-3,-2,-1,-.5f,0,1,0};
    const char* outLabels[]{"-60","-54","-48","-42","-36","-30","-27","-24","-21","-18","-15","-12","-9","-6","-5","-4","-3","-2","-1","-.5","0","+1","CLIP"};
    const bool out=processor.apvts.getRawParameterValue("meterMode")->load()>.5f;
    const bool powered=processor.apvts.getRawParameterValue("power")->load()>0.5f;
    for(int i=0;i<23;++i)
    {
        const float x=1015+static_cast<float>(i)*15.5f;
        const bool on=powered&&(out?shownOut>=outValues[i]:i<=16&&(i==16||shownGr>=-values[i]));
        if(on){g.setGradientFill(juce::ColourGradient(colour(0xffff2929).withAlpha(0.45f),x+5.5f,81,juce::Colours::transparentBlack,x+18,81,true));g.fillEllipse(x-8,67,27,28);}
        g.setColour(colour(0xff020303));g.fillRect(x-1,75.0f,13.0f,13.0f);
        g.setColour(colour(on?0xffff3030:0xff41191c));g.fillRect(x,76.0f,11.0f,11.0f);
        if(on){g.setColour(colour(0xffff8173));g.fillRect(x+1,77.0f,9.0f,2.0f);}
        g.setColour(colour(0xffdfdfe1));
        if(i<=16)text(g,out?outLabels[i]:labels[i],x-3,55,17,10,8.3f);
        text(g,out?outLabels[i]:labels[i],x-3,97,i==22?25.0f:17.0f,10,i==22?7.4f:8.3f);
    }
}

void FaceplateComponent::paint(juce::Graphics& g)
{
    g.fillAll(colour(0xff08090b));
    const juce::Rectangle<float> plate(5,4,1622,242);
    juce::Path outline;outline.addRoundedRectangle(plate,6);
    {
        juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(outline);
        auto aluminium=juce::ColourGradient(colour(0xffe4e6e9),0,4,colour(0xff8a9095),0,246,false);
        aluminium.addColour(0.08,colour(0xffe4e6e9));aluminium.addColour(0.35,colour(0xffdfe3e6));
        aluminium.addColour(0.64,colour(0xffd9dee1));aluminium.addColour(0.92,colour(0xffd1d6da));
        aluminium.addColour(0.96,colour(0xffa0a5aa));
        g.setGradientFill(aluminium);g.fillRect(plate);
        for(int y=5;y<246;++y)
        {
            const float alpha=y%4==0?0.28f:y%4==1?0.10f:y%4==2?0.025f:0.045f;
            g.setColour((y%4<2?juce::Colours::white:juce::Colours::black).withAlpha(alpha));
            g.drawHorizontalLine(y,5,1627);
        }
        // Subtle broad machining highlights retain the horizontal grain at any scale.
        for(int i=0;i<12;++i){g.setColour(juce::Colours::white.withAlpha(0.012f));g.fillRect(5.0f,9.0f+static_cast<float>(i)*19,1622.0f,7.0f);}
    }
    g.setColour(juce::Colours::white.withAlpha(0.6f));g.drawRoundedRectangle(plate.reduced(0.5f),5,1);
    g.setColour(juce::Colours::black.withAlpha(0.25f));g.drawRoundedRectangle(plate.reduced(2),4,1);
    for(float x:{59.0f,1575.0f}){g.setColour(juce::Colours::black.withAlpha(0.25f));g.drawVerticalLine(static_cast<int>(x),6,244);g.setColour(juce::Colours::white.withAlpha(0.24f));g.drawVerticalLine(static_cast<int>(x+1),6,244);}
    for(float x:{16.0f,1585.0f})for(float y:{28.0f,199.0f})
    {
        g.setColour(juce::Colours::white.withAlpha(0.8f));g.fillRoundedRectangle(x-1,y-1,33,24,12);
        g.setColour(colour(0xff474d50));g.fillRoundedRectangle(x,y,31,22,11);
        g.setColour(colour(0xff020506));g.fillRoundedRectangle(x+1,y+1,29,20,10);
    }
    for(auto point:{juce::Point<float>(165,53),{165,198},{1417,53},{1417,198}})screw(g,point.x,point.y);
    g.setColour(colour(0xff15191b));g.setFont(font("Arial Black",18,juce::Font::bold));
    g.drawText("VOCAL",68,121,104,23,juce::Justification::centredLeft);
    g.setColour(colour(0xffc6cccf));g.fillRect(68.0f,132.0f,95.0f,1.8f);
    g.setColour(colour(0xff171c1f));g.setFont(font("Arial",10.0f,juce::Font::bold));
    g.drawText("COMPRESSOR",68,145,104,15,juce::Justification::centredLeft);
    lamp(g,{206,32,60,60},processor.apvts.getRawParameterValue("power")->load()>0.5f);
    g.setColour(colour(0xff171c1f));text(g,"POWER",197,102,78,18,14);
    text(g,"ON",176,145,35,17,11);text(g,"OFF",258,145,35,17,11);
    constexpr float centres[]{374,557,740,924};
    const char* titles[]{"LOW CUT","GATE","COMP","GAIN"};
    for(int i=0;i<4;++i)
    {
        const float cx=centres[i]; constexpr int steps=4;
        const char* scales[4][5] = {{"OFF","100","200","300","400"},{"OFF","-60","-40","-20","0"},{"0","-9","-18","-27","-36"},{"-36","-27","-18","-9","0"}};
        for(int tick=0;tick<=steps;++tick) {
            const float a=juce::MathConstants<float>::pi*(7.0f/6.0f+(5.0f/3.0f)*tick/steps);
            const float dx=std::sin(a),dy=-std::cos(a);
            g.setColour(colour(0xff282e30));g.fillEllipse(cx+dx*48-1.7f,118+dy*48-1.7f,3.4f,3.4f);
            text(g,scales[i][tick],cx+dx*63-18,118+dy*63-7,36,14,10);
        }
        g.setColour(colour(0xff171c1f));
        text(g,knobs[static_cast<size_t>(i)].getTextFromValue(knobs[static_cast<size_t>(i)].getValue()),cx-53,174,106,16,11);
        text(g,titles[i],cx-64,197,128,21,14);
    }
    paintMeter(g);
    g.setColour(colour(0xff171c1f));
    const bool outputMode=processor.apvts.getRawParameterValue("meterMode")->load()>.5f;
    text(g,outputMode?"OUTPUT PEAK (dBFS)":"TOTAL GAIN REDUCTION",1012,117,360,15,11);
    text(g,juce::String(shownGr,1)+" dB GR     OUT "+juce::String(shownOut,1)+" dBFS",1012,133,360,13,10);
    const char* switchTitles[]{"DELTA","GR / OUT","FIX"};
    for(int i=0;i<3;++i){const float cx=1064+130.f*i;const bool active=switches[size_t(i+1)].getToggleState();
     text(g,switchTitles[i],cx-48,192,96,18,12);text(g,i==1?"GR":"OFF",cx-45,157,30,14,9);text(g,i==1?"OUT":"ON",cx+25,157,30,14,9);
     g.setColour(colour(active?0xffb84dff:0xff594364));g.fillEllipse(cx-5,215,10,4);g.setColour(colour(0xff171c1f));}
    // Hand-drawn vector script, independent of installed fonts.
    juce::Path signature;
    signature.startNewSubPath(0,22);signature.lineTo(12,2);signature.cubicTo(25,-3,25,7,13,12);signature.lineTo(7,12);
    signature.startNewSubPath(14,18);signature.cubicTo(21,6,29,9,22,18);signature.cubicTo(18,24,13,22,18,16);signature.lineTo(28,12);signature.lineTo(24,20);signature.lineTo(34,14);
    signature.startNewSubPath(29,29);signature.lineTo(39,10);signature.cubicTo(49,5,48,15,40,19);signature.lineTo(36,18);signature.lineTo(49,13);
    signature.startNewSubPath(51,10);signature.lineTo(46,20);signature.quadraticTo(49,23,57,14);
    signature.startNewSubPath(54,6);signature.lineTo(55,4);
    signature.startNewSubPath(54,17);signature.cubicTo(66,16,67,9,61,11);signature.cubicTo(50,17,56,26,68,17);
    signature.cubicTo(82,1,80,-3,73,8);signature.lineTo(68,19);signature.quadraticTo(68,23,78,16);
    signature.cubicTo(86,6,93,10,86,18);signature.cubicTo(79,26,77,16,84,13);signature.lineTo(91,11);signature.lineTo(81,29);signature.cubicTo(75,37,70,29,88,22);
    signature.lineTo(95,13);signature.lineTo(91,20);signature.quadraticTo(92,24,101,14);signature.lineTo(98,20);signature.quadraticTo(101,22,108,14);
    signature.cubicTo(116,7,122,10,115,18);signature.cubicTo(108,26,106,17,112,13);signature.lineTo(120,11);signature.lineTo(116,20);signature.lineTo(127,13);
    signature.startNewSubPath(121,29);signature.lineTo(132,10);signature.cubicTo(142,5,142,15,134,19);signature.lineTo(129,18);signature.lineTo(143,13);
    signature.cubicTo(153,6,156,12,148,19);signature.cubicTo(140,25,137,17,146,12);signature.quadraticTo(149,17,155,13);
    const auto transform=juce::AffineTransform::scale(0.80f,0.80f).translated(1436,107);
    g.setColour(juce::Colours::white.withAlpha(0.28f));g.strokePath(signature,juce::PathStrokeType(3.0f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded),transform.followedBy(juce::AffineTransform::translation(1,1)));
    g.setColour(colour(0xff111719));g.strokePath(signature,juce::PathStrokeType(3.0f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded),transform);
    juce::Path flourish;flourish.startNewSubPath(1447,138);flourish.lineTo(1572,130);flourish.lineTo(1522,141);flourish.lineTo(1536,133);
    g.strokePath(flourish,juce::PathStrokeType(1.6f));
}
FET67Editor::FET67Editor(FET67Processor& p):AudioProcessorEditor(p),processor(p),faceplate(p)
{
    addAndMakeVisible(faceplate);setResizable(true,true);setResizeLimits(817,126,4085,630);
    getConstrainer()->setFixedAspectRatio(static_cast<double>(kBaseWidth)/kBaseHeight);
    const int w=juce::jlimit(817,4085,processor.editorWidth.load());setSize(w,juce::roundToInt(static_cast<double>(w)*kBaseHeight/kBaseWidth));
}
void FET67Editor::resized()
{
    const float sx=static_cast<float>(getWidth())/kBaseWidth,sy=static_cast<float>(getHeight())/kBaseHeight;
    faceplate.setBounds(0,0,kBaseWidth,kBaseHeight);faceplate.setTransform(juce::AffineTransform::scale(juce::jmin(sx,sy)));
    processor.editorWidth.store(getWidth());processor.editorHeight.store(getHeight());
}
