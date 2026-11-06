#include "main_window.h"
#include "audio_engine.h"
#include "audio_export.h"
#include "waveform_widget.h"
#include "custom_widgets.h"
#include <QAudioSink>

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QTabWidget>

static std::vector<Preset> PRESETS = {
    {"Пиу",     {0.30f, 1200, 0,   0.0f, 0.15f, 0.80f, -800, "sine"}},
    {"Взрыв",   {0.80f, 80,   0,   0.9f, 0.60f, 0.90f, 0,    "sawtooth"}},
    {"Лазер",   {0.35f, 900,  200, 0.0f, 0.20f, 0.75f, -700, "square"}},
    {"Прыжок",  {0.25f, 400,  0,   0.0f, 0.10f, 0.70f, 600,  "square"}},
    {"Монета",  {0.40f, 988,  0,   0.0f, 0.15f, 0.75f, 494,  "sine"}},
    {"Удар",    {0.15f, 150,  0,   0.4f, 0.08f, 0.85f, 0,    "triangle"}},
    {"Свист",   {0.70f, 2000, 100, 0.0f, 0.40f, 0.70f, -600, "sine"}},
    {"Бум",     {1.20f, 60,   0,   0.7f, 0.90f, 0.95f, 0,    "sine"}},
    {"Пуля",    {0.12f, 800,  300, 0.6f, 0.05f, 0.85f, 0,    "square"}},
    {"Бонус",   {0.50f, 660,  0,   0.0f, 0.20f, 0.78f, 880,  "triangle"}},
    {"Урон",    {0.30f, 220,  150, 0.3f, 0.20f, 0.80f, -300, "sawtooth"}},
    {"Клик",    {0.05f, 1800, 0,   0.2f, 0.02f, 0.65f, 0,    "square"}},
};

MainWindow::MainWindow() {
    setWindowTitle("SFX Generator — C++ / Qt6");
    resize(960, 1080);
    setStyleSheet(R"(
        QMainWindow, QWidget { background:#1a1a2e; color:#cfe0ff; font-family:'Segoe UI'; }
        QLabel { color:#cfe0ff; }
        QPushButton { background:#1e2a44; color:#cfe0ff; border:none; padding:8px; border-radius:6px; }
        QPushButton:hover { background:#2a3a5c; }
        QPushButton:checked, QPushButton[preset="true"] { background:#6ea8ff; color:white; }
        QComboBox { background:#0a0f19; color:#cfe0ff; border:1px solid #2a3555; padding:4px; border-radius:4px; }
        QSlider::groove:horizontal { height:6px; background:#2a3555; border-radius:3px; }
        QSlider::handle:horizontal { background:#6ea8ff; width:16px; margin:-5px 0; border-radius:8px; }
        QTabWidget::pane { border: 1px solid #2a3555; border-radius: 6px; margin-top: 4px; }
        QTabBar::tab { background:#1e2a44; color:#cfe0ff; padding:8px 16px; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }
        QTabBar::tab:selected { background:#6ea8ff; color:white; }
        QCheckBox { color:#cfe0ff; font-weight:bold; padding:8px; }
    )");

    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(18, 18, 18, 18);
    setCentralWidget(central);

    auto* title = new QLabel("🎵 SFX Generator — C++ / Qt6");
    title->setStyleSheet("font-size:20px;font-weight:bold;color:#6ea8ff;");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* topRow = new QHBoxLayout;
    m_resetBtn = new QPushButton("⟲ Сброс к пресету");
    m_resetBtn->setEnabled(false);
    connect(m_resetBtn, &QPushButton::clicked, this, &MainWindow::resetToPreset);
    topRow->addWidget(m_resetBtn);
    topRow->addStretch();
    root->addLayout(topRow);

    auto* presetRow = new QGridLayout;
    int row = 0, col = 0;
    for (auto& pr : PRESETS) {
        auto* b = new QPushButton(pr.name);
        b->setProperty("preset", true);
        b->setCheckable(true);
        connect(b, &QPushButton::clicked, this, [this, &pr, b]() {
            for (auto* other : findChildren<QPushButton*>())
                if (other->property("preset").toBool()) other->setChecked(false);
            b->setChecked(true);
            m_currentPreset = pr;
            m_hasPreset = true;
            m_resetBtn->setEnabled(true);
            applyPreset(pr);
        });
        presetRow->addWidget(b, row, col);
        if (++col == 6) { col = 0; ++row; }
    }
    root->addLayout(presetRow);

    // ===== X/Y-пад и Крутилки =====
    auto* topControls = new QHBoxLayout;
    auto* padBox = new QVBoxLayout;
    padBox->addWidget(new QLabel("X/Y-пад (Частота / Громкость)"));
    m_xyPad = new XYPad;
    m_xyPad->setFixedSize(200, 150);
    connect(m_xyPad, &XYPad::valuesChanged, this, [this](float x, float y){
        m_frequency->setValue(20 + x * 2980);
        m_gain->setValue(y * 200);
        if (m_knobFreq) m_knobFreq->setValue(m_frequency->value());
        if (m_knobGain) m_knobGain->setValue(m_gain->value());
    });
    padBox->addWidget(m_xyPad);
    topControls->addLayout(padBox);

    auto* knobsBox = new QHBoxLayout;
    m_knobFreq = new Knob("Частота");
    m_knobFreq->setRange(20, 3000);
    m_knobFreq->setFormat([](int v){ return QString::number(v) + " Hz"; });
    m_knobFreq->setValue(440);
    connect(m_knobFreq, &Knob::valueChanged, this, [this](int v){
        m_frequency->setValue(v);
        if (m_xyPad) m_xyPad->setValues((v - 20) / 2980.0f, m_gain->value() / 200.0f);
    });
    knobsBox->addWidget(m_knobFreq);

    m_knobGain = new Knob("Громкость");
    m_knobGain->setRange(0, 200);
    m_knobGain->setFormat([](int v){ return QString::number(v) + "%"; });
    m_knobGain->setValue(80);
    connect(m_knobGain, &Knob::valueChanged, this, [this](int v){
        m_gain->setValue(v);
        if (m_xyPad) m_xyPad->setValues(m_frequency->value() / 3000.0f, v / 200.0f);
    });
    knobsBox->addWidget(m_knobGain);

    m_knobDecay = new Knob("Спад");
    m_knobDecay->setRange(1, 100);
    m_knobDecay->setFormat([](int v){ return QString::number(v/100.0, 'f', 2); });
    m_knobDecay->setValue(30);
    connect(m_knobDecay, &Knob::valueChanged, this, [this](int v){
        m_decay->setValue(v);
    });
    knobsBox->addWidget(m_knobDecay);

    topControls->addLayout(knobsBox);
    topControls->addStretch();
    root->addLayout(topControls);

    // ===== ВКЛАДКИ ЭФФЕКТОВ =====
    auto* tabs = new QTabWidget;

    auto* tabBase = new QWidget;
    auto* gridBase = new QGridLayout(tabBase);
    addSlider(gridBase, 0, 0, "Длительность", 5,  50,  5,   &m_duration,   [](int v){ return QString::number(v/100.0, 'f', 2) + " s"; });
    addSlider(gridBase, 0, 1, "Частота",      20, 3000, 1,  &m_frequency,  [](int v){ return QString::number(v) + " Hz"; });
    addSlider(gridBase, 0, 2, "Громкость",    0,  200,  1,  &m_gain,       [](int v){ return QString::number(v) + "%"; });
    addSlider(gridBase, 1, 0, "FM",           0,  500,  1,  &m_fm,         [](int v){ return QString::number(v) + " Hz"; });
    addSlider(gridBase, 1, 1, "Шум",          0,  100,  1,  &m_noise,      [](int v){ return QString::number(v) + "%"; });
    addSlider(gridBase, 1, 2, "Кроссфейд",    0,  200,  1,  &m_seamlessSlider, [](int v){ return (v == 0) ? "OFF" : QString::number(v) + " ms"; });
    tabs->addTab(tabBase, "Базовые");

    auto* tabEnv = new QWidget;
    auto* gridEnv = new QGridLayout(tabEnv);
    addSlider(gridEnv, 0, 0, "Спад (Decay)", 1,  100,  1,  &m_decay,      [](int v){ return QString::number(v/100.0, 'f', 2); });
    addSlider(gridEnv, 0, 1, "Атака",        1,  200,  1,  &m_attack,     [](int v){ return QString::number(v/1000.0, 'f', 3) + " s"; });
    addSlider(gridEnv, 0, 2, "Sustain",      0,  100,  1,  &m_sustain,    [](int v){ return QString::number(v) + "%"; });
    addSlider(gridEnv, 1, 0, "Release",      1,  200,  1,  &m_release,    [](int v){ return QString::number(v/1000.0, 'f', 3) + " s"; });
    m_volEnvEditor = new EnvelopeEditor("Volume Envelope");
    connect(m_volEnvEditor, &EnvelopeEditor::pointsChanged, this, [this]{ rebuild(); });
    gridEnv->addWidget(m_volEnvEditor, 2, 0, 1, 3);
    m_pitchEnvEditor = new EnvelopeEditor("Pitch Envelope");
    connect(m_pitchEnvEditor, &EnvelopeEditor::pointsChanged, this, [this]{ rebuild(); });
    gridEnv->addWidget(m_pitchEnvEditor, 3, 0, 1, 3);
    tabs->addTab(tabEnv, "Огибающие (ADSR)");

    auto* tabRhythm = new QWidget;
    auto* gridRhythm = new QGridLayout(tabRhythm);
    addSlider(gridRhythm, 0, 0, "Арпеджио (Гц)", 0, 20, 1, &m_arpRate, [](int v){ return (v == 0) ? "OFF" : QString::number(v) + " Hz"; });
    addSlider(gridRhythm, 0, 1, "Скважность", 1, 100, 1, &m_arpGate, [](int v){ return QString::number(v) + "%"; });
    addSlider(gridRhythm, 0, 2, "Вибрато",      0,  50,   1,  &m_vibRate,    [](int v){ return QString::number(v/10.0, 'f', 1) + " Hz"; });
    addSlider(gridRhythm, 1, 0, "Глубина виб.", 0,  200,  1,  &m_vibDepth,   [](int v){ return QString::number(v) + " Hz"; });
    auto* seqBox = new QVBoxLayout;
    auto* seqHead = new QHBoxLayout;
    seqHead->addWidget(new QLabel("Степ-секвенсор (16 шагов)"));
    m_stepModeBtn = new QCheckBox("Включить ритм");
    connect(m_stepModeBtn, &QCheckBox::toggled, this, [this](bool){ rebuild(); });
    seqHead->addWidget(m_stepModeBtn);
    seqHead->addStretch();
    seqBox->addLayout(seqHead);
    m_sequencer = new StepSequencer;
    connect(m_sequencer, &StepSequencer::stepToggled, this, [this]{ rebuild(); });
    seqBox->addWidget(m_sequencer);
    gridRhythm->addLayout(seqBox, 2, 0, 1, 3);
    tabs->addTab(tabRhythm, "Ритм / Арпеджио");

    auto* tabDist = new QWidget;
    auto* gridDist = new QGridLayout(tabDist);
    m_filterTypeCombo = new QComboBox;
    m_filterTypeCombo->addItems({"LPF (низкие)", "HPF (высокие)", "BPF (полоса)"});
    connect(m_filterTypeCombo, &QComboBox::currentIndexChanged, this, [this](int){ rebuild(); });
    gridDist->addWidget(new QLabel("Тип фильтра:"), 0, 0);
    gridDist->addWidget(m_filterTypeCombo, 0, 1);
    addSlider(gridDist, 1, 0, "Срез фильтра", 20, 20000, 100, &m_cutoff, [](int v){ return (v >= 20000) ? "OFF" : QString::number(v) + " Hz"; });
    addSlider(gridDist, 1, 1, "Резонанс", 0, 100, 1, &m_filterRes, [](int v){ return QString::number(v) + "%"; });
    m_distTypeCombo = new QComboBox;
    m_distTypeCombo->addItems({"Off", "Overdrive", "Hard Clip", "Wavefolder"});
    connect(m_distTypeCombo, &QComboBox::currentIndexChanged, this, [this](int){ rebuild(); });
    gridDist->addWidget(new QLabel("Тип дисторшна:"), 2, 0);
    gridDist->addWidget(m_distTypeCombo, 2, 1);
    addSlider(gridDist, 3, 0, "Драйв", 0, 100, 1, &m_distDrive, [](int v){ return QString::number(v) + "%"; });
    tabs->addTab(tabDist, "Фильтр / Дисторшн");

    auto* tabLofi = new QWidget;
    auto* gridLofi = new QGridLayout(tabLofi);
    addSlider(gridLofi, 0, 0, "Битность", 2, 16, 1, &m_bitDepth, [](int v){ return QString::number(v) + " bit"; });
    addSlider(gridLofi, 0, 1, "Снижение SR", 1, 50, 1, &m_srDiv, [](int v){ return "x" + QString::number(v); });
    tabs->addTab(tabLofi, "Lo-Fi (Bitcrusher)");

    auto* tabSpace = new QWidget;
    auto* gridSpace = new QGridLayout(tabSpace);
    addSlider(gridSpace, 0, 0, "Время эха", 0, 500, 1, &m_delayTime, [](int v){ return QString::number(v) + " ms"; });
    addSlider(gridSpace, 0, 1, "Feedback", 0, 95, 1, &m_delayFb, [](int v){ return QString::number(v) + "%"; });
    m_pingPong = new QCheckBox("Пинг-понг эхо");
    connect(m_pingPong, &QCheckBox::toggled, this, [this](bool){ rebuild(); });
    gridSpace->addWidget(m_pingPong, 1, 0);
    addSlider(gridSpace, 2, 0, "Реверб микс", 0, 100, 1, &m_reverbMix, [](int v){ return QString::number(v) + "%"; });
    addSlider(gridSpace, 2, 1, "Размер реверба", 0, 100, 1, &m_reverbSize, [](int v){ return QString::number(v) + "%"; });
    tabs->addTab(tabSpace, "Эхо / Реверб");

    auto* tabMod = new QWidget;
    auto* gridMod = new QGridLayout(tabMod);
    addSlider(gridMod, 0, 0, "Хорус микс", 0, 100, 1, &m_chorusMix, [](int v){ return QString::number(v) + "%"; });
    addSlider(gridMod, 0, 1, "Глубина хоруса", 0, 50, 1, &m_chorusDepth, [](int v){ return QString::number(v) + " ms"; });
    addSlider(gridMod, 1, 0, "Фэйзер микс", 0, 100, 1, &m_phaserMix, [](int v){ return QString::number(v) + "%"; });
    addSlider(gridMod, 1, 1, "Тремоло (Гц)", 0, 20, 1, &m_tremoloRate, [](int v){ return (v == 0) ? "OFF" : QString::number(v) + " Hz"; });
    addSlider(gridMod, 2, 0, "Ринг-мод (Гц)", 0, 2000, 10, &m_ringModFreq, [](int v){ return (v == 0) ? "OFF" : QString::number(v) + " Hz"; });
    tabs->addTab(tabMod, "Модуляция (FX)");

    root->addWidget(tabs);

    // Форма волны и осциллограмма
    auto* wfRow = new QHBoxLayout;
    wfRow->addWidget(new QLabel("Форма волны:"));
    m_waveCombo = new QComboBox;
    m_waveCombo->addItems({"sine", "square", "sawtooth", "triangle"});
    connect(m_waveCombo, &QComboBox::currentTextChanged, this, [this](const QString&){ rebuild(); });
    wfRow->addWidget(m_waveCombo);
    wfRow->addStretch();

    m_drawModeBtn = new QCheckBox("✏ Режим рисования");
    connect(m_drawModeBtn, &QCheckBox::toggled, this, [this](bool checked){
        m_waveform->setDrawMode(checked);
    });
    wfRow->addWidget(m_drawModeBtn);

    m_clearDrawBtn = new QPushButton("Очистить");
    connect(m_clearDrawBtn, &QPushButton::clicked, this, [this](){
        m_waveform->clearDrawing();
    });
    wfRow->addWidget(m_clearDrawBtn);
    root->addLayout(wfRow);

    m_waveform = new WaveformWidget;
    connect(m_waveform, &WaveformWidget::envelopeChanged, this, [this]{ rebuild(); });
    root->addWidget(m_waveform);

    // Кнопки управления
    auto* btnRow = new QHBoxLayout;
    m_loopBtn = new QCheckBox("🔁 Зациклить");
    connect(m_loopBtn, &QCheckBox::toggled, this, [this](bool){ play(); }); 
    btnRow->addWidget(m_loopBtn);

    auto mkBtn = [&](const QString& text, const QString& color, auto fn) {
        auto* b = new QPushButton(text);
        b->setStyleSheet(QString("background:%1;color:white;font-weight:bold;padding:12px;border-radius:8px;").arg(color));
        connect(b, &QPushButton::clicked, this, fn);
        btnRow->addWidget(b, 1);
    };
    mkBtn("▶ Воспроизвести", "#4a8dff", [this]{ play(); });
    mkBtn("🎲 Случайный",    "#b57aff", [this]{ randomize(); });
    mkBtn("⬇ WAV",           "#2ecc71", [this]{ exportFile("wav"); });
    mkBtn("⬇ OGG",           "#f39c12", [this]{ exportFile("ogg"); });
    
#ifdef HAVE_LAME
    mkBtn("⬇ MP3",           "#e74c3c", [this]{ exportFile("mp3"); });
#else
    auto* mp3Btn = new QPushButton("⬇ MP3 (Нет LAME)");
    mp3Btn->setStyleSheet("background:#5a2a2a;color:#888;font-weight:bold;padding:12px;border-radius:8px;");
    mp3Btn->setEnabled(false);
    btnRow->addWidget(mp3Btn, 1);
#endif

    root->addLayout(btnRow);

    m_status = new QLabel("Готово");
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setStyleSheet("color:#8fa8c8;");
    root->addWidget(m_status);

    rebuild();
}

MainWindow::~MainWindow() {
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
    }
}

void MainWindow::addSlider(QGridLayout* g, int r, int c, const QString& label,
                   int lo, int hi, int step, QSlider** out,
                   std::function<QString(int)> fmt) {
    auto* card = new QWidget;
    card->setStyleSheet("background:#141928;border-radius:8px;");
    auto* l = new QVBoxLayout(card);
    l->setContentsMargins(10, 8, 10, 8);
    auto* head = new QHBoxLayout;
    head->addWidget(new QLabel(label));
    auto* val = new QLabel(fmt(lo));
    val->setStyleSheet("color:#6ea8ff;font-weight:bold;");
    head->addStretch();
    head->addWidget(val);
    l->addLayout(head);

    auto* s = new QSlider(Qt::Horizontal);
    s->setRange(lo, hi);
    s->setSingleStep(step);
    connect(s, &QSlider::valueChanged, this, [this, val, fmt](int v){
        val->setText(fmt(v));
        rebuild();
    });
    l->addWidget(s);
    g->addWidget(card, r, c);
    *out = s;
}