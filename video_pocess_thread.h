#ifndef VIDEO_POCESS_THREAD_H
#define VIDEO_POCESS_THREAD_H

#include <QString>
#include <QThread>
#include <QMutex>

#include "opencv2/opencv.hpp"

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

    enum selectingTrackerType{
        CSRT,
        KCF
    };

protected:
    void run() override;

signals:
    void frameChanged(cv::Mat *data);

private:
    bool running;

    QMutex *dataLock;
    cv::Mat frame;

    int cameraID;
    QString videoPath;

    bool selectingVision;
    cv::Rect selectingBound;
    selectingTrackerType currentSelectingTrackerType;
    bool selectingNeedInit;
};

#endif // VIDEO_POCESS_THREAD_H
