#include "custom_widgets.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPainterPath>
#include <cmath>
#include <algorithm>

// ==========================================
// XY-PAD
// ==========================================
XYPad::XYPad(QWidget* parent) : QWidget(parent) {
    setMinimumSize(200, 150);
    setStyleSheet("background:#0a0f19; border:1px solid #2a3555; border-radius:8px;");
}

void XYPad::setValues(float x, float y) {
    m_x = std::clamp(x, 0.0f, 1.0f);
    m_y = std::clamp(y, 0.0f, 1.0f);
    update();
}

void XYPad::updateFromPos(QPoint pos) {
    m_x = std::clamp((float)pos.x() / width(), 0.0f, 1.0f);
    m_y = 1.0f - std::clamp((float)pos.y() / height(), 0.0f, 1.0f);
    emit valuesChanged(m_x, m_y);
    update();
}

void XYPad::mousePressEvent(QMouseEvent* e) { updateFromPos(e->pos()); }
void XYPad::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) updateFromPos(e->pos());
}

void XYPad::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int W = width(), H = height();

    p.setPen(QColor(30, 42, 68));
    for (int i = 1; i < 4; ++i) {
        p.drawLine(W*i/4, 0, W*i/4, H);
        p.drawLine(0, H*i/4, W, H*i/4);
    }

    int cx = m_x * W;
    int cy = (1.0f - m_y) * H;
    p.setBrush(QColor(110, 168, 255));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPoint(cx, cy), 10, 10);

    p.setPen(QColor(200, 220, 255));
    p.drawText(10, H - 10, QString("X: %1%  Y: %2%").arg(int(m_x*100)).arg(int(m_y*100)));
}

// ==========================================
// STEP SEQUENCER
// ==========================================
StepSequencer::StepSequencer(QWidget* parent) : QWidget(parent) {
    auto* layout = new QGridLayout(this);
    layout->setSpacing(4);
    layout->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < 16; ++i) {
        auto* b = new QPushButton();
        b->setCheckable(true);
        b->setFixedHeight(35);
        b->setStyleSheet("QPushButton { background:#1e2a44; border:none; border-radius:4px; } QPushButton:checked { background:#6ea8ff; }");
        connect(b, &QPushButton::toggled, this, [this, i, b](bool checked) {
            m_steps[i] = checked;
            emit stepToggled();
        });
        layout->addWidget(b, i / 8, i % 8);
    }
}

// ==========================================
// KNOB (Крутилка)
// ==========================================
Knob::Knob(const QString& label, QWidget* parent) : QWidget(parent), m_label(label) {
    setMinimumSize(60, 80);
    setStyleSheet("background:transparent;");
}

void Knob::setRange(int lo, int hi) { m_lo = lo; m_hi = hi; update(); }
void Knob::setValue(int v) { m_value = std::clamp(v, m_lo, m_hi); update(); }
void Knob::setFormat(std::function<QString(int)> fmt) { m_fmt = fmt; update(); }

void Knob::mousePressEvent(QMouseEvent* e) { m_lastY = e->pos().y(); }
void Knob::mouseMoveEvent(QMouseEvent* e) {
    if (!(e->buttons() & Qt::LeftButton)) return;
    int deltaY = m_lastY - e->pos().y();
    m_lastY = e->pos().y();
    
    float range = m_hi - m_lo;
    m_value = std::clamp(m_value + (int)(deltaY * range / 100.0f), m_lo, m_hi);
    emit valueChanged(m_value);
    update();
}

void Knob::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int W = width(), H = height();
    int size = std::min(W, H - 20);
    int cx = W / 2, cy = size / 2 + 5;
    int r = size / 2 - 5;

    p.setBrush(QColor(10, 15, 25));
    p.setPen(QPen(QColor(42, 53, 85), 2));
    p.drawEllipse(QPoint(cx, cy), r, r);

    float frac = (float)(m_value - m_lo) / std::max(1, m_hi - m_lo);
    float angle = -135.0f + frac * 270.0f;
    float rad = angle * M_PI / 180.0f;
    
    p.setPen(QPen(QColor(110, 168, 255), 3));
    p.drawLine(cx, cy, cx + std::cos(rad) * r * 0.8f, cy + std::sin(rad) * r * 0.8f);

    p.setPen(QColor(200, 220, 255));
    p.drawText(0, H - 15, W, 15, Qt::AlignCenter, m_label + ": " + m_fmt(m_value));
}

// ==========================================
// ENVELOPE EDITOR
// ==========================================
EnvelopeEditor::EnvelopeEditor(const QString& label, QWidget* parent) : QWidget(parent), m_label(label) {
    setMinimumHeight(100);
    setStyleSheet("background:#0a0f19; border:1px solid #2a3555; border-radius:8px;");
    m_points = {0.0f, 1.0f, 0.5f, 0.2f, 0.0f};
}

void EnvelopeEditor::setPoints(const std::vector<float>& points) {
    m_points = points;
    update();
}

void EnvelopeEditor::updateFromPos(QPoint pos) {
    int W = width(), H = height();
    float x = std::clamp((float)pos.x() / W, 0.0f, 1.0f);
    float y = 1.0f - std::clamp((float)pos.y() / H, 0.0f, 1.0f);
    
    int idx = (int)(x * (m_points.size() - 1));
    if (idx >= 0 && idx < (int)m_points.size()) {
        m_points[idx] = y;
        emit pointsChanged();
        update();
    }
}

void EnvelopeEditor::mousePressEvent(QMouseEvent* e) { updateFromPos(e->pos()); }
void EnvelopeEditor::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) updateFromPos(e->pos());
}

void EnvelopeEditor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int W = width(), H = height();

    p.setPen(QColor(30, 42, 68));
    for (int i = 1; i < 4; ++i) {
        p.drawLine(W*i/4, 0, W*i/4, H);
        p.drawLine(0, H*i/4, W, H*i/4);
    }

    if (m_points.size() < 2) return;
    QPainterPath path;
    for (size_t i = 0; i < m_points.size(); ++i) {
        float x = (float)i / (m_points.size() - 1) * W;
        float y = (1.0f - m_points[i]) * H;
        if (i == 0) path.moveTo(x, y);
        else path.lineTo(x, y);
    }
    p.setPen(QPen(QColor(110, 168, 255), 2));
    p.drawPath(path);

    p.setBrush(QColor(255, 255, 255));
    for (size_t i = 0; i < m_points.size(); ++i) {
        float x = (float)i / (m_points.size() - 1) * W;
        float y = (1.0f - m_points[i]) * H;
        p.drawEllipse(QPoint(x, y), 4, 4);
    }

    p.setPen(QColor(200, 220, 255));
    p.drawText(5, 15, m_label);
}