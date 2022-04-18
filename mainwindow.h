#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QRegularExpression>
#include <QMessageBox>
#include <QKeyEvent>
#include <QPoint>
#include <QMap>
#include <QActionGroup>
#include <QLabel>

#include <vector>
#include <array>

#include "opencv2/opencv.hpp"

#include "videoPocessThread.h"
#include "SelectingGraphicsScene.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionOpen_file_triggered();
    void on_actionOpen_camera_triggered();

    void updateFrame(cv::Mat *mat);
    void trackingStatusChange(bool tracking);
    void startSelectionTracker();
    void setSelection(bool selection);
    void updateDetection(std::vector<std::array<int, 4>> *detectionData);
    void updateStats(qreal fps);

    void trackerChange(QAction* action);
    void detectionChange(QAction* action);

private:
    void setVideoprocessThread();
    void clearVideoprocessThread();

    Ui::MainWindow *ui;

    SelectingGraphicsScene imageScene;
    QMutex dataLock;
    cv::Mat currentFrame;
    VideoProcessThread *proc;

    QMap<QAction*, VideoProcessThread::selectingTrackerType> buttonToTracker;
    QActionGroup trackerSelectGroup;

    QMap<QAction*, VideoProcessThread::detectionType> buttonToDetection;
    QActionGroup detectionSelectGroup;

    QLabel statusLabel;
};
#endif // MAINWINDOW_H
