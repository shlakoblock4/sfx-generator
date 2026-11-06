#pragma once
#include <QWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <functional>
#include <vector>

// ==========================================
// 1. X/Y-ПАД
// ==========================================
class XYPad : public QWidget {
    Q_OBJECT
public:
    explicit XYPad(QWidget* parent = nullptr);
    void setValues(float x, float y);
    float xValue() const { return m_x; }
    float yValue() const { return m_y; }
signals:
    void valuesChanged(float x, float y);
protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
private:
    float m_x = 0.5f;
    float m_y = 0.5f;
    void updateFromPos(QPoint pos);
};

// ==========================================
// 2. СТЕП-СЕКВЕНСОР
// ==========================================
class StepSequencer : public QWidget {
    Q_OBJECT
public:
    explicit StepSequencer(QWidget* parent = nullptr);
    bool isStepOn(int i) const { return i >= 0 && i < 16 ? m_steps[i] : false; }
signals:
    void stepToggled();
private:
    bool m_steps[16] = {false};
};

// ==========================================
// 3. КРУТИЛКА
// ==========================================
class Knob : public QWidget {
    Q_OBJECT
public:
    explicit Knob(const QString& label, QWidget* parent = nullptr);
    void setRange(int lo, int hi);
    void setValue(int v);
    int value() const { return m_value; }
    void setFormat(std::function<QString(int)> fmt);
signals:
    void valueChanged(int v);
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
private:
    int m_value = 0;
    int m_lo = 0, m_hi = 100;
    int m_lastY = 0;
    QString m_label;
    std::function<QString(int)> m_fmt = [](int v){ return QString::number(v); };
};

// ==========================================
// 4. ГРАФИЧЕСКИЙ РЕДАКТОР ОГИБАЮЩЕЙ
// ==========================================
class EnvelopeEditor : public QWidget {
    Q_OBJECT
public:
    explicit EnvelopeEditor(const QString& label, QWidget* parent = nullptr);
    void setPoints(const std::vector<float>& points);
    std::vector<float> points() const { return m_points; }
signals:
    void pointsChanged();
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
private:
    QString m_label;
    std::vector<float> m_points;
    void updateFromPos(QPoint pos);
};