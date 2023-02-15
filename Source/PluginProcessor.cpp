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

//Si j ai bien compris en gros ca fait une copie du bail ???
    auto chainSettings = getChainSettings(apvts);
    //On crée le filtre avec tous les bons coefficients qu'on veut attention au gain
    updatePeakFilter(chainSettings);
    /**ICI ON REFACTORISE EN UTILISANT LA FONCTION AUX*/
    // auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
    //                                                     chainSettings.peakFreq
    //                                                     ,chainSettings.peakQuality,
    //                                                     juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
    // //Be carefull with the dereference as it s put on the heap 
    // *leftChain.get<ChainPosition::Peak>().coefficients = *peakCoefficients;
    // *rightChain.get<ChainPosition::Peak>().coefficients = *peakCoefficients;



    //On est parti pour faire le passe haut et passe bas :
    //On va faire un peu le zbeul avex notre choix de ordre car il faut des ordres de 2 4 6 8 alors que notre courbe donne des resultats de 0 a 3
   auto cutCoefficient =  juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,sampleRate,(chainSettings.lowCutSlope+1)*2);

//pourquoi ici on fait une reference ??
   auto& leftLowCut = leftChain.get<ChainPosition::LowCut>();

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


    auto& rightLowCut = rightChain.get<ChainPosition::LowCut>();
    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);
    switch(chainSettings.lowCutSlope ){
        case Slope_12 :
        {
            *rightLowCut.get<0>().coefficients = *cutCoefficient[0];
            rightLowCut.setBypassed<0>(false);
            break;
        }
        case Slope_24 :
        {
            *rightLowCut.get<0>().coefficients = *cutCoefficient[0];
            rightLowCut.setBypassed<0>(false);
             *rightLowCut.get<1>().coefficients = *cutCoefficient[1];
            rightLowCut.setBypassed<1>(false);
            break;
        }
        break;
        case Slope_32 :
        {
             *rightLowCut.get<0>().coefficients = *cutCoefficient[0];
            rightLowCut.setBypassed<0>(false);
             *rightLowCut.get<1>().coefficients = *cutCoefficient[1];
            rightLowCut.setBypassed<1>(false);
            *rightLowCut.get<2>().coefficients = *cutCoefficient[2];
            rightLowCut.setBypassed<2>(false);
            break;
        }
        break;
        case Slope_48 :
        {
                *rightLowCut.get<0>().coefficients = *cutCoefficient[0];
            rightLowCut.setBypassed<0>(false);
             *rightLowCut.get<1>().coefficients = *cutCoefficient[1];
            rightLowCut.setBypassed<1>(false);
            *rightLowCut.get<2>().coefficients = *cutCoefficient[2];
            rightLowCut.setBypassed<2>(false);
            *rightLowCut.get<3>().coefficients = *cutCoefficient[3];
            rightLowCut.setBypassed<3>(false);
            break;
        }
        break;
    }

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

     auto chainSettings = getChainSettings(apvts);
     updatePeakFilter(chainSettings);
    // //On crée le filtre avec tous les bons coefficients qu'on veut attention au gain
    // auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
    //                                                     chainSettings.peakFreq
    //                                                     ,chainSettings.peakQuality,
    //                                                     juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
    // //Be carefull with the dereference as it s put on the heap 
    // *leftChain.get<ChainPosition::Peak>().coefficients = *peakCoefficients;
    // *rightChain.get<ChainPosition::Peak>().coefficients = *peakCoefficients;


    auto cutCoefficient =  juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,getSampleRate(),(chainSettings.lowCutSlope+1)*2);

//pourquoi ici on fait une reference ??
   auto& leftLowCut = leftChain.get<ChainPosition::LowCut>();

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


    auto& rightLowCut = rightChain.get<ChainPosition::LowCut>();
    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);
    switch(chainSettings.lowCutSlope ){
        case Slope_12 :
        {
            *rightLowCut.get<0>().coefficients = *cutCoefficient[0];
            rightLowCut.setBypassed<0>(false);
            break;
        }
        case Slope_24 :
        {
            *rightLowCut.get<0>().coefficients = *cutCoefficient[0];
            rightLowCut.setBypassed<0>(false);
             *rightLowCut.get<1>().coefficients = *cutCoefficient[1];
            rightLowCut.setBypassed<1>(false);
            break;
        }
        break;
        case Slope_32 :
        {
             *rightLowCut.get<0>().coefficients = *cutCoefficient[0];
            rightLowCut.setBypassed<0>(false);
             *rightLowCut.get<1>().coefficients = *cutCoefficient[1];
            rightLowCut.setBypassed<1>(false);
            *rightLowCut.get<2>().coefficients = *cutCoefficient[2];
            rightLowCut.setBypassed<2>(false);
            break;
        }
        break;
        case Slope_48 :
        {
                *rightLowCut.get<0>().coefficients = *cutCoefficient[0];
            rightLowCut.setBypassed<0>(false);
             *rightLowCut.get<1>().coefficients = *cutCoefficient[1];
            rightLowCut.setBypassed<1>(false);
            *rightLowCut.get<2>().coefficients = *cutCoefficient[2];
            rightLowCut.setBypassed<2>(false);
            *rightLowCut.get<3>().coefficients = *cutCoefficient[3];
            rightLowCut.setBypassed<3>(false);
            break;
        }
        break;
    }

    //Here we create another instance of audioBlock to compute on it
   juce::dsp::AudioBlock<float> block(buffer);

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
    

}

//==============================================================================
bool SimpleEqAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *SimpleEqAudioProcessor::createEditor()
{
    //return new SimpleEqAudioProcessorEditor(*this);
    //-> This thing might allow us to see all the parameters
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEqAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEqAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
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


    return settings;
}
//Never forget to dereference so you can get the value because passed by ref
void SimpleEqAudioProcessor::updateCoefficients(Coefficients& old,const Coefficients& replacement){
    *old=*replacement;
}

void SimpleEqAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings){
     //On crée le filtre avec tous les bons coefficients qu'on veut attention au gain
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                        chainSettings.peakFreq
                                                        ,chainSettings.peakQuality,
                                                        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
    //Be carefull with the dereference as it s put on the heap 
    // *leftChain.get<ChainPosition::Peak>().coefficients = *peakCoefficients;
    // *rightChain.get<ChainPosition::Peak>().coefficients = *peakCoefficients;
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

    return layout;
};
