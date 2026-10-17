#include "visualizer_widget.h"
#include <QPainter>
#include <QColor>
#include <algorithm>
#include <cmath>


#define M_PI 3.14159265358979323846


VisualizerWidget::VisualizerWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_magnitudes.resize(128, 0.0f);

    m_fftConfig = kiss_fftr_alloc(m_fftSize, 0, nullptr, nullptr);
    
    m_fftIn.resize(m_fftSize, 0.0f);
    m_fftOut.resize(m_fftSize / 2 + 1);

    m_windowFunc.resize(m_fftSize);
    for (int i = 0; i < m_fftSize; ++i) {
        m_windowFunc[i] = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * i / (m_fftSize - 1)));
    }
}

VisualizerWidget::~VisualizerWidget()
{
    if (m_fftConfig) {
        free(m_fftConfig);
    }
}

void VisualizerWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void VisualizerWidget::updateAudioData(const std::vector<float>& rawSamples)
{
    if (rawSamples.empty()) return;

    if (rawSamples.size() >= static_cast<size_t>(m_fftSize)) {
        m_magnitudes = doFFT(rawSamples.data());
    } else {
        std::vector<float> padded(m_fftSize, 0.0f);
        std::copy(rawSamples.begin(), rawSamples.end(), padded.begin());
        m_magnitudes = doFFT(padded.data());
    }
    
    update(); 
}

std::vector<float> VisualizerWidget::doFFT(const float* rawAudioSamples) {
    for (int i = 0; i < m_fftSize; ++i) {
        m_fftIn[i] = rawAudioSamples[i] * m_windowFunc[i];
    }

    kiss_fftr(m_fftConfig, m_fftIn.data(), m_fftOut.data());

    int numBins = m_fftSize / 2;
    std::vector<float> magnitudes(numBins);
    for (int i = 0; i < numBins; ++i) {
        float real = m_fftOut[i].r;
        float imag = m_fftOut[i].i;
        magnitudes[i] = std::sqrt(real * real + imag * imag) * 0.05f; 
    }

    return magnitudes;
}

void VisualizerWidget::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT);
    m_magnitudes.clear();
}

void VisualizerWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::transparent); 
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_magnitudes.empty()) return;

    int numBars = m_magnitudes.size();
    int barsToDraw = std::min(numBars, 32); 
    
    float barWidth = static_cast<float>(width()) / barsToDraw;
    float maxHeight = height();

    QLinearGradient gradient(0, height(), 0, 0);

    gradient.setColorAt(0.0, QColor("#00ff26")); 
    gradient.setColorAt(0.3, QColor("#f6ff00")); 
    gradient.setColorAt(0.8, QColor("#FF0055")); 

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);

    for (int i = 0; i < barsToDraw; ++i) {
        float magnitude = m_magnitudes[i]; 
        
        if (magnitude > 1.0f) magnitude = 1.0f;
        if (magnitude < 0.0f) magnitude = 0.0f;

        float barHeight = magnitude * maxHeight;
        float x = i * barWidth;
        float y = height() - barHeight;

        painter.drawRect(QRectF(x, y, barWidth - 1.0f, barHeight)); 
    }
}