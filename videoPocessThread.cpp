#include "videoPocessThread.h"

VideoProcessThread::VideoProcessThread(int camera, QMutex *lock):
    running(false), dataLock(lock), cameraID(camera), videoPath(""), selectingVision(false)
{
    currentSelectingTrackerType = CSRT;
    selectingNeedInit = selectionTrackingStart = false;
    detection = No;
}

VideoProcessThread::VideoProcessThread(QString videoPath, QMutex *lock):
    running(false), dataLock(lock), cameraID(-1), videoPath(videoPath), selectingVision(false)
{
    currentSelectingTrackerType = CSRT;
    selectingNeedInit = selectionTrackingStart = false;
    detection = No;
}

VideoProcessThread::~VideoProcessThread()
{}

void VideoProcessThread::run() {
    running = true;
    cv::VideoCapture cap;
    bool isNeedFrameRateControl = false;
    std::chrono::steady_clock::time_point begin, end;
    double fps = 0.0;
    if (cameraID != -1) {
        cap = cv::VideoCapture(cameraID);
    } else {
        cap = cv::VideoCapture(videoPath.toStdString(), cv::CAP_ANY, {cv::CAP_PROP_HW_ACCELERATION, cv::VIDEO_ACCELERATION_ANY, cv::CAP_PROP_HW_DEVICE, -1});
        fps = cap.get(cv::CAP_PROP_FPS);
        fps = 1000.0 / fps;
        isNeedFrameRateControl = true;
    }

    cv::Mat tmp_frame;
    while(running) {
        if (isNeedFrameRateControl){
            begin = std::chrono::steady_clock::now();
        }

        cap >> tmp_frame;
        if (tmp_frame.empty()) {
            break;
        }

        calculateStats(tmp_frame);

        if (selectingVision){
            if (selectionTrackingStart)
            {
                if (selectingNeedInit){
                    switch (currentSelectingTrackerType) {
                    case CSRT:
                        selectingTracker = cv::TrackerCSRT::create();
                        break;

                    case KCF:
                        selectingTracker = cv::TrackerKCF::create();
                        break;

                    case MIL:
                        selectingTracker = cv::TrackerMIL::create();
                        break;

                    case MOSSE:
                        selectingTracker = cv::legacy::upgradeTrackingAPI(cv::legacy::TrackerMOSSE::create());
                        break;

                    case Boosting:
                        selectingTracker = cv::legacy::upgradeTrackingAPI(cv::legacy::TrackerBoosting::create());
                        break;

                    case MedianFlow:
                        selectingTracker = cv::legacy::upgradeTrackingAPI(cv::legacy::TrackerMedianFlow::create());
                        break;

                    case TLD:
                        selectingTracker = cv::legacy::upgradeTrackingAPI(cv::legacy::TrackerTLD::create());
                        break;

                    default:
                        break;
                    }
                    selectingTracker->init(tmp_frame, selectingBound);
                    selectingNeedInit = false;
                } else {
                    static uint fail_count = 0;
                    if (selectingTracker->update(tmp_frame, selectingBound)){
                        fail_count = 0;
                    } else {
                        fail_count++;
                    }
                    if (fail_count >= 5){
                        selectingVision = false;
                        emit trackingStatusUpdate(false);
                    }
                }
            }
            cv::rectangle(tmp_frame, selectingBound, cv::Scalar(0, 0, 255), 1);
        } else {
            switch (detection) {
            case Motion:
                detectMotion(tmp_frame);
                break;
            default:
                break;
            }
            if (detection != No){
                emit detectionChanged(&detectionBorder);
                for (auto &i: detectionBorder){
                    cv::rectangle(tmp_frame, cv::Rect(cv::Point(i[0], i[1]), cv::Point(i[2], i[3])), cv::Scalar(0, 0, 255), 1);
                }
            }
        }

        cv::cvtColor(tmp_frame, tmp_frame, cv::COLOR_BGR2RGB);

        dataLock->lock();
        tmp_frame.copyTo(frame);
        dataLock->unlock();

        if (isNeedFrameRateControl){
            end = std::chrono::steady_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            if (diff < fps){
                std::this_thread::sleep_for(std::chrono::milliseconds((long long)fps - diff));
            }
        }

        emit frameChanged(&frame);
    }
}

void VideoProcessThread::setSelecting(int x1, int y1, int x2, int y2){
    selectingBound = cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2));
}

void VideoProcessThread::setSelectionVisiable(bool state){
    selectingVision = state;
    if (!state)
        selectionTrackingStart = false;
}

void VideoProcessThread::changeSelectionTracker(selectingTrackerType tracker){
    selectingNeedInit = true;
    currentSelectingTrackerType = tracker;
}

void VideoProcessThread::startSelectionTracker(){
    selectingNeedInit = true;
    selectionTrackingStart = true;
}

void VideoProcessThread::detectMotion(const cv::Mat &in){
    static cv::Ptr<cv::BackgroundSubtractorMOG2> segmentor = cv::createBackgroundSubtractorMOG2(500, 16, true);
    cv::Mat fgmask;
    segmentor->apply(in, fgmask);
    if (fgmask.empty()) {
        return;
    }

    cv::threshold(fgmask, fgmask, 25, 255, cv::THRESH_BINARY);

    const int noise_size = 9;
    static cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(noise_size, noise_size));;

    cv::erode(fgmask, fgmask, kernel);
    cv::dilate(fgmask, fgmask, kernel, cv::Point(-1, -1), 3);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(fgmask, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    detBorderLock.lock();
    detectionBorder = {};
    for (auto &i: contours){
        cv::Rect rect = cv::boundingRect(i);
        detectionBorder.push_back({rect.tl().x, rect.tl().y, rect.br().x, rect.br().y});
    }
    detBorderLock.unlock();
}

void VideoProcessThread::changeDetectionType(detectionType deT){
    detection = deT;
}

void VideoProcessThread::calculateStats(const cv::Mat &in){
    static qreal fps = 0;
    static auto begin = std::chrono::steady_clock::now();
    static int count = 0;
    count++;
    auto current = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(current - begin).count();
    if (diff > 1000){
        fps = (qreal)count / diff * 1000.0;
        begin = current;
        count = 0;
    }
    emit statsChanged(fps);
}
