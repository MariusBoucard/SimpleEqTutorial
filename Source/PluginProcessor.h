/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope {
  Slope_12,
  Slope_24,
  Slope_32,
  Slope_48

};
/**
 * We define a struct to regroup every single one of our parameters :
*/

struct ChainSettings
{
  float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{1.f};
  float lowCutFreq{ 0 }, highCutFreq{ 0 };
  Slope lowCutSlope{ Slope::Slope_12 },highCutSlope{  Slope::Slope_12 };
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
  //Be carefull with the order as this this will be used to access the peak in the chain
  enum ChainPosition { 
    LowCut,
    Peak,
    HighCut
  };

  void updatePeakFilter(const ChainSettings& ChainSettings);
  //We declare an aliase on the coefficient to change them more easily
  using Coefficients = Filter::CoefficientsPtr;

  //Static method to avoid over us of the memory
  static void updateCoefficients(Coefficients& old,const Coefficients& replacement);

  //Damn here we go for the template function so it can be use wether by the low cut or the high cut
  template<typename ChainType, typename CoefficientType>
  void updateCutFilter(ChainType& leftLowCut, 
                        const CoefficientType& cutCoefficient,
                        const ChainSettings chainSettings){
                         // auto cutCoefficient =  juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,getSampleRate(),(chainSettings.lowCutSlope+1)*2);

//pourquoi ici on fait une reference ??
  //  auto& leftLowCut = leftChain.get<ChainPosition::LowCut>();

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    switch(chainSettings.lowCutSlope ){
        case Slope_12 :
        {
            *leftLowCut.get<0>().coefficients = *cutCoefficient[0];
            leftLowCut.setBypassed<0>(false);
            break;
        }
        case Slope_24 :
        {
            *leftLowCut.get<0>().coefficients = *cutCoefficient[0];
            leftLowCut.setBypassed<0>(false);
             *leftLowCut.get<1>().coefficients = *cutCoefficient[1];
            leftLowCut.setBypassed<1>(false);
            break;
        }
        break;
        case Slope_32 :
        {
             *leftLowCut.get<0>().coefficients = *cutCoefficient[0];
            leftLowCut.setBypassed<0>(false);
             *leftLowCut.get<1>().coefficients = *cutCoefficient[1];
            leftLowCut.setBypassed<1>(false);
            *leftLowCut.get<2>().coefficients = *cutCoefficient[2];
            leftLowCut.setBypassed<2>(false);
            break;
        }
        break;
        case Slope_48 :
        {
                *leftLowCut.get<0>().coefficients = *cutCoefficient[0];
            leftLowCut.setBypassed<0>(false);
             *leftLowCut.get<1>().coefficients = *cutCoefficient[1];
            leftLowCut.setBypassed<1>(false);
            *leftLowCut.get<2>().coefficients = *cutCoefficient[2];
            leftLowCut.setBypassed<2>(false);
            *leftLowCut.get<3>().coefficients = *cutCoefficient[3];
            leftLowCut.setBypassed<3>(false);
            break;
        }
        break;
    }
                        }
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEqAudioProcessor)
};
