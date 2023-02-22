/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert( std::is_same_v<T, juce::AudioBuffer<float>>,
                      "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for( auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                           numSamples,
                           false,   //clear everything?
                           true,    //including the extra space?
                           true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }
    
    void prepare(size_t numElements)
    {
        static_assert( std::is_same_v<T, std::vector<float>>,
                      "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for( auto& buffer : buffers )
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }
    
    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if( write.blockSize1 > 0 )
        {
            buffers[write.startIndex1] = t;
            return true;
        }
        
        return false;
    }
    
    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if( read.blockSize1 > 0 )
        {
            t = buffers[read.startIndex1];
            return true;
        }
        
        return false;
    }
    
    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo {Capacity};
};

enum Channel
{
    Right, //effectively 0
    Left //effectively 1
};

template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }
    
    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse );
        auto* channelPtr = buffer.getReadPointer(channelToUse);
        
        for( int i = 0; i < buffer.getNumSamples(); ++i )
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);
        
        bufferToFill.setSize(1,             //channel
                             bufferSize,    //num samples
                             false,         //keepExistingContent
                             true,          //clear extra space
                             true);         //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;
    
    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);
            
            fifoIndex = 0;
        }
        
        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

enum Slope
{
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
  float peakFreq{0}, peakGainInDecibels{0}, peakQuality{1.f};
  float lowCutFreq{0}, highCutFreq{20000};
  Slope lowCutSlope{Slope::Slope_12}, highCutSlope{Slope::Slope_12};
};
  enum ChainPosition
  {
    LowCut,
    Peak,
    HighCut
  };
  using Filter = juce::dsp::IIR::Filter<float>;

  using Coefficients = Filter::CoefficientsPtr;

  // Static method to avoid over us of the memory
  void updateCoefficients(Coefficients &old, const Coefficients &replacement);
  Coefficients makePeakFilter(const ChainSettings& chainSettings,double sampleRate);
 // We re defining here a lot of aliases to avoid doing extra stuff you know :
 template <int Index, typename ChainType, typename CoefficientType>
  void update(ChainType &chain, const CoefficientType &coefficient)
  {
    updateCoefficients(chain.get<Index>().coefficients, coefficient[Index]);
    chain.setBypassed<Index>(false);
  }
  // Damn here we go for the template function so it can be use wether by the low cut or the high cut
  template <typename ChainType, typename CoefficientType>
  void updateCutFilter(ChainType &leftLowCut,
                       const CoefficientType &cutCoefficient,
                       const Slope &lowCutSlope)
  {

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);
    // Petit trick de faire a l envers oiur pas generer plus de code que pévu
    switch (lowCutSlope)
    {
    case Slope_48:
    {
      update<3>(leftLowCut, cutCoefficient);
    }
    case Slope_32:
    {
      update<2>(leftLowCut, cutCoefficient);
    }
    // break;
    case Slope_24:
    {
      update<1>(leftLowCut, cutCoefficient);
    }
    case Slope_12:
    {
      update<0>(leftLowCut, cutCoefficient);
    }
    }
  }


  inline auto makeLowCutFilter(const ChainSettings& chainSettings,double sampleRate)
  {
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,sampleRate,(chainSettings.lowCutSlope+1)*2);
  }

  inline auto makeHighCutFilter(const ChainSettings& chainSettings,double sampleRate)
  {
        return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,sampleRate,(chainSettings.highCutSlope+1)*2);

  }
  // 4 filter so it cans do - 48 db because each is 12; So here we re using the precedent filters to declare a chain that will create our
  // Low cuts and high cuts filter
  using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
  // Lets create the moni chian we could use on every mono channel :
  using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts);
//==============================================================================
/**
 */
class SimpleEqAudioProcessor : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
    ,
                               public juce::AudioProcessorARAExtension
#endif
{
public:
  //==============================================================================
  SimpleEqAudioProcessor();
  ~SimpleEqAudioProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
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
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;
  //========================================
  // We're gonna declare some variables in here so the GUI can access it (public)
  // Here for exemple the Audioprocessor takes a parameterlayout so we create a fuction that will return that
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};
  using BlockType = juce::AudioBuffer<float>;
  SingleChannelSampleFifo<BlockType> leftChannelFifo {Channel::Left};
  SingleChannelSampleFifo<BlockType> rightChannelFifo{ Channel::Right};
  
private:
 
  // To use this in stereo, we create 2 instances
  MonoChain leftChain, rightChain;
  // Be carefull with the order as this this will be used to access the peak in the chain


  void updatePeakFilter(const ChainSettings &ChainSettings);
  // We declare an aliase on the coefficient to change them more easily
  
 

  //More refactor :
  void updateLowCutFilters(const ChainSettings& chainSettings);
  void updateHighCutFilters(const ChainSettings& chainSettings);

  void updateFilter();
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEqAudioProcessor)
};
