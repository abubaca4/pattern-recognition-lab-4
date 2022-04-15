#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QRegularExpression>
#include <QMessageBox>
#include <QKeyEvent>
#include <QPoint>
#include <QMap>

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
    void setSelection(bool selection);
    void startSelectionTracker();

private:
    void updateSelection();
    void setVideoprocessThread();
    void clearVideoprocessThread();

    Ui::MainWindow *ui;

    SelectingGraphicsScene imageScene;
    QMutex dataLock;
    cv::Mat currentFrame;
    VideoProcessThread *proc;
};
#endif // MAINWINDOW_H
