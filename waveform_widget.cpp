#include "waveform_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

WaveformWidget::WaveformWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(120);
    setStyleSheet("background:#0a0f19;border:1px solid #2a3555;");
    m_drawnEnv.assign(m_resolution, 1.0f);
}

void WaveformWidget::setSignal(const std::vector<float>* sig) {
    m_signal = sig;
    update();
}

void WaveformWidget::setDrawMode(bool enabled) {
    m_drawMode = enabled;
    setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
}

void WaveformWidget::clearDrawing() {
    m_drawnEnv.assign(m_resolution, 1.0f);
    emit envelopeChanged();
    update();
}

void WaveformWidget::updateEnvFromPos(QPoint pos) {
    if (!m_drawMode || m_drawnEnv.empty()) return;
    int W = width(), H = height();
    int idx = (int)((float)pos.x() / W * m_resolution);
    idx = std::clamp(idx, 0, m_resolution - 1);
    float val = 1.0f - std::clamp((float)pos.y() / H, 0.0f, 1.0f);
    m_drawnEnv[idx] = val;
    emit envelopeChanged();
    update();
}

void WaveformWidget::mousePressEvent(QMouseEvent* e) {
    if (m_drawMode && e->button() == Qt::LeftButton) {
        m_drawing = true;
        updateEnvFromPos(e->pos());
    }
}

void WaveformWidget::mouseMoveEvent(QMouseEvent* e) {
    if (m_drawing) {
        updateEnvFromPos(e->pos());
    }
}

void WaveformWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_drawing = false;
    }
}

void WaveformWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const int W = width(), H = height();

    p.setPen(QColor(22, 30, 46));
    for (int i = 1; i < 4; ++i) p.drawLine(0, H*i/4, W, H*i/4);
    p.setPen(QColor(42, 53, 85));
    p.drawLine(0, H/2, W, H/2);

    if (m_signal && !m_signal->empty()) {
        const auto& s = *m_signal;
        const int N = (int)s.size();
        const int step = std::max(1, N / W);
        QPainterPath path;
        path.moveTo(0, H/2);
        for (int x = 0; x < W; ++x) {
            int start = x * N / W;
            int end   = std::min(start + step, N);
            float peak = 0;
            for (int j = start; j < end; ++j)
                if (std::fabs(s[j]) > std::fabs(peak)) peak = s[j];
            float y = H/2.0f - peak * (H/2.0f) * 0.9f;
            path.lineTo(x, y);
        }
        p.setPen(QPen(QColor(110, 168, 255), 2));
        p.drawPath(path);
    }

    if (m_drawMode && !m_drawnEnv.empty()) {
        QPainterPath envPath;
        for (int i = 0; i < m_resolution; ++i) {
            float x = (float)i / (m_resolution - 1) * W;
            float y = (1.0f - m_drawnEnv[i]) * H;
            if (i == 0) envPath.moveTo(x, y);
            else envPath.lineTo(x, y);
        }
        p.setPen(QPen(QColor(255, 122, 217), 3)); 
        p.drawPath(envPath);
        
        p.setBrush(QColor(255, 255, 255));
        for (int i = 0; i < m_resolution; ++i) {
            float x = (float)i / (m_resolution - 1) * W;
            float y = (1.0f - m_drawnEnv[i]) * H;
            p.drawEllipse(QPoint(x, y), 3, 3);
        }
    }
}