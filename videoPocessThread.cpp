#include "videoPocessThread.h"

VideoProcessThread::VideoProcessThread(int camera, QMutex *lock):
    running(false), dataLock(lock), cameraID(camera), videoPath(""), selectingVision(false), isFrameControlEnabled(true)
{
    currentSelectingTrackerType = CSRT;
    selectingNeedInit = selectionTrackingStart = false;
    detection = No;
    motionSegmentor = cv::createBackgroundSubtractorMOG2(500, 16, true);
    const int noise_size = 9;
    motionKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(noise_size, noise_size));
    fps = 0.0;
    statsBegin = std::chrono::steady_clock::now();
    mouseX = mouseY = statsFrameCount = 0;
    britnessLow = 150;
    britnessHigh = 255;
}

VideoProcessThread::VideoProcessThread(QString videoPath, QMutex *lock):
    running(false), dataLock(lock), cameraID(-1), videoPath(videoPath), selectingVision(false), isFrameControlEnabled(true)
{
    currentSelectingTrackerType = CSRT;
    selectingNeedInit = selectionTrackingStart = false;
    detection = No;
    motionSegmentor = cv::createBackgroundSubtractorMOG2(500, 16, true);
    const int noise_size = 9;
    motionKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(noise_size, noise_size));
    fps = 0.0;
    statsBegin = std::chrono::steady_clock::now();
    mouseX = mouseY = statsFrameCount = 0;
    britnessLow = 150;
    britnessHigh = 255;
}

VideoProcessThread::~VideoProcessThread()
{}

void VideoProcessThread::run() {
    running = true;
    cv::VideoCapture cap;
    bool isNeedFrameRateControl = false;
    std::chrono::steady_clock::time_point begin, end;
    double fpsDelayTaget = 0.0;
    if (cameraID != -1) {
        cap = cv::VideoCapture(cameraID);
    } else {
        cap = cv::VideoCapture(videoPath.toStdString(), cv::CAP_ANY, {cv::CAP_PROP_HW_ACCELERATION, cv::VIDEO_ACCELERATION_ANY, cv::CAP_PROP_HW_DEVICE, -1});
        fpsDelayTaget = cap.get(cv::CAP_PROP_FPS);
        fpsDelayTaget = 1000.0 / fpsDelayTaget;
        isNeedFrameRateControl = true;
    }

    uint trackerFailСount = 0;

    cv::Mat tmp_frame;
    while(running) {
        if (isFrameControlEnabled && isNeedFrameRateControl){
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
                    trajectory = {};
                } else {
                    if (selectingTracker->update(tmp_frame, selectingBound)){
                        trackerFailСount = 0;
                    } else {
                        trackerFailСount++;
                    }
                    if (trackerFailСount >= 5){
                        selectionTrackingStart = selectingVision = false;
                        emit trackingStatusUpdate(false);
                    }
                }
                trajectory.enqueue(cv::Point((selectingBound.tl().x + selectingBound.br().x)/2, (selectingBound.tl().y + selectingBound.br().y)/2));
                if (trajectory.count() > 100){
                    trajectory.dequeue();
                }
                uint collorStart = 255 - (trajectory.count() - 1)*2;
                for (auto i = trajectory.begin(); i != trajectory.end(); i++){
                    auto next = i;
                    next++;
                    if (next == trajectory.end())
                        break;
                    cv::line(tmp_frame, *i, *next, cv::Scalar(0, collorStart, 0), 2);
                    collorStart+=2;
                }
            }
            cv::rectangle(tmp_frame, selectingBound, cv::Scalar(0, 0, 255), 1);
        } else {
            switch (detection) {
            case Motion:
                detectMotion(tmp_frame);
                break;

            case Contrast:
                detectContrast(tmp_frame);
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

        if (isFrameControlEnabled && isNeedFrameRateControl){
            end = std::chrono::steady_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            if (diff < fpsDelayTaget){
                this->thread()->msleep((long long)fpsDelayTaget - diff);
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
    cv::Mat fgmask;
    motionSegmentor->apply(in, fgmask);
    if (fgmask.empty()) {
        return;
    }

    cv::threshold(fgmask, fgmask, 25, 255, cv::THRESH_BINARY);

    cv::erode(fgmask, fgmask, motionKernel);
    cv::dilate(fgmask, fgmask, motionKernel, cv::Point(-1, -1), 3);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(fgmask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    detBorderLock.lock();
    detectionBorder = {};
    for (auto &i: contours){
        cv::Rect rect = cv::boundingRect(i);
        detectionBorder.push_back({rect.tl().x, rect.tl().y, rect.br().x, rect.br().y});
    }
    detBorderLock.unlock();
}

void VideoProcessThread::detectContrast(const cv::Mat &in){
    cv::Mat grayIn;
    cv::cvtColor(in, grayIn, cv::COLOR_BGR2GRAY);
    cv::Mat thresh;
    cv::threshold(grayIn, thresh, britnessLow, britnessHigh, cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

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
    statsFrameCount++;
    auto current = std::chrono::steady_clock::now();
    long long diff = std::chrono::duration_cast<std::chrono::milliseconds>(current - statsBegin).count();
    if (diff > 1000){
        fps = (qreal)statsFrameCount / diff * 1000.0;
        statsBegin = current;
        statsFrameCount = 0;
    }
    cv::Mat grayIn;
    cv::cvtColor(in, grayIn, cv::COLOR_BGR2GRAY);
    cv::Scalar mean, std;
    cv::meanStdDev(grayIn, mean, std);
    qreal min, max;
    cv::minMaxLoc(grayIn, &min, &max);
    emit statsChanged(fps, mean[0], std[0], min, max, mouseX, mouseY, grayIn.at<uint8_t>(cv::Point(mouseX, mouseY)));
}

void VideoProcessThread::setFrameControlStatus(bool status){
    isFrameControlEnabled = status;
}

void VideoProcessThread::mouseCordChange(int x, int y){
    mouseX = x;
    mouseY = y;
}

void VideoProcessThread::setContrastBorders(uint low, uint high){
    britnessLow = low;
    britnessHigh = high;
}
