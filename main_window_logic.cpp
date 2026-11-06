#include "main_window.h"
#include "audio_engine.h"
#include "audio_export.h"
#include "custom_widgets.h"
#include "waveform_widget.h"   // <--- Добавить для WaveformWidget
#include <QLabel>              // <--- Добавить для QLabel

#include <QSlider>
#include <QComboBox>
#include <QCheckBox>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QBuffer>
#include <QFileDialog>
#include <QFileInfo>
#include <algorithm>
#include <random>
#include <chrono>

Params MainWindow::params() const {
    Params p;
    p.duration  = m_duration->value() / 100.0f;
    p.frequency = (float)m_frequency->value();
    p.fm        = (float)m_fm->value();
    p.noise     = m_noise->value() / 100.0f;
    p.decay     = m_decay->value() / 100.0f;
    p.gain      = m_gain->value() / 100.0f;
    p.waveform  = m_waveCombo->currentText();
    p.slide     = m_slide;
    
    p.attack    = m_attack->value() / 1000.0f;
    p.sustain   = m_sustain->value() / 100.0f;
    p.release   = m_release->value() / 1000.0f;
    p.vibrato_rate  = m_vibRate->value() / 10.0f;
    p.vibrato_depth = (float)m_vibDepth->value();
    
    p.seamless_crossfade = m_seamlessSlider->value() / 1000.0f;
    p.arp_rate = (float)m_arpRate->value();
    p.arp_gate = m_arpGate->value() / 100.0f;

    p.filter_type   = m_filterTypeCombo->currentIndex();
    p.filter_cutoff = (float)m_cutoff->value();
    p.filter_resonance = m_filterRes->value() / 100.0f;
    
    p.dist_type     = m_distTypeCombo->currentIndex();
    p.dist_drive    = m_distDrive->value() / 100.0f;
    
    p.bit_depth     = (float)m_bitDepth->value();
    p.sample_rate_div = (float)m_srDiv->value();
    
    p.delay_time    = (float)m_delayTime->value();
    p.delay_feedback= m_delayFb->value() / 100.0f;
    p.ping_pong     = m_pingPong->isChecked();
    p.reverb_mix    = m_reverbMix->value() / 100.0f;
    p.reverb_size   = m_reverbSize->value() / 100.0f;

    p.chorus_mix    = m_chorusMix->value() / 100.0f;
    p.chorus_depth  = (float)m_chorusDepth->value();
    p.phaser_mix    = m_phaserMix->value() / 100.0f;
    p.tremolo_rate  = (float)m_tremoloRate->value();
    p.ring_mod_freq = (float)m_ringModFreq->value();

    p.step_mode = m_stepModeBtn->isChecked();
    for (int i = 0; i < 16; ++i) {
        p.steps[i] = m_sequencer->isStepOn(i);
    }

    p.vol_env = m_volEnvEditor->points();
    p.pitch_env = m_pitchEnvEditor->points();
    p.drawn_env = m_waveform->getDrawnEnvelope();
    return p;
}

void MainWindow::rebuild() {
    m_signal = synthesize(params());
    m_waveform->setSignal(&m_signal);
    if (m_audioSink) m_audioSink->stop();
    
    if (m_knobFreq) m_knobFreq->setValue(m_frequency->value());
    if (m_knobGain) m_knobGain->setValue(m_gain->value());
    if (m_knobDecay) m_knobDecay->setValue(m_decay->value());
}

void MainWindow::resetToPreset() {
    if (m_hasPreset) applyPreset(m_currentPreset);
}

void MainWindow::applyPreset(const Preset& pr) {
    QList<QSlider*> sliders = {m_duration, m_frequency, m_fm, m_noise, m_decay, m_gain, m_attack, m_sustain, m_release, m_vibRate, m_vibDepth, m_seamlessSlider, m_arpRate, m_arpGate, m_cutoff, m_filterRes, m_distDrive, m_bitDepth, m_srDiv, m_delayTime, m_delayFb, m_reverbMix, m_reverbSize, m_chorusMix, m_chorusDepth, m_phaserMix, m_tremoloRate, m_ringModFreq};
    for (auto* s : sliders) s->blockSignals(true);
    m_waveCombo->blockSignals(true);
    m_filterTypeCombo->blockSignals(true);
    m_distTypeCombo->blockSignals(true);
    m_pingPong->blockSignals(true);

    m_duration->setValue((int)(pr.p.duration * 100));
    m_frequency->setValue((int)pr.p.frequency);
    m_fm->setValue((int)pr.p.fm);
    m_noise->setValue((int)(pr.p.noise * 100));
    m_decay->setValue((int)(pr.p.decay * 100));
    m_gain->setValue((int)(pr.p.gain * 100));
    m_waveCombo->setCurrentText(pr.p.waveform);
    m_slide = pr.p.slide;
    
    m_attack->setValue((int)(pr.p.attack * 1000));
    m_sustain->setValue((int)(pr.p.sustain * 100));
    m_release->setValue((int)(pr.p.release * 1000));
    m_vibRate->setValue((int)(pr.p.vibrato_rate * 10));
    m_vibDepth->setValue((int)pr.p.vibrato_depth);
    
    m_seamlessSlider->setValue((int)(pr.p.seamless_crossfade * 1000));
    m_loopBtn->setChecked(false);
    m_arpRate->setValue((int)pr.p.arp_rate);
    m_arpGate->setValue((int)(pr.p.arp_gate * 100));

    m_filterTypeCombo->setCurrentIndex(pr.p.filter_type);
    m_cutoff->setValue((int)pr.p.filter_cutoff);
    m_filterRes->setValue((int)(pr.p.filter_resonance * 100));
    
    m_distTypeCombo->setCurrentIndex(pr.p.dist_type);
    m_distDrive->setValue((int)(pr.p.dist_drive * 100));
    
    m_bitDepth->setValue((int)pr.p.bit_depth);
    m_srDiv->setValue((int)pr.p.sample_rate_div);
    
    m_delayTime->setValue((int)pr.p.delay_time);
    m_delayFb->setValue((int)(pr.p.delay_feedback * 100));
    m_pingPong->setChecked(pr.p.ping_pong);
    m_reverbMix->setValue((int)(pr.p.reverb_mix * 100));
    m_reverbSize->setValue((int)(pr.p.reverb_size * 100));

    m_chorusMix->setValue((int)(pr.p.chorus_mix * 100));
    m_chorusDepth->setValue((int)pr.p.chorus_depth);
    m_phaserMix->setValue((int)(pr.p.phaser_mix * 100));
    m_tremoloRate->setValue((int)pr.p.tremolo_rate);
    m_ringModFreq->setValue((int)pr.p.ring_mod_freq);

    m_volEnvEditor->setPoints(pr.p.vol_env);
    m_pitchEnvEditor->setPoints(pr.p.pitch_env);
    m_waveform->clearDrawing();

    for (auto* s : sliders) s->blockSignals(false);
    m_waveCombo->blockSignals(false);
    m_filterTypeCombo->blockSignals(false);
    m_distTypeCombo->blockSignals(false);
    m_pingPong->blockSignals(false);
    
    rebuild();
}

void MainWindow::randomize() {
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 rng(static_cast<unsigned int>(seed));
    std::uniform_int_distribution<int> d(0, 100);
    
    QList<QSlider*> sliders = {m_duration, m_frequency, m_fm, m_noise, m_decay, m_gain, m_attack, m_sustain, m_release, m_vibRate, m_vibDepth, m_seamlessSlider, m_arpRate, m_arpGate, m_cutoff, m_filterRes, m_distDrive, m_bitDepth, m_srDiv, m_delayTime, m_delayFb, m_reverbMix, m_reverbSize, m_chorusMix, m_chorusDepth, m_phaserMix, m_tremoloRate, m_ringModFreq};
    for (auto* s : sliders) s->blockSignals(true);
    m_waveCombo->blockSignals(true);
    m_filterTypeCombo->blockSignals(true);
    m_distTypeCombo->blockSignals(true);
    m_pingPong->blockSignals(true);

    m_duration->setValue(10 + d(rng) * 3 / 2);
    m_frequency->setValue(60 + std::uniform_int_distribution<int>(0, 1900)(rng));
    m_fm->setValue(std::uniform_int_distribution<int>(0, 300)(rng));
    m_noise->setValue(std::uniform_int_distribution<int>(0, 80)(rng));
    m_decay->setValue(5 + std::uniform_int_distribution<int>(0, 70)(rng));
    m_gain->setValue(50 + std::uniform_int_distribution<int>(0, 100)(rng));
    QStringList wf = {"sine", "square", "sawtooth", "triangle"};
    m_waveCombo->setCurrentText(wf[std::uniform_int_distribution<int>(0, 3)(rng)]);
    m_slide = std::uniform_real_distribution<float>(-1000, 1000)(rng);
    
    m_attack->setValue(std::uniform_int_distribution<int>(1, 50)(rng));
    m_sustain->setValue(std::uniform_int_distribution<int>(20, 100)(rng));
    m_release->setValue(std::uniform_int_distribution<int>(10, 100)(rng));
    m_vibRate->setValue(std::uniform_int_distribution<int>(0, 20)(rng));
    m_vibDepth->setValue(std::uniform_int_distribution<int>(0, 50)(rng));
    
    m_seamlessSlider->setValue(0);
    m_loopBtn->setChecked(false);
    m_arpRate->setValue(std::uniform_int_distribution<int>(0, 15)(rng));
    m_arpGate->setValue(std::uniform_int_distribution<int>(20, 100)(rng));

    m_filterTypeCombo->setCurrentIndex(std::uniform_int_distribution<int>(0, 2)(rng));
    m_cutoff->setValue(std::uniform_int_distribution<int>(200, 20000)(rng));
    m_filterRes->setValue(std::uniform_int_distribution<int>(0, 80)(rng));
    
    m_distTypeCombo->setCurrentIndex(std::uniform_int_distribution<int>(0, 3)(rng));
    m_distDrive->setValue(std::uniform_int_distribution<int>(0, 100)(rng));
    
    m_bitDepth->setValue(std::uniform_int_distribution<int>(2, 16)(rng));
    m_srDiv->setValue(std::uniform_int_distribution<int>(1, 10)(rng));
    
    m_delayTime->setValue(std::uniform_int_distribution<int>(0, 300)(rng));
    m_delayFb->setValue(std::uniform_int_distribution<int>(0, 80)(rng));
    m_pingPong->setChecked(std::uniform_int_distribution<int>(0, 1)(rng));
    m_reverbMix->setValue(std::uniform_int_distribution<int>(0, 100)(rng));
    m_reverbSize->setValue(std::uniform_int_distribution<int>(0, 100)(rng));

    m_chorusMix->setValue(std::uniform_int_distribution<int>(0, 80)(rng));
    m_chorusDepth->setValue(std::uniform_int_distribution<int>(0, 20)(rng));
    m_phaserMix->setValue(std::uniform_int_distribution<int>(0, 80)(rng));
    m_tremoloRate->setValue(std::uniform_int_distribution<int>(0, 15)(rng));
    m_ringModFreq->setValue(std::uniform_int_distribution<int>(0, 10)(rng) * 100);

    m_volEnvEditor->setPoints({0.0f, 1.0f, 0.5f, 0.2f, 0.0f});
    m_pitchEnvEditor->setPoints({0.5f, 0.5f, 0.5f});
    m_waveform->clearDrawing();

    for (auto* s : sliders) s->blockSignals(false);
    m_waveCombo->blockSignals(false);
    m_filterTypeCombo->blockSignals(false);
    m_distTypeCombo->blockSignals(false);
    m_pingPong->blockSignals(false);

    m_hasPreset = false;
    m_resetBtn->setEnabled(false);
    rebuild();
}

void MainWindow::play() {
    if (m_signal.empty()) return;
    QAudioFormat fmt;
    fmt.setSampleRate(SR);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    auto dev = QMediaDevices::defaultAudioOutput();
    if (m_audioSink) { m_audioSink->stop(); delete m_audioSink; }
    m_audioSink = new QAudioSink(dev, fmt, this);

    std::vector<int16_t> pcm(m_signal.size());
    for (size_t i = 0; i < m_signal.size(); ++i)
        pcm[i] = (int16_t)std::clamp(m_signal[i] * 32767.0f, -32768.0f, 32767.0f);

    m_audioBuf = new QBuffer(this);
    m_audioBuf->setData((const char*)pcm.data(), (int)(pcm.size() * 2));
    m_audioBuf->open(QIODevice::ReadOnly);
    
    m_audioSink->start(m_audioBuf);
    m_status->setText("♪ Играет...");

    connect(m_audioSink, &QAudioSink::stateChanged, this, [this](QAudio::State s){
        if (s == QAudio::IdleState) {
            if (m_loopBtn && m_loopBtn->isChecked()) {
                m_audioBuf->seek(0);
                m_audioSink->start(m_audioBuf);
            } else {
                m_status->setText("Готово");
            }
        }
    });
}

void MainWindow::exportFile(const QString& ext) {
    if (m_signal.empty()) return;
    QString path = QFileDialog::getSaveFileName(this, "Сохранить", "sfx." + ext,
        QString("%1 (*.%1)").arg(ext.toUpper()));
    if (path.isEmpty()) return;

    bool ok = false;
    if (ext == "wav") ok = writeWav(path, m_signal);
    else if (ext == "mp3") ok = writeMp3(path, m_signal);
    else if (ext == "ogg") ok = writeOgg(path, m_signal);

    if (ok) m_status->setText("✅ Сохранено: " + QFileInfo(path).fileName());
    else    m_status->setText("❌ Ошибка сохранения (проверьте зависимости)");
}