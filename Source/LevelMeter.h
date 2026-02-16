#pragma once
#include <JuceHeader.h>

class LevelMeter : public juce::Component, public juce::Timer
{
public:
    LevelMeter() { startTimerHz(30); }
    void setLevel(float newLevel) { level = newLevel; }

    void paint(juce::Graphics& g) override {
        auto area = getLocalBounds().toFloat();
        g.setColour(juce::Colours::black);
        g.fillRect(area);

        float h = area.getHeight() * juce::jlimit(0.0f, 1.0f, level);
        auto gradient = juce::ColourGradient(juce::Colours::green, 0, area.getHeight(),
                                            juce::Colours::red, 0, 0, false);
        g.setGradientFill(gradient);
        g.fillRect(area.withTop(area.getHeight() - h));
    }

    void timerCallback() override { repaint(); }

private:
    float level = 0.0f;
};
