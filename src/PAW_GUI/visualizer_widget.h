#pragma once 

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <vector>
#include <kiss_fft.h>
#include <kiss_fftr.h>

class VisualizerWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit VisualizerWidget(QWidget *parent = nullptr);
    ~VisualizerWidget(); 

    void Clear();
    void updateAudioData(const std::vector<float>& rawSamples);

protected:
    void paintGL() override;
    void initializeGL() override;

private:
    int m_fftSize = 1024;
    kiss_fftr_cfg m_fftConfig;
    std::vector<kiss_fft_scalar> m_fftIn;
    std::vector<kiss_fft_cpx> m_fftOut;
    std::vector<float> m_windowFunc;
    std::vector<float> m_magnitudes;
    
    std::vector<float> doFFT(const float* rawAudioSamples);
};