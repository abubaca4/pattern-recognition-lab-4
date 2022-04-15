#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , imageScene(this)
    , proc(nullptr)
{
    ui->setupUi(this);

    ui->graphicsView->setScene(&imageScene);
    connect(&imageScene, &SelectingGraphicsScene::SelectionChanged, this, &MainWindow::setSelection);
    connect(&imageScene, &SelectingGraphicsScene::SelectionEnd, this, &MainWindow::startSelectionTracker);
}

MainWindow::~MainWindow()
{
    disconnect(&imageScene, &SelectingGraphicsScene::SelectionChanged, this, &MainWindow::setSelection);
    disconnect(&imageScene, &SelectingGraphicsScene::SelectionEnd, this, &MainWindow::startSelectionTracker);
    delete ui;
}

void MainWindow::on_actionOpen_file_triggered()
{
    clearVideoprocessThread();
    QFileDialog dialog(this);
    dialog.setWindowTitle("Open video file ...");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Videos (*.mp4 *.avi)"));
    static QRegularExpression name_chacker = QRegularExpression(".+\\.(mp4|avi)");
    QStringList fileNames;
    if (dialog.exec()) {
        fileNames = dialog.selectedFiles();
        if (name_chacker.match(fileNames.at(0)).hasMatch()) {
            proc = new VideoProcessThread(fileNames.at(0), &dataLock);
        } else {
            QMessageBox::information(this, "Information", "Open error: bad format of filename.");
        }
    }
    if (fileNames.isEmpty()){
        return;
    }
    setVideoprocessThread();
}

void MainWindow::on_actionOpen_camera_triggered()
{
    int camID = 0;
    clearVideoprocessThread();
    proc = new VideoProcessThread(camID, &dataLock);
    setVideoprocessThread();
}

void MainWindow::updateFrame(cv::Mat *mat)
{
    dataLock.lock();
    mat->copyTo(currentFrame);
    dataLock.unlock();
    QImage frame(
                currentFrame.data,
                currentFrame.cols,
                currentFrame.rows,
                currentFrame.step,
                QImage::Format_RGB888);
    QPixmap image = QPixmap::fromImage(frame);
    imageScene.clear();
    ui->graphicsView->resetTransform();
    imageScene.addPixmap(image);
    imageScene.update();
    ui->graphicsView->setSceneRect(image.rect());
}

void MainWindow::setSelection(bool selection)
{
    if (proc != nullptr){
        if (selection != imageScene.isSelectionVisiable){
            proc->setSelectionVisiable(selection);
        }
        if (selection){
            proc->setSelecting(imageScene.selectionStart.x(), imageScene.selectionStart.y(), imageScene.selectionEnd.x(), imageScene.selectionEnd.y());
        }
    }
    imageScene.isSelectionVisiable = selection;
}


inline void MainWindow::setVideoprocessThread(){
    if (proc != nullptr){
        connect(proc, &VideoProcessThread::frameChanged, this, &MainWindow::updateFrame);
        proc->start();
    }
}

inline void MainWindow::clearVideoprocessThread(){
    if (proc != nullptr){
        proc->setRunning(false);
        disconnect(proc, &VideoProcessThread::frameChanged, this, &MainWindow::updateFrame);
        connect(proc, &VideoProcessThread::finished, proc, &VideoProcessThread::deleteLater);
        proc = nullptr;
    }
}

void MainWindow::startSelectionTracker(){
    if (proc != nullptr){
        proc->startSelectionTracker();
    }
}
