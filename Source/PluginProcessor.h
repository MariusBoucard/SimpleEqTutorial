/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/**
 * We define a struct to regroup every single one of our parameters :
*/

struct ChainSettings
{
  float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{1.f};
  float lowCutFreq{ 0 }, highCutFreq{ 0 };
  int lowCutSlope{ 0 },highCutSlope{ 0 };
}; 
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);
//==============================================================================
/**
*/
class SimpleEqAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimpleEqAudioProcessor();
    ~SimpleEqAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    //========================================
    //We're gonna declare some variables in here so the GUI can access it (public)
    //Here for exemple the Audioprocessor takes a parameterlayout so we create a fuction that will return that
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{*this,nullptr,"Parameters", createParameterLayout()};
private:
    //We re defining here a lot of aliases to avoid doing extra stuff you know :
    using Filter =  juce::dsp::IIR::Filter<float>;

//4 filter so it cans do - 48 db because each is 12; So here we re using the precedent filters to declare a chain that will create our 
//Low cuts and high cuts filter
    using CutFilter = juce::dsp::ProcessorChain<Filter,Filter,Filter,Filter>;
//Lets create the moni chian we could use on every mono channel :
  using MonoChain = juce::dsp::ProcessorChain<CutFilter,Filter,CutFilter>;
  //To use this in stereo, we create 2 instances
  MonoChain leftChain, rightChain; 
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEqAudioProcessor)
};
