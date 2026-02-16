#pragma once
#include <JuceHeader.h>
#include "SynthCore.h"

class MatrixFMSynthAudioProcessor : public juce::AudioProcessor
{
public:
    MatrixFMSynthAudioProcessor();
    ~MatrixFMSynthAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MatrixFMSynth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return "Default"; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // The logic declarations
    void randomizeMatrix();
    void savePreset();
    void loadPreset();
    void loadDefaultPreset();

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioVisualiserComponent visualizer { 1 };
    float rmsLevelLeft = 0.0f;

private:
    juce::Synthesiser synth;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatrixFMSynthAudioProcessor)
};
