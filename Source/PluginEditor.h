#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MatrixFMSynthAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    MatrixFMSynthAudioProcessorEditor (MatrixFMSynthAudioProcessor&);
    ~MatrixFMSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    MatrixFMSynthAudioProcessor& audioProcessor;

    // Groups
    juce::GroupComponent envelopeGroup;
    juce::GroupComponent toolsGroup;
    juce::GroupComponent matrixGroup;
    juce::GroupComponent masterGroup;

    // Header Controls
    juce::ComboBox uiScaleMenu;
    juce::TextButton saveBtn { "SAVE" }, loadBtn { "LOAD" }, initBtn { "INIT" };
    juce::TextButton randomBtn { "RANDOMIZE MATRIX" };
    juce::Label visualizerLabel;

    // Master Section
    juce::Slider masterSlider;
    juce::Label masterLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAtt;

    // ADSR
    juce::Slider adsrSliders[4];
    juce::Label adsrLabels[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> adsrAtts[4];

    // Operator Row Components
    juce::OwnedArray<juce::Slider> opGains;        // Serial "Volume"
    juce::OwnedArray<juce::Label> volLabels;      // "VOL" Labels
    
    juce::OwnedArray<juce::Slider> opRatios;
    juce::OwnedArray<juce::ComboBox> opModes;
    
    juce::OwnedArray<juce::Slider> opContextKnobs1;
    juce::OwnedArray<juce::Label> contextLabels1;
    
    juce::OwnedArray<juce::Slider> opContextKnobs2;
    juce::OwnedArray<juce::Label> contextLabels2;

    juce::OwnedArray<juce::Slider> matrixKnobs;
    
    juce::OwnedArray<juce::Slider> opGainsOut;     // Parallel "Volume Out"
    juce::OwnedArray<juce::Label> volOutLabels;   // "OUT" Labels

    juce::Label rowLabels[6];
    juce::Label colLabels[6];

    // Simple Level Meter
    struct SimpleMeter : public juce::Component {
        float level = 0.0f;
        void setLevel(float l) { level = l; repaint(); }
        void paint(juce::Graphics& g) override {
            g.fillAll(juce::Colours::black);
            g.setColour(juce::Colours::green.withAlpha(0.8f));
            g.fillRect(0, (int)(getHeight()*(1.0f-level)), getWidth(), (int)(getHeight()*level));
        }
    } meter;

    // Attachments
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> atts;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtts;

    // Window Management
    juce::ResizableCornerComponent resizer;
    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MatrixFMSynthAudioProcessorEditor)
};
