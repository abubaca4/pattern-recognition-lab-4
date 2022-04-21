#ifndef VIDEOPOCESSTHREAD_H
#define VIDEOPOCESSTHREAD_H

#include <QString>
#include <QThread>
#include <QMutex>
#include <QQueue>

#include "opencv2/opencv.hpp"
#include "opencv2/tracking.hpp"
#include "opencv2/tracking/tracking_legacy.hpp"

#include <chrono>
#include <thread>
#include <vector>
#include <array>

class VideoProcessThread : public QThread
{
    Q_OBJECT

public:
    VideoProcessThread(int camera, QMutex *lock);
    VideoProcessThread(QString videoPath, QMutex *lock);
    ~VideoProcessThread();

    void setRunning(bool run) {running = run; };
    void setSelecting(int x1, int y1, int x2, int y2);
    void setSelectionVisiable(bool state);
    void startSelectionTracker();

    enum selectingTrackerType{
        CSRT,
        KCF,
        MIL,
        MOSSE,
        Boosting,
        MedianFlow,
        TLD
    };

    void changeSelectionTracker(selectingTrackerType tracker);

    enum detectionType{
        No,
        Motion,
        Contrast
    };

    void changeDetectionType(detectionType deT);
    QMutex detBorderLock;
    void setFrameControlStatus(bool status);
    void setContrastBorders(uint low, uint high);

protected:
    void run() override;

public slots:
    void mouseCordChange(int x, int y);

signals:
    void frameChanged(cv::Mat *data);
    void trackingStatusUpdate(bool tracking);
    void detectionChanged(std::vector<std::array<int, 4>> *data);
    void statsChanged(qreal fps, qreal mean, qreal std, qreal min, qreal max, int xMouse, int yMouse, int britness);

private:
    bool running;

    QMutex *dataLock;
    cv::Mat frame;

    int cameraID;
    QString videoPath;

    bool selectingVision, selectionTrackingStart;
    cv::Rect selectingBound;
    selectingTrackerType currentSelectingTrackerType;
    bool selectingNeedInit;
    cv::Ptr<cv::Tracker> selectingTracker;

    detectionType detection;
    std::vector<std::array<int, 4>> detectionBorder;
    void detectMotion(const cv::Mat &in);
    cv::Ptr<cv::BackgroundSubtractorMOG2> motionSegmentor;
    cv::Mat motionKernel;
    void detectContrast(const cv::Mat &in);

    void calculateStats(const cv::Mat &in);
    qreal fps;
    std::chrono::steady_clock::time_point statsBegin;
    uint statsFrameCount;

    bool isFrameControlEnabled;

    QQueue<cv::Point> trajectory;

    int mouseX, mouseY;

    uint britnessLow, britnessHigh;
};

#endif // VIDEOPOCESSTHREAD_H
