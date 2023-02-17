/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEqAudioProcessorEditor::SimpleEqAudioProcessorEditor (SimpleEqAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    peakFreqSliderAttachement(audioProcessor.apvts, "Peak Freq",peakFreqSlider),
    peakGainSliderAttachement(audioProcessor.apvts, "Peak Gain",peakGainSlider),
    peakQualitySliderAttachement(audioProcessor.apvts, "Peak Quality",peakQualitySlider),
    highCutFreqSliderAttachement(audioProcessor.apvts, "HighCut Freq",highCutFreqSlider),
    highCutSlopeSliderAttachement(audioProcessor.apvts, "HighCut Slope",highCutSlopeSlider),
    lowCutFreqSliderAttachement(audioProcessor.apvts, "LowCut Freq",lowCutFreqSlider),
    lowCutSlopeSliderAttachement(audioProcessor.apvts, "LowCut Slope",lowCutSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for(auto comp : getComps()){
      addAndMakeVisible(comp);
    }
    setSize (600, 400);
}

SimpleEqAudioProcessorEditor::~SimpleEqAudioProcessorEditor()
{
}

//==============================================================================
void SimpleEqAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void SimpleEqAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    //on enleve un tier pour pouvoir afficher la reponse dedans
    auto responseArea = bounds.removeFromTop(bounds.getHeight()*0.33);
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth()*0.33);
    //on fait la moiter de ce qu'il reste tavu pas 1:3 du total
    auto highCutArea = bounds.removeFromRight(bounds.getWidth()*0.5);

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight()*0.5));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight()*0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutSlopeSlider.setBounds(highCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight()*0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight()*0.5));
    peakQualitySlider.setBounds(bounds);
}

std::vector<juce::Component*> SimpleEqAudioProcessorEditor::getComps(){
  return {
    &peakFreqSlider,
    &peakGainSlider,
    &peakQualitySlider,
    &lowCutFreqSlider,
    &highCutFreqSlider,
    &lowCutSlopeSlider,
    &highCutSlopeSlider
  };
}