#pragma once

#include <JuceHeader.h>

/** * VoiceParams must be defined at the very top so that the 
 * FMVoice class knows what it is when it tries to use it.
 */
struct VoiceParams {
    float adsr[4];       
    int modes[6];        
    float ratios[6];     
    float gains[6];      // Serial "Volume" (to next op)
    float gainsOut[6];   // Parallel "Volume Out" (direct to master)
    float res[6];        
    float res2[6];       
    float matrix[6][6];  
};

/**
 * Individual Operator: Oscillators and Filters
 */
class Operator {
public:
    enum Mode { Sine = 0, LoPass, Comb };

    void prepare(double sr, int samplesPerBlock) {
        sampleRate = sr;
        juce::dsp::ProcessSpec spec { sr, (juce::uint32)samplesPerBlock, 1 };
        
        filter.prepare(spec);
        filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        
        delayLine.prepare(spec);
        delayLine.setMaximumDelayInSamples(sr * 0.2); // Slightly larger buffer for low freq comb
        
        reset();
    }

    void reset() {
        phase = 0.0f;
        filter.reset();
        delayLine.reset();
    }

    float process(float input, float hz, float pMod, float c1, float c2, int modeIdx) {
        Mode m = static_cast<Mode>(modeIdx);
        
        if (m == Sine) {
            float phaseOffset = c1 * juce::MathConstants<float>::twoPi;
            float out = std::sin(phase + pMod + phaseOffset);
            
            phase += (juce::MathConstants<float>::twoPi * hz) / (float)sampleRate;
            if (phase >= juce::MathConstants<float>::twoPi) phase -= juce::MathConstants<float>::twoPi;
            
            return out + input; 
        }
        
        if (m == LoPass) {
            float cutoff = juce::jlimit(20.0f, 20000.0f, juce::jmap(c1, 20.0f, 20000.0f));
            filter.setCutoffFrequency(cutoff);
            filter.setResonance(juce::jlimit(0.1f, 10.0f, (c2 * 9.9f) + 0.1f));
            return filter.processSample(0, input);
        }

        if (m == Comb) {
            float feedback = juce::jlimit(0.0f, 0.98f, c1);
            float combHz = juce::jlimit(20.0f, 10000.0f, juce::jmap(c2, 20.0f, 10000.0f));
            float delaySamples = (float)sampleRate / combHz;
            
            delayLine.setDelay(delaySamples);
            float delayed = delayLine.popSample(0);
            delayLine.pushSample(0, input + (delayed * feedback));
            return input + delayed;
        }

        return input;
    }

private:
    double sampleRate = 44100.0;
    float phase = 0.0f;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::DelayLine<float> delayLine { 50000 };
};

/**
 * Standard JUCE Sound definition
 */
struct FMSound : public juce::SynthesiserSound {
    FMSound() {}
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

/**
 * The Voice: Manages the chain of 6 Operators
 */
class FMVoice : public juce::SynthesiserVoice {
public:
    FMVoice() {
        for (int i = 0; i < 6; ++i) 
            ops[i] = std::make_unique<Operator>();
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override {
        return dynamic_cast<FMSound*> (sound) != nullptr;
    }

    void prepare(double sr, int samplesPerBlock) {
        adsr.setSampleRate(sr);
        for (auto& op : ops) op->prepare(sr, samplesPerBlock);
    }

    void setCurrentParams(const VoiceParams& p) { 
        params = p; 
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override {
        noteHz = (float)juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        
        juce::ADSR::Parameters adsrParams;
        adsrParams.attack  = params.adsr[0];
        adsrParams.decay   = params.adsr[1];
        adsrParams.sustain = params.adsr[2];
        adsrParams.release = params.adsr[3];
        
        adsr.setParameters(adsrParams);
        for (auto& op : ops) op->reset();
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override {
        adsr.noteOff();
        if (!allowTailOff) clearCurrentNote();
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override {
        if (!adsr.isActive()) return;

        for (int s = 0; s < numSamples; ++s) {
            float runningChainBuffer = 0.0f;
            float parallelOutputSum = 0.0f; 
            float opOuts[6] = { 0.0f }; 

            for (int i = 0; i < 6; ++i) {
                float phaseMod = 0.0f;
                for (int j = 0; j < 6; ++j) {
                    phaseMod += opOuts[j] * params.matrix[i][j];
                }

                float hz = noteHz * params.ratios[i];

                // Process if there is active gain in either output path
                if (params.gains[i] > 0.0001f || params.gainsOut[i] > 0.0001f) {
                    float processed = ops[i]->process(runningChainBuffer, hz, phaseMod, 
                                                     params.res[i], params.res2[i], params.modes[i]);
                    
                    parallelOutputSum += processed * params.gainsOut[i];
                    runningChainBuffer = processed * params.gains[i];
                    opOuts[i] = processed;
                }
            }

            float adsrVal = adsr.getNextSample();
            float finalSample = (parallelOutputSum + runningChainBuffer) * adsrVal * 0.2f;

            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
                outputBuffer.addSample(ch, startSample + s, finalSample);
            }

            if (adsrVal <= 0.001f && !adsr.isActive()) clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

private:
    std::unique_ptr<Operator> ops[6]; 
    juce::ADSR adsr;
    VoiceParams params;
    float noteHz = 0.0f;
};
