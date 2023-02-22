/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
//=====================================
// Custom to create our sliders in the same way and not have to redo every thing again

struct LookAndFeel : juce::LookAndFeel_V4
{
  void drawRotarySlider(juce::Graphics&,
   int x,
    int y,
     int width,
     int height,
     float sliderPosProportional,
     float rotaryStartAngle,
  float rotaryEndAngle,
   juce::Slider&) override ;
};
struct RotarySliderWithLabels : juce::Slider
{
  RotarySliderWithLabels(juce::RangedAudioParameter& rap,const juce::String& unitsuffix) :
   juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                        juce::Slider::TextEntryBoxPosition::NoTextBox),
                                param(&rap),
                                suffix(unitsuffix)
  { 
    setLookAndFeel(&lnf);
  }

  ~RotarySliderWithLabels()
  {
    setLookAndFeel(nullptr);
  }
  void paint(juce::Graphics& g) override ;
  juce::Rectangle<int> getSliderBounds() const;
  int getTextHeight() const { return 14;}
  juce::String getDisplayString() const;
  private:
  LookAndFeel lnf;
  juce::RangedAudioParameter* param;
  juce::String suffix;
};

struct ResponseCurveComponent : juce::Component,
                                juce::AudioProcessorParameter::Listener,
                                juce::Timer
{
  ResponseCurveComponent(SimpleEqAudioProcessor &);
  ~ResponseCurveComponent();
  void parameterValueChanged(int parameterIndex, float newValue) override;

  void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
  void timerCallback() override;
  void paint(juce::Graphics& g) override;
  private:
    juce::Atomic<bool> parametersChanged{false};
  MonoChain monoChain;
  SimpleEqAudioProcessor& audioProcessor;
};

//==============================================================================
/**
 */
class SimpleEqAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
  SimpleEqAudioProcessorEditor(SimpleEqAudioProcessor &);
  ~SimpleEqAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;
  

private:
  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  SimpleEqAudioProcessor &audioProcessor;
  RotarySliderWithLabels peakFreqSlider,
      peakGainSlider,
      peakQualitySlider,
      lowCutFreqSlider,
      highCutFreqSlider,
      lowCutSlopeSlider,
      highCutSlopeSlider;
  // juce::Atomic<bool> parametersChanged{false};
  // Okay we're gonna put them all in a slider so we can iterate over them :
  std::vector<juce::Component *> getComps();

  // Set some aliases to avoid having to type real meaning of apvts :
  using APVTS = juce::AudioProcessorValueTreeState;
  using Attachement = APVTS::SliderAttachment;
  Attachement peakFreqSliderAttachement,
      peakGainSliderAttachement,
      peakQualitySliderAttachement,
      lowCutFreqSliderAttachement,
      highCutFreqSliderAttachement,
      lowCutSlopeSliderAttachement,
      highCutSlopeSliderAttachement;


  ResponseCurveComponent responseCurveComponent;
  // MonoChain monoChain;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEqAudioProcessorEditor)
};
