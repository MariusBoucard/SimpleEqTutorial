/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEqAudioProcessor::SimpleEqAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
#endif
{
}

SimpleEqAudioProcessor::~SimpleEqAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEqAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEqAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SimpleEqAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SimpleEqAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SimpleEqAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEqAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEqAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEqAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SimpleEqAudioProcessor::getProgramName(int index)
{
    return {};
}

void SimpleEqAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

//==============================================================================
void SimpleEqAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    //We can see that we always use juce object to pass data between juce object
    //Be carefull respecting this structure
    juce::dsp::ProcessSpec spec ;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels =1;
    spec.sampleRate = sampleRate;
    leftChain.prepare(spec);
    rightChain.prepare(spec);

    auto chainSettings = getChainSettings(apvts);

    updateFilter();

leftChannelFifo.prepare(samplesPerBlock);
rightChannelFifo.prepare(samplesPerBlock);
//Lambda funciton here
    osc.initialise([](float x) { return std::sin(x); });
    spec.numChannels = getTotalNumOutputChannels();
    osc.prepare(spec);
    osc.setFrequency(5000);
}

void SimpleEqAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEqAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif



void SimpleEqAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    updateFilter();
    //Pour pas entendre ce qui arrive
    
   juce::dsp::AudioBlock<float> block(buffer);
    //  buffer.clear();
    // juce::dsp::ProcessContextReplacing<float> stereoContext(block);
    // osc.process(stereoContext);
   //Then we have to split both of the stereo channels into mono :
   auto leftBlock = block.getSingleChannelBlock(0);
   auto rightBlock = block.getSingleChannelBlock(1);
   //On more complicated things like drumkit in midi we might have more channels right ?

   //Now lets create a context to wrap that blocks so the chain can use this to compute :
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    //then process thanks to our process chains :
    leftChain.process(leftContext);
    rightChain.process(rightContext);
    
    rightChannelFifo.update(buffer);
    leftChannelFifo.update(buffer);

}

//==============================================================================
bool SimpleEqAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *SimpleEqAudioProcessor::createEditor()
{
    //Back to editor without nothing to allow us to create it 
    return new SimpleEqAudioProcessorEditor(*this);
    //-> This thing might allow us to see all the parameters
   // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEqAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData,true);
    apvts.state.writeToStream(mos);
}

void SimpleEqAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree= juce::ValueTree::readFromData(data,sizeInBytes);
    if (tree.isValid()){
        apvts.replaceState(tree);
        updateFilter();
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEqAudioProcessor();
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    //That's great because with can get values not normalized, within the previous defined range

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.lowCutSlope = static_cast<Slope>( apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope =static_cast<Slope>( apvts.getRawParameterValue("HighCut Slope")->load());
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();


     settings.lowCutBypassed = apvts.getRawParameterValue("LowCut Bypassed")->load() > 0.5f;
    settings.highCutBypassed = apvts.getRawParameterValue("HighCut Bypassed")-> load() >0.5f;
    settings.peakBypassed = apvts.getRawParameterValue("Peak Bypassed")->load() > 0.5f;
    return settings;
}
//Never forget to dereference so you can get the value because passed by ref
void /*SimpleEqAudioProcessor::*/updateCoefficients(Coefficients& old,const Coefficients& replacement){
    *old=*replacement;
}

  void SimpleEqAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings){

    auto cutCoefficient = makeLowCutFilter(chainSettings,getSampleRate());
// //pourquoi ici on fait une reference ??
leftChain.setBypassed<ChainPosition::LowCut>(chainSettings.lowCutBypassed);

rightChain.setBypassed<ChainPosition::LowCut>(chainSettings.lowCutBypassed);

    auto& leftLowCut = leftChain.get<ChainPosition::LowCut>();
     updateCutFilter(leftLowCut,cutCoefficient,chainSettings.lowCutSlope);


    auto& rightLowCut = rightChain.get<ChainPosition::LowCut>();
     updateCutFilter(rightLowCut,cutCoefficient,chainSettings.lowCutSlope);
  }

  void SimpleEqAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings){
  auto highCutCoefficient = makeHighCutFilter(chainSettings,getSampleRate());
 auto& leftHighCut = leftChain.get<ChainPosition::HighCut>();

leftChain.setBypassed<ChainPosition::HighCut>(chainSettings.highCutBypassed);

rightChain.setBypassed<ChainPosition::HighCut>(chainSettings.highCutBypassed);
    updateCutFilter(leftHighCut,highCutCoefficient,chainSettings.highCutSlope);



    auto& rightHighCut = rightChain.get<ChainPosition::HighCut>();
    updateCutFilter(rightHighCut,highCutCoefficient,chainSettings.highCutSlope);
  }
void SimpleEqAudioProcessor::updateFilter(){
    auto chainSettings = getChainSettings(apvts);
    updateLowCutFilters(chainSettings);
    updateHighCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
}

Coefficients makePeakFilter(const ChainSettings& chainSettings,double sampleRate){
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                        chainSettings.peakFreq
                                                        ,chainSettings.peakQuality,
                                                        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}
void SimpleEqAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings){
     //On crée le filtre avec tous les bons coefficients qu'on veut attention au gain
    // auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
    //                                                     chainSettings.peakFreq
    //                                                     ,chainSettings.peakQuality,
    //                                                     juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
    //Be carefull with the dereference as it s put on the heap 
    auto peakCoefficients = makePeakFilter(chainSettings,getSampleRate());
    leftChain.setBypassed<ChainPosition::Peak>(chainSettings.peakBypassed);

rightChain.setBypassed<ChainPosition::Peak>(chainSettings.peakBypassed);
    updateCoefficients(leftChain.get<ChainPosition::Peak>().coefficients,peakCoefficients);
    updateCoefficients(rightChain.get<ChainPosition::Peak>().coefficients,peakCoefficients);



}
//=======================
/**
 * /3 differents kind of bands : high cut, low cut and bands
 * fist 2 ones we can controle slope anf freq
 * Freq,gain,Q for other
 *
 * AudioParameter is a class in juce that allow portability between different things
 */
juce::AudioProcessorValueTreeState::ParameterLayout SimpleEqAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    // Wants unique pointer soooooo ->
    // layout.add(std::make_unique<juce::AudioParameterFloat>(
    //     "LowCut Freq", "LowCut Freq", 20.f, 20000.f, 150));
    //Not a great way to do, even if easier because no skew
    // The skew value help us dimention the slider : <1.0, low freq expend, otherwise, high freq
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                        "LowCut Freq",
                                                        "LowCut Freq",
                                                        juce::NormalisableRange<float>(20.f,20000.f,1.f,0.25f),
                                                        20.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                        "HighCut Freq",
                                                        "HighCut Freq",
                                                        juce::NormalisableRange<float>(20.f,20000.f,1.f,0.25f),
                                                        20000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                        "Peak Freq",
                                                        "Peak Freq",
                                                        juce::NormalisableRange<float>(20.f,20000.f,1.f,0.25f),
                                                        750.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                        "Peak Gain",
                                                        "Peak Gain",
                                                        juce::NormalisableRange<float>(-24.f,24.f,0.1f,1.f),
                                                        0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
                                                        "Peak Quality",
                                                        "Peak Quality",
                                                        juce::NormalisableRange<float>(0.1f,10.f,0.05f,1.f),
                                                        1.f));
    //Just do this so we can reuse this but damn, juce has it's own stringarray wow
    juce::StringArray stringArray;
    for (int i=0;i<4;i++){
        juce::String str;
        str << (12 + i*12);
        str <<  " db/octave brotha";
        stringArray.add(str);
    }

    //Now we're gonna use the choise object use to let user select a subset of values
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope","LowCut Slope",stringArray,0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope","HighCut Slope",stringArray,0));

    layout.add(std::make_unique<juce::AudioParameterBool>("LowCut Bypassed","LowCut Bypass",false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Peak Bypassed","Peak Bypass",false));
    layout.add(std::make_unique<juce::AudioParameterBool>("HighCut Bypassed","HighCut Bypass",false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Analyzer Enabled","Analyzer Enabled",true));




    return layout;
};
