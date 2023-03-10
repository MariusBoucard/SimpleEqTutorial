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
auto enabled = slider.isEnabled();


  g.setColour(enabled ? Colour(97u, 18u, 167u) : Colours::darkgrey );
  g.fillEllipse(bounds);
  g.setColour(enabled ? Colour(255u, 154u, 1u) : Colours::grey);
  g.drawEllipse(bounds, 1.f); 

  // Ok we have to cast here to make sure we can use the inner function
  if (auto *rswl = dynamic_cast<RotarySliderWithLabels *>(&slider))
  {
    auto center = bounds.getCentre();
    // To rotate the thing, we define it in a path
    Path p;
    Rectangle<float> r;
    r.setLeft(center.getX() - 2);
    r.setRight(center.getX() + 2);
    r.setTop(bounds.getY());
    r.setBottom(center.getY() - rswl->getTextHeight() * 2);
    p.addRoundedRectangle(r, 2.f);

    jassert(rotaryEndAngle > rotaryStartAngle);

    auto sliderAngRad = jmap(SliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
    p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
    g.fillPath(p);
    g.setFont(rswl->getTextHeight());
    auto text = rswl->getDisplayString();
    auto strWidth = g.getCurrentFont().getStringWidth(text);
    r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
    r.setCentre(bounds.getCentre());
    g.setColour(enabled ? Colours::black : Colours::darkgrey);
    g.fillRect(r);
    g.setColour(enabled ? Colours::white : Colours::lightgrey);
    g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
  }
}

void LookAndFeel::drawToggleButton(juce::Graphics &g,
                                   juce::ToggleButton &toggleButton,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
  using namespace juce;
  if (auto *pb = dynamic_cast<PowerButton *>(&toggleButton))
  {

    Path powerButton;

    auto bounds = toggleButton.getLocalBounds();
    // g.setColour(Colours::red);
    // g.drawRect(bounds);
    auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
    auto r = bounds.withSizeKeepingCentre(size, size).toFloat();
    // Because radiant
    float ang = 30.f;

    size -= 6;
    powerButton.addCentredArc(r.getCentreX(),
                              r.getCentreY(),
                              size * 0.5f,
                              size * 0.5,
                              0.f,
                              degreesToRadians(ang), degreesToRadians(360.f - ang),
                              true);

    powerButton.startNewSubPath(r.getCentreX(), r.getY());
    powerButton.lineTo(r.getCentre());
    auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colours::green;
    g.setColour(color);

    PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
    g.strokePath(powerButton, pst);
    g.drawEllipse(r, 2);
  }
  else if (auto *pb = dynamic_cast<AnalyzerButton *>(&toggleButton))
  {
        auto color = ! toggleButton.getToggleState() ?  Colours::dimgrey : Colours::green;
        g.setColour(color);
        auto bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);
        // auto insertRect = bounds.reduced(4);
        // Path randomPath;
        // Random r;
        // randomPath.startNewSubPath(insertRect.getX(),insertRect.getY()+insertRect.getHeight()*r.nextFloat());

        // for(auto x = insertRect.getX()+1 ;x<insertRect.getRight();x+=2){
        //           randomPath.lineTo(x,insertRect.getY()+insertRect.getHeight()*r.nextFloat());

        // }

        g.strokePath(pb->randomPath,PathStrokeType(1.f));
  }
}

void RotarySliderWithLabels::paint(juce::Graphics &g)
{
  using namespace juce;
  auto startAng = degreesToRadians(180.f + 45.f);
  auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
  auto range = getRange();
  auto sliderBounds = getSliderBounds();
  // g.setColour(Colours::red);
  // g.drawRect(getLocalBounds());
  // g.setColour(Colours::yellow);
  // g.drawRect(sliderBounds);

  getLookAndFeel().drawRotarySlider(g,
                                    sliderBounds.getX(),
                                    sliderBounds.getY(),
                                    sliderBounds.getWidth(),
                                    sliderBounds.getHeight(),
                                    jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), startAng, endAng, *this);

  auto center = sliderBounds.toFloat().getCentre();
  auto radius = sliderBounds.getWidth() * 0.5f;
  g.setColour(Colour(0u, 172u, 1u));
  g.setFont(getTextHeight());
  auto numChoices = labels.size();
  for (int i = 0; i < numChoices; i++)
  {
    auto pos = labels[i].pos;
    jassert(0.f <= pos);
    jassert(pos <= 1.f);
    auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
    auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
    Rectangle<float> r;
    auto str = labels[i].label;
    r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
    r.setCentre(c);
    r.setY(r.getY() + getTextHeight());
    g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
  }
  // Jmap permet de normaliser notre valeur entre 0 et 1 important car tous les sliders les memes
}
juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
  auto bounds = getLocalBounds();
  auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
  size -= getTextHeight() * 2;
  juce::Rectangle<int> r;
  r.setSize(size, size);
  r.setCentre(bounds.getCentreX(), 0);
  r.setY(2);
  return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
  if (auto *choiceParam = dynamic_cast<juce::AudioParameterChoice *>(param))
  {
    return choiceParam->getCurrentChoiceName();
  }
  juce::String str;
  bool addK = false;
  if (auto *floatParam = dynamic_cast<juce::AudioParameterFloat *>(param))
  {
    float val = getValue();
    if (val > 999.f)
    {
      val /= 1000.f;
      addK = true;
    }
    str = juce::String(val, (addK ? 2 : 0));
  }
  else
  {
    jassertfalse; // Simply add some correctness to our code
  }
  if (suffix.isNotEmpty())
  {
    str << " ";
    if (addK)
    {
      str << "k";
    }
    str << suffix;
  }
  return str;
}
//=================================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEqAudioProcessor &p) : audioProcessor(p),
                                                                            leftPathProducer(audioProcessor.leftChannelFifo),
                                                                            rightPathProducer(audioProcessor.rightChannelFifo)
// leftChannelFifo(&audioProcessor.leftChannelFifo)
{
  const auto &params = audioProcessor.getParameters();
  for (auto param : params)
  {
    param->addListener(this);
  }

  updateChain();
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

//===========================Lets move from timer calleback in here
void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
  // FFT START HERE SEEMS HARDDDD
  juce::AudioBuffer<float> tempIncomingBuffer;

  while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
  {
    if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
    {
      auto size = tempIncomingBuffer.getNumSamples();
      // On commence a ecrire dans monobuffer en 0, on copy ce qu'il y a depuis size, puis on copie tout le reste - size car on va pas plus loin
      juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                        monoBuffer.getReadPointer(0, size),
                                        monoBuffer.getNumSamples() - size);

      // Puis on colle a la fin de notre monoBuffer ce qui vient du tempIncomingBUffer
      juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                        tempIncomingBuffer.getReadPointer(0, 0),
                                        size);

      leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
    }
  }

  /**
   *  if there are FFt data to pull
   * if we can pull a buffer then generate a path
   */
  const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();

  /*
  48000/2048 = 23 hz : this is the binwidth
  */
  const auto binWidth = sampleRate / (double)fftSize;

  while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
  {
    std::vector<float> fftData;
    if (leftChannelFFTDataGenerator.getFFTData(fftData))
    {
      pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
    }
  }

  /**
   * while there are path that can be pull, pull as many as we can
   * we only display the most recent one
   */
  while (pathProducer.getNumPathsAvailable())
  {
    pathProducer.getPath(leftChannelFFTPath);
  }
}
void ResponseCurveComponent::timerCallback()
{
  if(shouldShowFFTAnalisis){
      auto fftBounds = getAnalysisArea().toFloat();
  auto sampleRate = audioProcessor.getSampleRate();
  leftPathProducer.process(fftBounds, sampleRate);
  rightPathProducer.process(fftBounds, sampleRate);
  }


  if (parametersChanged.compareAndSetBool(false, true))
  {
    // update the monochain
    updateChain();
    // set a repaint
  }
  repaint();
}

void ResponseCurveComponent::updateChain()
{
  auto chainSettings = getChainSettings(audioProcessor.apvts);

  monoChain.setBypassed<ChainPosition::HighCut>(chainSettings.highCutBypassed);
  monoChain.setBypassed<ChainPosition::LowCut>(chainSettings.lowCutBypassed);
  monoChain.setBypassed<ChainPosition::Peak>(chainSettings.peakBypassed);

  auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
  updateCoefficients(monoChain.get<ChainPosition::Peak>().coefficients, peakCoefficients);

  auto lowCutCoefficient = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
  auto highCutCoefficient = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
  updateCutFilter(monoChain.get<ChainPosition::LowCut>(), lowCutCoefficient, chainSettings.lowCutSlope);
  updateCutFilter(monoChain.get<ChainPosition::HighCut>(), highCutCoefficient, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint(juce::Graphics &g)
{
  using namespace juce;
  // (Our component is opaque, so we must completely fill the background with a solid colour)
  // g.fillAll(Colours::black);

  g.drawImage(background, getLocalBounds().toFloat());
  auto responseArea = getAnalysisArea();
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

    if (!monoChain.isBypassed<ChainPosition::LowCut>())
    {
      if (!lowcut.isBypassed<0>())
        mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
      if (!lowcut.isBypassed<1>())
        mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
      if (!lowcut.isBypassed<2>())
        mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
      if (!lowcut.isBypassed<3>())
        mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
    }

    if (!monoChain.isBypassed<ChainPosition::HighCut>())
    {

      if (!highcut.isBypassed<0>())
        mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

      if (!highcut.isBypassed<1>())
        mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

      if (!highcut.isBypassed<2>())
        mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

      if (!highcut.isBypassed<3>())
        mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
    }
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

  if(shouldShowFFTAnalisis){
 auto leftChannelFFTPath = leftPathProducer.getPath();
  auto rightChannelFFTPath = rightPathProducer.getPath();

  leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()-10));
  g.setColour(Colours::skyblue);
  g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

  rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()-10));
  g.setColour(Colours::yellow);
  g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
  }
 

  g.setColour(Colours::orange);
  g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
  g.setColour(Colours::white);

  g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized()
{
  using namespace juce;
  background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
  Graphics g(background);
  Array<float> freqs{
      20, /*30, 40,*/ 50, 100,
      200, /*300, 400,*/ 500, 1000,
      2000, /*3000, 4000,*/ 5000, 10000, 20000

  };
  auto renderArea = getAnalysisArea();
  auto left = renderArea.getX();
  auto right = renderArea.getRight();
  auto top = renderArea.getY();
  auto bottom = renderArea.getBottom();
  auto width = renderArea.getWidth();

  Array<float> xs;
  for (auto f : freqs)
  {
    auto normX = mapFromLog10(f, 20.f, 20000.f);
    xs.add(left + width * normX);
  }

  g.setColour(Colours::dimgrey);
  for (auto x : xs)
  {
    // auto normX = mapFromLog10(f,20.f,20000.f);
    g.drawVerticalLine(x, top, bottom);
  }

  Array<float> gain{
      -24, -12, 0, 12, 24};
  for (auto db : gain)
  {
    auto y = jmap(db, -24.f, 24.f, float(bottom), float(top));
    g.setColour(db == 0.f ? Colours::purple : Colours::darkgrey);
    g.drawHorizontalLine(y, left, right);
  }
  //  g.drawRect(getAnalysisArea());

  g.setColour(Colours::lightgrey);
  const int fontHeight = 10;
  g.setFont(fontHeight);

  for (int i = 0; i < freqs.size(); i++)
  {
    auto f = freqs[i];
    auto x = xs[i];
    bool addK = false;
    String str;
    if (f > 999.f)
    {
      addK = true;
      f /= 1000.f;
    }

    str << f;
    if (addK)
    {
      str << "k";
    }
    str << "Hz";
    auto textWidth = g.getCurrentFont().getStringWidth(str);

    Rectangle<int> r;
    r.setSize(textWidth, fontHeight);
    r.setCentre(x, 0);
    r.setY(1);
    g.drawFittedText(str, r, juce::Justification::centred, 1);
  }

  for (auto db : gain)
  {
    auto y = jmap(db, -24.f, 24.f, float(bottom), float(top));
    String str;
    if (db > 0)
    {
      str << "+";
    }
    str << db;
    auto textWidth = g.getCurrentFont().getStringWidth(str);
    Rectangle<int> r;
    r.setSize(textWidth, fontHeight);
    r.setX(getWidth() - textWidth);
    r.setCentre(r.getCentreX(), y);

    g.setColour(db == 0.f ? Colours::purple : Colours::lightgrey);
    g.drawFittedText(str, r, juce::Justification::centred, 1);

    str.clear();
    str << (db - 24.f);
    textWidth = g.getCurrentFont().getStringWidth(str);
    r.setX(1);
    r.setSize(textWidth, fontHeight);
    g.setColour(Colours::lightgrey);
    g.drawFittedText(str, r, juce::Justification::centred, 1);
  }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
  auto bounds = getLocalBounds();
  // One instance of live constant per line attention
  //  bounds.reduce(10,
  //  8);
  bounds.removeFromTop(12);
  bounds.removeFromBottom(2);
  bounds.removeFromLeft(20);
  bounds.removeFromRight(20);

  return bounds;
}
juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
  auto bounds = getRenderArea();
  bounds.removeFromBottom(4);
  bounds.removeFromTop(4);
  return bounds;
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
      lowCutSlopeSliderAttachement(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),

      lowCutBypassButtonAttachement(audioProcessor.apvts, "LowCut Bypassed", lowCutBypassButton),
      peakBypassButtonAttachement(audioProcessor.apvts, "Peak Bypassed", peakBypassButton),

      highCutBypassButtonAttachement(audioProcessor.apvts, "HighCut Bypassed", highCutBypassButton),
      analyzerEnabledButtonAttachement(audioProcessor.apvts, "Analyzer Enabled", analyzerEnabledButton)
{
  // Add min and max for sliders :
  peakFreqSlider.labels.add({0.f, "20 hz"});
  peakFreqSlider.labels.add({1.f, "20 Khz"});

  lowCutFreqSlider.labels.add({0.f, "20 hz"});
  lowCutFreqSlider.labels.add({1.f, "20 Khz"});

  highCutFreqSlider.labels.add({0.f, "20 hz"});
  highCutFreqSlider.labels.add({1.f, "20 Khz"});

  peakGainSlider.labels.add({0.f, "-24 dB"});
  peakGainSlider.labels.add({1.f, "24 dB"});

  peakQualitySlider.labels.add({0.f, "0.1 Q"});
  peakQualitySlider.labels.add({1.f, "1 Q"});

  lowCutSlopeSlider.labels.add({0.f, "12 dB/oct"});
  lowCutSlopeSlider.labels.add({1.f, "48 dB/oct"});

  highCutSlopeSlider.labels.add({0.f, "12 dB/oct"});
  highCutSlopeSlider.labels.add({1.f, "48 dB/oct"});
  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  for (auto comp : getComps())
  {
    addAndMakeVisible(comp);
  }

  peakBypassButton.setLookAndFeel(&lnf);
  lowCutBypassButton.setLookAndFeel(&lnf);
  highCutBypassButton.setLookAndFeel(&lnf);
  analyzerEnabledButton.setLookAndFeel(&lnf);

auto safePtr = juce::Component::SafePointer<SimpleEqAudioProcessorEditor>(this);
//Lambda function
peakBypassButton.onClick = [safePtr](){
if (auto * comp = safePtr.getComponent()){
  auto bypassed = comp->peakBypassButton.getToggleState();
  comp->peakFreqSlider.setEnabled(!bypassed);
    comp->peakGainSlider.setEnabled(!bypassed);
  comp->peakQualitySlider.setEnabled(!bypassed);


}
};

lowCutBypassButton.onClick = [safePtr](){
if (auto * comp = safePtr.getComponent()){
  auto bypassed = comp->lowCutBypassButton.getToggleState();
  comp->lowCutFreqSlider.setEnabled(!bypassed);
    comp->lowCutSlopeSlider.setEnabled(!bypassed);


}
};


highCutBypassButton.onClick = [safePtr](){
if (auto * comp = safePtr.getComponent()){
  auto bypassed = comp->highCutBypassButton.getToggleState();
  comp->highCutFreqSlider.setEnabled(!bypassed);
    comp->highCutSlopeSlider.setEnabled(!bypassed);
}
};


analyzerEnabledButton.onClick = [safePtr](){
if (auto * comp = safePtr.getComponent()){
  auto enabled = comp->analyzerEnabledButton.getToggleState();
  comp->responseCurveComponent.toggleAnalysisEnablement(enabled);
}

};
  setSize(600, 480);
}

SimpleEqAudioProcessorEditor::~SimpleEqAudioProcessorEditor()
{
  peakBypassButton.setLookAndFeel(nullptr);
  lowCutBypassButton.setLookAndFeel(nullptr);
  highCutBypassButton.setLookAndFeel(nullptr);
    analyzerEnabledButton.setLookAndFeel(nullptr);

}

//==============================================================================
void SimpleEqAudioProcessorEditor::paint(juce::Graphics &g)
{
  using namespace juce;
  // (Our component is opaque, so we must completely fill the background with a solid colour)
  g.fillAll(Colours::black);
}

void SimpleEqAudioProcessorEditor::resized()
{
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..

  auto bounds = getLocalBounds();

  auto analyzerEnabledArea = bounds.removeFromTop(25);  
  analyzerEnabledArea.setWidth(100);
  analyzerEnabledArea.setX(5);
  analyzerEnabledArea.removeFromTop(2);
  analyzerEnabledButton.setBounds(analyzerEnabledArea);

  bounds.removeFromTop(5);
  float hRatio = 25.f / 100.f; // Juce_Live_Constant(33) -> Allow us to tweak in real time
  // on enleve un tier pour pouvoir afficher la reponse dedans
  auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

  bounds.removeFromTop(5);
  auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);

  // on fait la moiter de ce qu'il reste tavu pas 1:3 du total
  auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
  responseCurveComponent.setBounds(responseArea);

  lowCutBypassButton.setBounds(lowCutArea.removeFromTop(25));

  highCutBypassButton.setBounds(highCutArea.removeFromTop(25));
  lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
  highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
  lowCutSlopeSlider.setBounds(lowCutArea);
  highCutSlopeSlider.setBounds(highCutArea);

  peakBypassButton.setBounds(bounds.removeFromTop(25));
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
      &responseCurveComponent,

      &lowCutBypassButton,
      &highCutBypassButton,
      &peakBypassButton,
      &analyzerEnabledButton};
}