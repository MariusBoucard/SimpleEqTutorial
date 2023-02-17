/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
//=====================================
//Custom to create our sliders in the same way and not have to redo every thing again

struct CustomRotatorySlider : juce::Slider
{
  CustomRotatorySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
  juce::Slider::TextEntryBoxPosition::NoTextBox)
  {

  }
};

//==============================================================================
/**
*/
class SimpleEqAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleEqAudioProcessorEditor (SimpleEqAudioProcessor&);
    ~SimpleEqAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEqAudioProcessor& audioProcessor;
    CustomRotatorySlider peakFreqSlider,
    peakGainSlider,
    peakQualitySlider,
    lowCutFreqSlider,
    highCutFreqSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider;

    //Okay we're gonna put them all in a slider so we can iterate over them :
    std::vector<juce::Component*> getComps();

    //Set some aliases to avoid having to type real meaning of apvts :
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachement = APVTS::SliderAttachment;
    Attachement peakFreqSliderAttachement,
    peakGainSliderAttachement,
    peakQualitySliderAttachement,
    lowCutFreqSliderAttachement,
    highCutFreqSliderAttachement,
    lowCutSlopeSliderAttachement,
    highCutSlopeSliderAttachement;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEqAudioProcessorEditor)
};
