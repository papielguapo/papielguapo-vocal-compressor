#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

inline constexpr int kBaseWidth = 1634, kBaseHeight = 252;

class ValueEntrySlider final : public juce::Slider
{
public:
    bool inverted = false;
    double proportionOfLengthToValue(double p) override { return juce::Slider::proportionOfLengthToValue(inverted ? 1.0-p : p); }
    double valueToProportionOfLength(double v) override { const auto p=juce::Slider::valueToProportionOfLength(v); return inverted ? 1.0-p : p; }
    std::function<void()> onValueEntry;
    void mouseDown(const juce::MouseEvent& event) override
    { if(event.getNumberOfClicks()<2) juce::Slider::mouseDown(event); }
    void mouseDoubleClick(const juce::MouseEvent&) override { if(onValueEntry) onValueEntry(); }
};

class FET67LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class FaceplateComponent final : public juce::Component, private juce::Timer
{
public:
    explicit FaceplateComponent(FET67Processor&);
    ~FaceplateComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void paintMeter(juce::Graphics&);
    void beginValueEntry(size_t);
    void commitValueEntry();
    void cancelValueEntry();
    FET67Processor& processor;
    FET67LookAndFeel look;
    std::array<ValueEntrySlider,3> knobs;
    juce::TextEditor valueEntry;
    int editingKnob = -1;
    std::array<juce::ToggleButton,1> switches;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::array<std::unique_ptr<SA>,3> knobAttachments;
    std::array<std::unique_ptr<BA>,1> switchAttachments;
    float shownGr = 0, shownOut = -100;
    juce::TooltipWindow tooltips { this, 500 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FaceplateComponent)
};

class FET67Editor final : public juce::AudioProcessorEditor
{
public:
    explicit FET67Editor(FET67Processor&);
    void resized() override;
private:
    FET67Processor& processor;
    FaceplateComponent faceplate;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FET67Editor)
};
