#pragma once
#include <QMainWindow>
#include <vector>
#include <functional>
#include "audio_params.h"

class QSlider;
class QComboBox;
class QLabel;
class QPushButton;
class QCheckBox;
class QAudioSink;
class QBuffer;
class QGridLayout;
class WaveformWidget;
class XYPad;
class StepSequencer;
class Knob;
class EnvelopeEditor;

struct Preset { QString name; Params p; };

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow() override;

private:
    Params params() const;
    void addSlider(QGridLayout* g, int r, int c, const QString& label,
                   int lo, int hi, int step, QSlider** out,
                   std::function<QString(int)> fmt);
    void rebuild();
    void resetToPreset();
    void applyPreset(const Preset& pr);
    void randomize();
    void play();
    void exportFile(const QString& ext);

    QSlider*   m_duration  = nullptr;
    QSlider*   m_frequency = nullptr;
    QSlider*   m_fm        = nullptr;
    QSlider*   m_noise     = nullptr;
    QSlider*   m_decay     = nullptr;
    QSlider*   m_gain      = nullptr;
    
    QSlider*   m_attack    = nullptr;
    QSlider*   m_sustain   = nullptr;
    QSlider*   m_release   = nullptr;
    
    QSlider*   m_vibRate   = nullptr;
    QSlider*   m_vibDepth  = nullptr;
    
    QSlider*   m_seamlessSlider = nullptr;
    QSlider*   m_arpRate   = nullptr;
    QSlider*   m_arpGate   = nullptr;

    QComboBox* m_filterTypeCombo = nullptr;
    QSlider*   m_cutoff    = nullptr;
    QSlider*   m_filterRes = nullptr;

    QComboBox* m_distTypeCombo = nullptr;
    QSlider*   m_distDrive = nullptr;

    QSlider*   m_bitDepth  = nullptr;
    QSlider*   m_srDiv     = nullptr;

    QSlider*   m_delayTime = nullptr;
    QSlider*   m_delayFb   = nullptr;
    QCheckBox* m_pingPong  = nullptr;
    QSlider*   m_reverbMix = nullptr;
    QSlider*   m_reverbSize= nullptr;

    QSlider*   m_chorusMix = nullptr;
    QSlider*   m_chorusDepth = nullptr;
    QSlider*   m_phaserMix = nullptr;
    QSlider*   m_tremoloRate = nullptr;
    QSlider*   m_ringModFreq = nullptr;

    QComboBox* m_waveCombo = nullptr;
    WaveformWidget* m_waveform = nullptr;
    QLabel*    m_status    = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QCheckBox* m_loopBtn   = nullptr;
    QAudioSink* m_audioSink = nullptr;
    QBuffer*   m_audioBuf   = nullptr;
    std::vector<float> m_signal;
    float m_slide = 0.0f;

    XYPad* m_xyPad = nullptr;
    StepSequencer* m_sequencer = nullptr;
    QCheckBox* m_stepModeBtn = nullptr;
    Knob* m_knobFreq = nullptr;
    Knob* m_knobGain = nullptr;
    Knob* m_knobDecay = nullptr;

    EnvelopeEditor* m_volEnvEditor = nullptr;
    EnvelopeEditor* m_pitchEnvEditor = nullptr;

    QCheckBox* m_drawModeBtn = nullptr;
    QPushButton* m_clearDrawBtn = nullptr;

    bool m_hasPreset = false;
    Preset m_currentPreset;
};