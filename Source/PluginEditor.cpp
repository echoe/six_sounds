#include "PluginProcessor.h"
#include "PluginEditor.h"

MatrixFMSynthAudioProcessorEditor::MatrixFMSynthAudioProcessorEditor (MatrixFMSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), resizer (this, &constrainer)
{
    // 1. UI Window & Scaling
    addAndMakeVisible(uiScaleMenu);
    uiScaleMenu.addItemList({"50%", "75%", "100%", "125%", "150%"}, 1);
    uiScaleMenu.setSelectedId(3);
    uiScaleMenu.onChange = [this] {
        float scale = uiScaleMenu.getText().replace("%", "").getFloatValue() / 100.0f;
        setTransform(juce::AffineTransform::scale(scale));
    };

    addAndMakeVisible(resizer);
    constrainer.setMinimumSize(1200, 800);

    // 2. Global Group Titles
    envelopeGroup.setText("ENVELOPE"); addAndMakeVisible(envelopeGroup);
    toolsGroup.setText("PRESETS"); addAndMakeVisible(toolsGroup);
    matrixGroup.setText("MODULATION MATRIX"); addAndMakeVisible(matrixGroup);
    masterGroup.setText("OUTPUT"); addAndMakeVisible(masterGroup);

    // 3. Preset Buttons
    addAndMakeVisible(saveBtn); saveBtn.onClick = [&] { audioProcessor.savePreset(); };
    addAndMakeVisible(loadBtn); loadBtn.onClick = [&] { audioProcessor.loadPreset(); };
    addAndMakeVisible(initBtn); initBtn.onClick = [&] { audioProcessor.loadDefaultPreset(); };

    // 4. Matrix Tools & Visuals
    addAndMakeVisible(randomBtn); 
    randomBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred.darker(0.6f));
    randomBtn.onClick = [this] { audioProcessor.randomizeMatrix(); };

    addAndMakeVisible(visualizerLabel);
    visualizerLabel.setText("VISUALIZER", juce::dontSendNotification);
    visualizerLabel.setJustificationType(juce::Justification::centred);

    // 5. Master Section
    masterLabel.setText("MASTER", juce::dontSendNotification);
    masterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterLabel);

    addAndMakeVisible(masterSlider);
    masterSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    masterAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "master_out", masterSlider);

    // 6. ADSR
    juce::StringArray adsrNames = {"Attack", "Decay", "Sustain", "Release"};
    for (int i = 0; i < 4; ++i) {
        addAndMakeVisible(adsrSliders[i]);
        adsrSliders[i].setSliderStyle(juce::Slider::LinearVertical);
        adsrAtts[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, adsrNames[i].toLowerCase(), adsrSliders[i]);
        addAndMakeVisible(adsrLabels[i]);
        adsrLabels[i].setText(adsrNames[i], juce::dontSendNotification);
        adsrLabels[i].setJustificationType(juce::Justification::centred);
    }

    // 7. Operator Rows
    for (int i = 0; i < 6; ++i) {
        juce::String opId = "op" + juce::String(i);

        // --- Left side: Serial Volume ---
        auto* vl = volLabels.add(std::make_unique<juce::Label>("", "VOL"));
        vl->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(vl);
        auto* v = opGains.add(std::make_unique<juce::Slider>());
        v->setSliderStyle(juce::Slider::LinearVertical);
        addAndMakeVisible(v);
        atts.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, opId + "gain", *v));

        // --- Middle: Controls ---
        auto* r = opRatios.add(std::make_unique<juce::Slider>());
        r->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        r->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 45, 18);
        addAndMakeVisible(r);
        atts.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, opId + "ratio", *r));

        auto* m = opModes.add(std::make_unique<juce::ComboBox>());
        m->addItemList({"Sine", "LoPass", "Comb"}, 1);
        addAndMakeVisible(m);
        modeAtts.add(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, opId + "mode", *m));

        auto* cl1 = contextLabels1.add(std::make_unique<juce::Label>("", "PHASE"));
        addAndMakeVisible(cl1);
        auto* ck1 = opContextKnobs1.add(std::make_unique<juce::Slider>());
        ck1->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        addAndMakeVisible(ck1);
        atts.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, opId + "res", *ck1));

        auto* cl2 = contextLabels2.add(std::make_unique<juce::Label>("", ""));
        addAndMakeVisible(cl2);
        auto* ck2 = opContextKnobs2.add(std::make_unique<juce::Slider>());
        ck2->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        addAndMakeVisible(ck2);
        atts.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, opId + "res2", *ck2));

        // Logic for mode switching
        m->onChange = [m, r, cl1, cl2, ck2] {
            int mode = m->getSelectedId();
            r->setEnabled(mode == 1);
            if (mode == 1) { // Sine
                cl1->setText("PHASE", juce::dontSendNotification);
                cl2->setText("", juce::dontSendNotification);
                ck2->setVisible(false);
            } else if (mode == 2) { // LoPass
                cl1->setText("CUTOFF", juce::dontSendNotification);
                cl2->setText("RES", juce::dontSendNotification);
                ck2->setVisible(true);
            } else { // Comb
                cl1->setText("FEEDBACK", juce::dontSendNotification);
                cl2->setText("FREQ", juce::dontSendNotification);
                ck2->setVisible(true);
            }
        };
        m->onChange();

        // --- Matrix Grid ---
        for (int j = 0; j < 6; ++j) {
            auto* s = matrixKnobs.add(std::make_unique<juce::Slider>());
            s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            addAndMakeVisible(s);
            atts.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "m"+juce::String(i)+juce::String(j), *s));
        }

        // --- Right side: Parallel Volume Out ---
        auto* volOutL = volOutLabels.add(std::make_unique<juce::Label>("", "OUT"));
        volOutL->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(volOutL);
        auto* vo = opGainsOut.add(std::make_unique<juce::Slider>());
        vo->setSliderStyle(juce::Slider::LinearVertical);
        addAndMakeVisible(vo);
        atts.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, opId + "gainOut", *vo));

        // Row/Col Labels
        addAndMakeVisible(rowLabels[i]); rowLabels[i].setText("OP" + juce::String(i), juce::dontSendNotification);
        addAndMakeVisible(colLabels[i]); colLabels[i].setText("OP" + juce::String(i), juce::dontSendNotification);
        colLabels[i].setJustificationType(juce::Justification::centred);
    }

    addAndMakeVisible(audioProcessor.visualizer);
    addAndMakeVisible(meter);
    startTimerHz(30);
    setSize (1250, 850);
}

MatrixFMSynthAudioProcessorEditor::~MatrixFMSynthAudioProcessorEditor() { stopTimer(); }

void MatrixFMSynthAudioProcessorEditor::timerCallback() { meter.setLevel(audioProcessor.rmsLevelLeft); }

void MatrixFMSynthAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll (juce::Colours::black);
}

void MatrixFMSynthAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(15);
    resizer.setBounds(getWidth() - 20, getHeight() - 20, 20, 20);

    // --- Header ---
    auto header = area.removeFromTop(160);
    uiScaleMenu.setBounds(header.removeFromRight(80).removeFromTop(25));
    
    auto toolsArea = header.removeFromLeft(130);
    toolsGroup.setBounds(toolsArea);
    auto btnArea = toolsArea.reduced(10); btnArea.removeFromTop(15);
    saveBtn.setBounds(btnArea.removeFromTop(25)); btnArea.removeFromTop(5);
    loadBtn.setBounds(btnArea.removeFromTop(25)); btnArea.removeFromTop(5);
    initBtn.setBounds(btnArea.removeFromTop(25));

    auto masterArea = header.removeFromRight(110);
    masterGroup.setBounds(masterArea);
    masterLabel.setBounds(masterArea.removeFromTop(25));
    masterSlider.setBounds(masterArea.reduced(5));

    envelopeGroup.setBounds(header.removeFromLeft(380).reduced(2));
    auto adsrArea = envelopeGroup.getBounds().reduced(10); adsrArea.removeFromTop(15);
    for(int i=0; i<4; ++i) {
        auto col = adsrArea.removeFromLeft(adsrArea.getWidth()/(4-i));
        adsrLabels[i].setBounds(col.removeFromTop(20));
        adsrSliders[i].setBounds(col);
    }

    auto vizArea = header.reduced(5);
    visualizerLabel.setBounds(vizArea.removeFromTop(20));
    audioProcessor.visualizer.setBounds(vizArea);

    // --- Matrix Section ---
    area.removeFromTop(15);
    matrixGroup.setBounds(area);
    randomBtn.setBounds(matrixGroup.getRight() - 170, matrixGroup.getY() + 5, 160, 22);

    auto mGrid = area.reduced(20); mGrid.removeFromTop(35);
    auto colHeader = mGrid.removeFromTop(25);
    colHeader.removeFromLeft(460); // Offset for row labels and left controls
    
    // Position column labels over the matrix knobs
    auto matrixLabelWidth = colHeader.getWidth() - 60; // leave room for Vol Out
    for(int i=0; i<6; ++i) 
        colLabels[i].setBounds(matrixLabelWidth / 6 * i + 460, colHeader.getY(), matrixLabelWidth / 6, 25);

    int rowH = mGrid.getHeight() / 6;
    for (int i = 0; i < 6; ++i) {
        auto row = mGrid.removeFromTop(rowH);
        rowLabels[i].setBounds(row.removeFromLeft(35));
        
        // Serial Vol
        auto vArea = row.removeFromLeft(40);
        volLabels[i]->setBounds(vArea.removeFromTop(15));
        opGains[i]->setBounds(vArea);

        // Ratio & Mode
        opRatios[i]->setBounds(row.removeFromLeft(80).reduced(5));
        opModes[i]->setBounds(row.removeFromLeft(95).withSizeKeepingCentre(90, 24));
        
        // Context 1
        auto c1 = row.removeFromLeft(95);
        contextLabels1[i]->setBounds(c1.removeFromTop(15));
        opContextKnobs1[i]->setBounds(c1.reduced(5));

        // Context 2
        auto c2 = row.removeFromLeft(95);
        contextLabels2[i]->setBounds(c2.removeFromTop(15));
        opContextKnobs2[i]->setBounds(c2.reduced(5));

        // Matrix
        auto mKnobsArea = row.removeFromLeft(row.getWidth() - 50);
        int mWidth = mKnobsArea.getWidth() / 6;
        for (int j = 0; j < 6; ++j) {
            matrixKnobs[i * 6 + j]->setBounds(mKnobsArea.removeFromLeft(mWidth).reduced(8));
        }

        // Parallel Vol Out
        auto voArea = row.removeFromLeft(50);
        volOutLabels[i]->setBounds(voArea.removeFromTop(15));
        opGainsOut[i]->setBounds(voArea);
    }
}
