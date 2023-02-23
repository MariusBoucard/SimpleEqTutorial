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
//==============FFT Analyzer

enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
    /**
     produces the FFT data from an audio buffer.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();
        
        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());
        
        // first apply a windowing function to our data
        window->multiplyWithWindowingTable (fftData.data(), fftSize);       // [1]
        
        // then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform (fftData.data());  // [2]
        
        int numBins = (int)fftSize / 2;
        
        //normalize the fft values.
        for( int i = 0; i < numBins; ++i )
        {
            auto v = fftData[i];
//            fftData[i] /= (float) numBins;
            if( !std::isinf(v) && !std::isnan(v) )
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = v;
        }
        
        //convert them to decibels
        for( int i = 0; i < numBins; ++i )
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }
        
        fftDataFifo.push(fftData);
    }
    
    void changeOrder(FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>
        
        order = newOrder;
        auto fftSize = getFFTSize();
        
        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);
        
        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    
    Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
                      juce::Rectangle<float> fftBounds,
                      int fftSize,
                      float binWidth,
                      float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                              negativeInfinity, 0.f,
                              float(bottom+10),   top);
        };

        auto y = map(renderData[0]);

//        jassert( !std::isnan(y) && !std::isinf(y) );
        if( std::isnan(y) || std::isinf(y) )
            y = bottom;
        
        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

        for( int binNum = 1; binNum < numBins; binNum += pathResolution )
        {
            y = map(renderData[binNum]);

//            jassert( !std::isnan(y) && !std::isinf(y) );

            if( !std::isnan(y) && !std::isinf(y) )
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};


//================================================
struct LookAndFeel : juce::LookAndFeel_V4
{
  void drawRotarySlider(juce::Graphics &,
                        int x,
                        int y,
                        int width,
                        int height,
                        float sliderPosProportional,
                        float rotaryStartAngle,
                        float rotaryEndAngle,
                        juce::Slider &) override;
  void drawToggleButton(juce::Graphics &g,
                         juce::ToggleButton & toggleButton,
                           bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
};
struct RotarySliderWithLabels : juce::Slider
{
  RotarySliderWithLabels(juce::RangedAudioParameter &rap, const juce::String &unitsuffix) : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
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
  struct LabelPos
  {
    float pos;
    juce::String label;
  };
  juce::Array<LabelPos> labels;

  void paint(juce::Graphics &g) override;
  juce::Rectangle<int> getSliderBounds() const;
  int getTextHeight() const { return 14; }
  juce::String getDisplayString() const;

private:
  LookAndFeel lnf;
  juce::RangedAudioParameter *param;
  juce::String suffix;
};


struct PathProducer{
  PathProducer(SingleChannelSampleFifo<SimpleEqAudioProcessor::BlockType>& scsf) : 
  leftChannelFifo(&scsf)
  {
  leftChannelFFTDataGenerator.changeOrder(FFTOrder::order4096);
  monoBuffer.setSize(1,leftChannelFFTDataGenerator.getFFTSize());
  }
  void process(juce::Rectangle<float> fftBounds,double sampleRate);
  juce::Path getPath() const { return leftChannelFFTPath;}
  private:
   SingleChannelSampleFifo<SimpleEqAudioProcessor::BlockType>* leftChannelFifo;
  juce::AudioBuffer<float> monoBuffer;
  FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;
  AnalyzerPathGenerator<juce::Path> pathProducer; 
  juce::Path leftChannelFFTPath;
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
  void paint(juce::Graphics &g) override;
  void updateChain();
  void resized() override;

  void toggleAnalysisEnablement(bool enabled)
  {
    shouldShowFFTAnalisis = enabled;
  }
private:
  juce::Atomic<bool> parametersChanged{false};
  MonoChain monoChain;
  SimpleEqAudioProcessor &audioProcessor;
  juce::Image background;
  juce::Rectangle<int> getRenderArea();
  juce::Rectangle<int> getAnalysisArea();

  PathProducer leftPathProducer,rightPathProducer;
  bool shouldShowFFTAnalisis = true ;
 };

//==============================================================================
/**
 * 
 */
struct PowerButton : juce::ToggleButton { };

struct AnalyzerButton : juce::ToggleButton {
  void resized() override
  {
    auto bounds = getLocalBounds();
    auto insertRect = bounds.reduced(4);

    randomPath.clear();

     juce::Random r;
        randomPath.startNewSubPath(insertRect.getX(),insertRect.getY()+insertRect.getHeight()*r.nextFloat());

        for(auto x = insertRect.getX()+1 ;x<insertRect.getRight();x+=2){
                  randomPath.lineTo(x,insertRect.getY()+insertRect.getHeight()*r.nextFloat());

        }
  }
  juce::Path randomPath;

 };
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

      PowerButton lowCutBypassButton, highCutBypassButton,peakBypassButton;
      AnalyzerButton analyzerEnabledButton;

    using ButtonAttachement = APVTS::ButtonAttachment;
    ButtonAttachement lowCutBypassButtonAttachement, highCutBypassButtonAttachement,peakBypassButtonAttachement, analyzerEnabledButtonAttachement;
      
        ResponseCurveComponent responseCurveComponent;

        LookAndFeel lnf;
  // MonoChain monoChain;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEqAudioProcessorEditor)
};
