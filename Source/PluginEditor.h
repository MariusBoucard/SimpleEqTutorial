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
class SimpleEqAudioProcessorEditor  : public juce::AudioProcessorEditor,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
public:
    SimpleEqAudioProcessorEditor (SimpleEqAudioProcessor&);
    ~SimpleEqAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
   void parameterValueChanged (int parameterIndex, float newValue) override;

        /** Indicates that a parameter change gesture has started.

            E.g. if the user is dragging a slider, this would be called with gestureIsStarting
            being true when they first press the mouse button, and it will be called again with
            gestureIsStarting being false when they release it.

            IMPORTANT NOTE: This will be called synchronously, and many audio processors will
            call it during their audio callback. This means that not only has your handler code
            got to be completely thread-safe, but it's also got to be VERY fast, and avoid
            blocking. If you need to handle this event on your message thread, use this callback
            to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
            message thread.
        */
        void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override { }
        void timerCallback() override;
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
    juce::Atomic<bool> parametersChanged { false };
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

    MonoChain monoChain;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEqAudioProcessorEditor)
};
