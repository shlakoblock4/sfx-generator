#pragma once
#include <QWidget>
#include <vector>
#include <QMouseEvent>

class WaveformWidget : public QWidget {
    Q_OBJECT
public:
    explicit WaveformWidget(QWidget* parent = nullptr);
    void setSignal(const std::vector<float>* sig);
    void setDrawMode(bool enabled);
    std::vector<float> getDrawnEnvelope() const { return m_drawnEnv; }
    void clearDrawing();

signals:
    void envelopeChanged();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    const std::vector<float>* m_signal = nullptr;
    bool m_drawMode = false;
    bool m_drawing = false;
    std::vector<float> m_drawnEnv; 
    int m_resolution = 64; 
    void updateEnvFromPos(QPoint pos);
};