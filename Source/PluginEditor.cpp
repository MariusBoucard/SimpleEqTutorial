/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/


#include "PluginProcessor.h"
#include "PluginEditor.h"
//=================================================================================
void LookAndFeel::drawRotarySlider(juce::Graphics &g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float SliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider &slider)
{
 using namespace juce;
   auto bounds = Rectangle<float>(x, y, width, height);
  g.setColour(Colour(97u, 18u, 167u)); 
   g.fillEllipse(bounds);
g.setColour(Colour(255u, 154u, 1u));
   g.drawEllipse(bounds, 1.f);

  auto center = bounds.getCentre();
  // To rotate the thing, we define it in a path
  Path p;
  Rectangle<float> r;
  r.setLeft(center.getX()-2);
  r.setRight(center.getX()+2);
  r.setTop(bounds.getY());
  r.setBottom(center.getY());
  p.addRectangle(r);

  jassert(rotaryEndAngle>rotaryStartAngle);

  auto sliderAngRad = jmap(SliderPosProportional,0.f,1.f,rotaryStartAngle,rotaryEndAngle);
  p.applyTransform(AffineTransform().rotated(sliderAngRad,center.getX(),center.getY()));
  g.fillPath(p);
 }
void RotarySliderWithLabels::paint(juce::Graphics &g)
{
  using namespace juce;
  auto startAng = degreesToRadians(180.f + 45.f);
  auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
  auto range = getRange();
  auto sliderBounds = getSliderBounds();
  g.setColour(Colours::red);
  g.drawRect(getLocalBounds());
  g.setColour(Colours::yellow);
  g.drawRect(sliderBounds);

  getLookAndFeel().drawRotarySlider(g,
                                     sliderBounds.getX(),
                                      sliderBounds.getY(),
                                       sliderBounds.getWidth(), 
                                       sliderBounds.getHeight(),
                                    jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), startAng, endAng, *this);
  // Jmap permet de normaliser notre valeur entre 0 et 1 important car tous les sliders les memes
}
juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
  auto bounds = getLocalBounds();
  auto size = juce::jmin(bounds.getWidth(),bounds.getHeight());
  size -= getTextHeight()*2;
  juce::Rectangle<int> r;
  r.setSize(size,size);
  r.setCentre(bounds.getCentreX(),0);
  r.setY(2);
  return r;
}
//=================================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEqAudioProcessor &p) : audioProcessor(p)
{
  const auto &params = audioProcessor.getParameters();
  for (auto param : params)
  {
    param->addListener(this);
  }
  // dont forget it otherwise s actiove pas
  startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
  const auto &params = audioProcessor.getParameters();
  for (auto param : params)
  {
    param->removeListener(this);
  }
}
void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
  parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
  if (parametersChanged.compareAndSetBool(false, true))
  {
    // update the monochain
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPosition::Peak>().coefficients, peakCoefficients);

    auto lowCutCoefficient = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficient = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPosition::LowCut>(), lowCutCoefficient, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPosition::HighCut>(), highCutCoefficient, chainSettings.highCutSlope);
    // set a repaint
    repaint();
  }
}

void ResponseCurveComponent::paint(juce::Graphics &g)
{
  using namespace juce;
  // (Our component is opaque, so we must completely fill the background with a solid colour)
 // g.fillAll(Colours::black);

  auto responseArea = getLocalBounds();
  auto w = responseArea.getWidth();
  auto &lowcut = monoChain.get<ChainPosition::LowCut>();
  auto &peak = monoChain.get<ChainPosition::Peak>();
  auto &highcut = monoChain.get<ChainPosition::HighCut>();

  // COurbe drawng
  auto sampleRate = audioProcessor.getSampleRate();
  std::vector<double> mags;
  mags.resize(w);
  for (int i = 0; i < w; i++)
  {
    double mag = 1.f;
    auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
    if (!monoChain.isBypassed<ChainPosition::Peak>())
      mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

    if (!lowcut.isBypassed<0>())
      mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
    if (!lowcut.isBypassed<1>())
      mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
    if (!lowcut.isBypassed<2>())
      mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
    if (!lowcut.isBypassed<3>())
      mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

    if (!highcut.isBypassed<0>())
      mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

    if (!highcut.isBypassed<1>())
      mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

    if (!highcut.isBypassed<2>())
      mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

    if (!highcut.isBypassed<3>())
      mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

    mags[i] = Decibels::gainToDecibels(mag);
  }

  Path responseCurve;
  const double outputMin = responseArea.getBottom();
  const double outputMax = responseArea.getY();
  auto map = [outputMin, outputMax](double input)
  {
    return jmap(input, -24.0, 24.0, outputMin, outputMax);
  };

  // Ca c'est le premier point de notre ligne
  responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

  for (int i = 1; i < mags.size(); i++)
  {
    responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
  }
  g.setColour(Colours::orange);
  g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
  g.setColour(Colours::white);

  g.strokePath(responseCurve, PathStrokeType(2.f));
}

//==============================================================================
SimpleEqAudioProcessorEditor::SimpleEqAudioProcessorEditor(SimpleEqAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),

      peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
      peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
      peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), "Q"),
      highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
      highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/oct"),
      lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
      lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/oct"),
      responseCurveComponent(audioProcessor),
      peakFreqSliderAttachement(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
      peakGainSliderAttachement(audioProcessor.apvts, "Peak Gain", peakGainSlider),
      peakQualitySliderAttachement(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
      highCutFreqSliderAttachement(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
      highCutSlopeSliderAttachement(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),
      lowCutFreqSliderAttachement(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
      lowCutSlopeSliderAttachement(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider)
{
  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  for (auto comp : getComps())
  {
    addAndMakeVisible(comp);
  }

  setSize(600, 400);
}

SimpleEqAudioProcessorEditor::~SimpleEqAudioProcessorEditor()
{
}

//==============================================================================
void SimpleEqAudioProcessorEditor::paint(juce::Graphics &g)
{
  using namespace juce;
  // (Our component is opaque, so we must completely fill the background with a solid colour)
//  g.fillAll(Colours::black);
}

void SimpleEqAudioProcessorEditor::resized()
{
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..

  auto bounds = getLocalBounds();
  // on enleve un tier pour pouvoir afficher la reponse dedans
  auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
  auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
  
  // on fait la moiter de ce qu'il reste tavu pas 1:3 du total
  auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
  responseCurveComponent.setBounds(responseArea);
  lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
  highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
  lowCutSlopeSlider.setBounds(lowCutArea);
  highCutSlopeSlider.setBounds(highCutArea);

  peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
  peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
  peakQualitySlider.setBounds(bounds);
}

std::vector<juce::Component *> SimpleEqAudioProcessorEditor::getComps()
{
  return {
      &peakFreqSlider,
      &peakGainSlider,
      &peakQualitySlider,
      &lowCutFreqSlider,
      &highCutFreqSlider,
      &lowCutSlopeSlider,
      &highCutSlopeSlider,
      &responseCurveComponent};
}