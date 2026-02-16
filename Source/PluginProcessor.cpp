#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SynthCore.h"

MatrixFMSynthAudioProcessor::MatrixFMSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
#endif
       apvts (*this, nullptr, "Parameters", createParams())
{
    // Fix: FMVoice is now concrete and can be instantiated
    for (int i = 0; i < 8; ++i) 
        synth.addVoice (new FMVoice());

    synth.addSound (new FMSound());
    visualizer.setRepaintRate (30);
    visualizer.setBufferSize (512);
}

MatrixFMSynthAudioProcessor::~MatrixFMSynthAudioProcessor() {}

void MatrixFMSynthAudioProcessor::randomizeMatrix()
{
    juce::Random r;
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            auto paramId = "m" + juce::String(i) + juce::String(j);
            if (auto* param = apvts.getParameter(paramId)) {
                float newVal = (r.nextFloat() > 0.7f) ? (r.nextFloat() * 0.8f) : 0.0f;
                param->setValueNotifyingHost(newVal);
            }
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MatrixFMSynthAudioProcessor::createParams()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Global Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>("attack", "Attack", 0.001f, 5.0f, 0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("decay", "Decay", 0.001f, 5.0f, 0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain", 0.0f, 1.0f, 0.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("release", "Release", 0.001f, 5.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("master_out", "Master", 0.0f, 1.0f, 0.5f));

    for (int i = 0; i < 6; ++i) {
        juce::String opId = "op" + juce::String(i);
        layout.add(std::make_unique<juce::AudioParameterInt>(opId + "mode", "Mode", 0, 2, 0));
        layout.add(std::make_unique<juce::AudioParameterFloat>(opId + "ratio", "Ratio", 0.0f, 16.0f, 1.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(opId + "gain", "Volume", 0.0f, 1.0f, (i == 0 ? 0.7f : 0.0f)));
        layout.add(std::make_unique<juce::AudioParameterFloat>(opId + "gainOut", "Volume Out", 0.0f, 1.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(opId + "res", "Context 1", 0.0f, 1.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(opId + "res2", "Context 2", 0.0f, 1.0f, 0.0f));

        for (int j = 0; j < 6; ++j)
            layout.add(std::make_unique<juce::AudioParameterFloat>("m" + juce::String(i) + juce::String(j), "Mod", 0.0f, 10.0f, 0.0f));
    }
    return layout;
}

void MatrixFMSynthAudioProcessor::prepareToPlay (double sr, int samples) {
    synth.setCurrentPlaybackSampleRate (sr);
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        if (auto* v = dynamic_cast<FMVoice*>(synth.getVoice(i))) 
            v->prepare(sr, samples);
    }
}

void MatrixFMSynthAudioProcessor::releaseResources() {}

void MatrixFMSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Fix: Fill the ADSR array safely
    VoiceParams vp;
    const char* adsrParams[] = { "attack", "decay", "sustain", "release" };
    for (int i = 0; i < 4; ++i)
        vp.adsr[i] = *apvts.getRawParameterValue(adsrParams[i]);

    for (int i = 0; i < 6; ++i) {
        juce::String opId = "op" + juce::String(i);
        vp.modes[i] = (int)*apvts.getRawParameterValue(opId + "mode");
        vp.ratios[i] = *apvts.getRawParameterValue(opId + "ratio");
        vp.gains[i] = *apvts.getRawParameterValue(opId + "gain");
        vp.gainsOut[i] = *apvts.getRawParameterValue(opId + "gainOut");
        vp.res[i] = *apvts.getRawParameterValue(opId + "res");
        vp.res2[i] = *apvts.getRawParameterValue(opId + "res2");
        for (int j = 0; j < 6; ++j)
            vp.matrix[i][j] = *apvts.getRawParameterValue("m" + juce::String(i) + juce::String(j));
    }

    for (int i = 0; i < synth.getNumVoices(); ++i) {
        if (auto* v = dynamic_cast<FMVoice*>(synth.getVoice(i))) 
            v->setCurrentParams(vp);
    }

    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
    
    float masterGain = *apvts.getRawParameterValue("master_out");
    buffer.applyGain(masterGain);
    
    rmsLevelLeft = buffer.getMagnitude(0, 0, buffer.getNumSamples());
    visualizer.pushBuffer (buffer);
}

juce::AudioProcessorEditor* MatrixFMSynthAudioProcessor::createEditor() { return new MatrixFMSynthAudioProcessorEditor (*this); }

void MatrixFMSynthAudioProcessor::getStateInformation (juce::MemoryBlock& d) { 
    auto s = apvts.copyState(); 
    std::unique_ptr<juce::XmlElement> x(s.createXml()); 
    copyXmlToBinary(*x, d); 
}

void MatrixFMSynthAudioProcessor::setStateInformation (const void* d, int s) { 
    std::unique_ptr<juce::XmlElement> x(getXmlFromBinary(d, s)); 
    if(x) apvts.replaceState(juce::ValueTree::fromXml(*x)); 
}

void MatrixFMSynthAudioProcessor::savePreset() {
    // 1. Initialize the member variable
    fileChooser = std::make_unique<juce::FileChooser> (
        "Save Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.xml"
    );

    // 2. Launch asynchronously
    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
    [this] (const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file != juce::File{}) {
            // Ensure .xml extension
            if (file.getFileExtension() != ".xml")
                file = file.withFileExtension(".xml");

            auto state = apvts.copyState();
            std::unique_ptr<juce::XmlElement> xml (state.createXml());
            xml->writeTo (file);
        }
    });
}

void MatrixFMSynthAudioProcessor::loadPreset() {
    fileChooser = std::make_unique<juce::FileChooser> (
        "Load Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.xml"
    );

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
    [this] (const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile()) {
            std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
            if (xml != nullptr) {
                apvts.replaceState (juce::ValueTree::fromXml (*xml));
            }
        }
    });
}

void MatrixFMSynthAudioProcessor::loadDefaultPreset() {
    // 1. Reset Global ADSR and Master
    if (auto* p = apvts.getParameter("attack")) p->setValueNotifyingHost(p->getDefaultValue());
    if (auto* p = apvts.getParameter("decay")) p->setValueNotifyingHost(p->getDefaultValue());
    if (auto* p = apvts.getParameter("sustain")) p->setValueNotifyingHost(p->getDefaultValue());
    if (auto* p = apvts.getParameter("release")) p->setValueNotifyingHost(p->getDefaultValue());
    if (auto* p = apvts.getParameter("master_out")) p->setValueNotifyingHost(p->getDefaultValue());

    // 2. Reset Operators
    for (int i = 0; i < 6; ++i) {
        juce::String opId = "op" + juce::String(i);
        // Reset Mode to Sine (0)
        if (auto* p = apvts.getParameter(opId + "mode")) p->setValueNotifyingHost(p->getDefaultValue());
        // Reset Ratio to 1.0
        if (auto* p = apvts.getParameter(opId + "ratio")) p->setValueNotifyingHost(p->getDefaultValue()); 
        // Reset Context Knobs
        if (auto* p = apvts.getParameter(opId + "res")) p->setValueNotifyingHost(p->getDefaultValue());
        if (auto* p = apvts.getParameter(opId + "res2")) p->setValueNotifyingHost(p->getDefaultValue());
        // Reset Level: OP0 is audible (0.7), others are silent (0.0)
        if (auto* p = apvts.getParameter(opId + "gain")) {
            float defaultGain = (i == 0) ? p->getNormalisableRange().convertTo0to1(0.7f) : 0.0f;
            p->setValueNotifyingHost(defaultGain);
        }
        // Reset Parallel "Volume Out" to 0.0
        if (auto* p = apvts.getParameter(opId + "gainOut")) {
            p->setValueNotifyingHost(0.0f);
        }
        // Reset the entire Matrix row for this operator
        for (int j = 0; j < 6; ++j) {
            if (auto* p = apvts.getParameter("m" + juce::String(i) + juce::String(j)))
                p->setValueNotifyingHost(0.0f);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MatrixFMSynthAudioProcessor(); }
