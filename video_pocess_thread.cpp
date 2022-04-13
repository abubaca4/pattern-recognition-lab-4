#include "video_pocess_thread.h"

VideoProcessThread::VideoProcessThread(int camera, QMutex *lock):
    running(false), dataLock(lock), cameraID(camera), videoPath(""), selectingVision(false)
{
    currentSelectingTrackerType = CSRT;
    selectingNeedInit = false;
}

VideoProcessThread::VideoProcessThread(QString videoPath, QMutex *lock):
    running(false), dataLock(lock), cameraID(-1), videoPath(videoPath), selectingVision(false)
{
    currentSelectingTrackerType = CSRT;
    selectingNeedInit = false;
}

VideoProcessThread::~VideoProcessThread()
{}

void VideoProcessThread::run() {
    running = true;
    cv::VideoCapture cap;
    if (cameraID != -1) {
        cap = cv::VideoCapture(cameraID);
    } else {
        cap = cv::VideoCapture(videoPath.toStdString(), cv::CAP_ANY, {cv::CAP_PROP_HW_ACCELERATION, cv::VIDEO_ACCELERATION_ANY, cv::CAP_PROP_HW_DEVICE, -1});
    }

    cv::Mat tmp_frame;
    while(running) {
        cap >> tmp_frame;
        if (tmp_frame.empty()) {
            break;
        }

        if (selectingVision){
            if (selectingNeedInit){
            }
            cv::rectangle(tmp_frame, selectingBound, cv::Scalar(0, 0, 255), 1);
        }

        cvtColor(tmp_frame, tmp_frame, cv::COLOR_BGR2RGB);

        dataLock->lock();
        tmp_frame.copyTo(frame);
        dataLock->unlock();

        emit frameChanged(&frame);
    }
}

void VideoProcessThread::setSelecting(int x1, int y1, int x2, int y2){
    selectingBound = cv::Rect(x1, y1, x2 - x1, y2 - y1);
}

void VideoProcessThread::setSelectionVisiable(bool state){
    selectingVision = state;
}
