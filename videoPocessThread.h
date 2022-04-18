#ifndef VIDEOPOCESSTHREAD_H
#define VIDEOPOCESSTHREAD_H

#include <QString>
#include <QThread>
#include <QMutex>

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
        Motion
    };

    void changeDetectionType(detectionType deT);
    QMutex detBorderLock;

protected:
    void run() override;

signals:
    void frameChanged(cv::Mat *data);
    void trackingStatusUpdate(bool tracking);
    void detectionChanged(std::vector<std::array<int, 4>> *data);
    void statsChanged(qreal fps);

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

    void calculateStats(const cv::Mat &in);

    const bool isFrameControlEnabled;
};

#endif // VIDEOPOCESSTHREAD_H
