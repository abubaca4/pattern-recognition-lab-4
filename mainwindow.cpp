#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , imageScene(this)
    , proc(nullptr)
    , trackerSelectGroup(this)
    , detectionSelectGroup(this)
{
    ui->setupUi(this);

    ui->statusbar->addPermanentWidget(&statusLabel);

    ui->graphicsView->setScene(&imageScene);
    ui->graphicsView->setMouseTracking(true);
    connect(&imageScene, &SelectingGraphicsScene::SelectionChanged, this, &MainWindow::setSelection);
    connect(&imageScene, &SelectingGraphicsScene::SelectionEnd, this, &MainWindow::startSelectionTracker);

    buttonToTracker.insert(ui->actionCSRT, VideoProcessThread::selectingTrackerType::CSRT);
    buttonToTracker.insert(ui->actionKCF, VideoProcessThread::selectingTrackerType::KCF);
    buttonToTracker.insert(ui->actionMIL, VideoProcessThread::selectingTrackerType::MIL);
    buttonToTracker.insert(ui->actionMOSSE, VideoProcessThread::selectingTrackerType::MOSSE);
    buttonToTracker.insert(ui->actionBoosting, VideoProcessThread::selectingTrackerType::Boosting);
    buttonToTracker.insert(ui->actionMedianFlow, VideoProcessThread::selectingTrackerType::MedianFlow);
    buttonToTracker.insert(ui->actionTLD, VideoProcessThread::selectingTrackerType::TLD);

    trackerSelectGroup.setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);
    foreach(QAction *action, ui->menuTracker_select->actions()){
        trackerSelectGroup.addAction(action);
    }
    connect(&trackerSelectGroup, &QActionGroup::triggered, this, &MainWindow::trackerChange);

    buttonToDetection.insert(ui->actionManual, VideoProcessThread::detectionType::No);
    buttonToDetection.insert(ui->actionMotion, VideoProcessThread::detectionType::Motion);
    buttonToDetection.insert(ui->actionContrast, VideoProcessThread::detectionType::Contrast);

    detectionSelectGroup.setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);
    foreach(QAction *action, ui->menuSelection_mode->actions()){
        detectionSelectGroup.addAction(action);
    }
    connect(&detectionSelectGroup, &QActionGroup::triggered, this, &MainWindow::detectionChange);
    detectionChange(nullptr);

    contrastBritnessLow = 150;
    contrastBritnessHigh = 255;
}

MainWindow::~MainWindow()
{
    disconnect(&imageScene, &SelectingGraphicsScene::SelectionChanged, this, &MainWindow::setSelection);
    disconnect(&imageScene, &SelectingGraphicsScene::SelectionEnd, this, &MainWindow::startSelectionTracker);
    disconnect(&trackerSelectGroup, &QActionGroup::triggered, this, &MainWindow::trackerChange);
    disconnect(&detectionSelectGroup, &QActionGroup::triggered, this, &MainWindow::detectionChange);
    clearVideoprocessThread();
    delete ui;
}

void MainWindow::trackerChange(QAction* action){
    Q_UNUSED(action);
    if (proc != nullptr){
        if (buttonToTracker.contains(trackerSelectGroup.checkedAction())){
            proc->changeSelectionTracker(buttonToTracker[trackerSelectGroup.checkedAction()]);
        }
    }
}

void MainWindow::detectionChange(QAction* action){
    Q_UNUSED(action);
    if (buttonToDetection.contains(detectionSelectGroup.checkedAction())){
        auto detectionMode = buttonToDetection[detectionSelectGroup.checkedAction()];
        switch (detectionMode) {
        case VideoProcessThread::detectionType::Motion:
        case VideoProcessThread::detectionType::Contrast:
            imageScene.selectionT = SelectingGraphicsScene::selectionType::borders;
            break;
        case VideoProcessThread::detectionType::No:
        default:
            imageScene.selectionT = SelectingGraphicsScene::selectionType::Manual;
            break;
        }
        if (proc != nullptr){
            proc->changeDetectionType(detectionMode);
        }
    }
}

void MainWindow::on_actionOpen_file_triggered()
{
    clearVideoprocessThread();
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Open video file ..."));
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Videos (*.mp4 *.avi)"));
    static QRegularExpression name_chacker = QRegularExpression(".+\\.(mp4|avi)");
    QStringList fileNames;
    if (dialog.exec()) {
        fileNames = dialog.selectedFiles();
        if (name_chacker.match(fileNames.at(0)).hasMatch()) {
            proc = new VideoProcessThread(fileNames.at(0), &dataLock);
        } else {
            QMessageBox::information(this, tr("Information"), tr("Open error: bad format of filename."));
        }
    } else {
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
        connect(proc, &VideoProcessThread::trackingStatusUpdate, this, &MainWindow::trackingStatusChange);
        connect(proc, &VideoProcessThread::detectionChanged, this, &MainWindow::updateDetection);
        connect(proc, &VideoProcessThread::statsChanged, this, &MainWindow::updateStats);
        connect(&imageScene, &SelectingGraphicsScene::mouseMove, proc, &VideoProcessThread::mouseCordChange);
        proc->start();
        trackerChange(nullptr);
        detectionChange(nullptr);
        on_actionFrame_control_triggered();
        proc->setContrastBorders(contrastBritnessLow, contrastBritnessHigh);
    }
}

inline void MainWindow::clearVideoprocessThread(){
    if (proc != nullptr){
        proc->setRunning(false);
        disconnect(proc, &VideoProcessThread::frameChanged, this, &MainWindow::updateFrame);
        disconnect(proc, &VideoProcessThread::trackingStatusUpdate, this, &MainWindow::trackingStatusChange);
        disconnect(proc, &VideoProcessThread::detectionChanged, this, &MainWindow::updateDetection);
        disconnect(proc, &VideoProcessThread::statsChanged, this, &MainWindow::updateStats);
        disconnect(&imageScene, &SelectingGraphicsScene::mouseMove, proc, &VideoProcessThread::mouseCordChange);
        connect(proc, &VideoProcessThread::finished, proc, &VideoProcessThread::deleteLater);    
        proc = nullptr;
        imageScene.isSelectionVisiable = false;
    }
}

void MainWindow::startSelectionTracker(){
    if (proc != nullptr){
        proc->startSelectionTracker();
    }
}

void MainWindow::trackingStatusChange(bool tracking){
    imageScene.isSelectionVisiable = tracking;
}

void MainWindow::updateDetection(std::vector<std::array<int, 4>> *detectionData){
    proc->detBorderLock.lock();
    imageScene.detectionBorders = *detectionData;
    proc->detBorderLock.unlock();
}

void MainWindow::updateStats(qreal fps, qreal mean, qreal std, qreal min, qreal max, int x, int y, int britness){
    static std::string status = tr("FPS: %.2f, mean: %.2f, std: %.2f, min/max pix int: (%.2f, %.2f), mouse x/y/britnes (%d, %d, %d)").toStdString();
    statusLabel.setText(QString::asprintf(status.c_str(), fps, mean, std, min, max, x, y, britness));
}

void MainWindow::on_actionFrame_control_triggered()
{
    if (proc != nullptr){
        proc->setFrameControlStatus(ui->actionFrame_control->isChecked());
    }
}

void MainWindow::on_actionSet_contrast_borders_triggered()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Contrast settings"));

    QLabel title(tr("If low border smaller than high they will be swapped"), &dialog);

    QIntValidator borderValidator(0, 255, &dialog);

    QLabel lowBorderLabel(tr("Low britness border:"), &dialog);
    QLineEdit lowBorderEdit(QString::number(contrastBritnessLow), &dialog);
    lowBorderEdit.setValidator(&borderValidator);
    lowBorderLabel.setBuddy(&lowBorderEdit);

    QLabel highBorderLabel(tr("High britness border:"), &dialog);
    QLineEdit highBorderEdit(QString::number(contrastBritnessHigh), &dialog);
    highBorderEdit.setValidator(&borderValidator);
    highBorderLabel.setBuddy(&highBorderEdit);

    QPushButton okButton(tr("Ok"), &dialog), closeButton(tr("Close"), &dialog);
    connect(&closeButton, &QPushButton::clicked, &dialog, &QDialog::close);
    connect(&okButton, &QPushButton::clicked, this, [&](bool checked){
        Q_UNUSED(checked);
        contrastBritnessLow = qMin(lowBorderEdit.text().toInt(), highBorderEdit.text().toInt());
        contrastBritnessHigh = qMax(lowBorderEdit.text().toInt(), highBorderEdit.text().toInt());
        dialog.accept();
    });

    QVBoxLayout mainLayout(&dialog);
    QHBoxLayout secondLine, thirdLine, fourthLine;
    mainLayout.addWidget(&title);
    secondLine.addWidget(&lowBorderLabel);
    secondLine.addWidget(&lowBorderEdit);
    mainLayout.addLayout(&secondLine);
    thirdLine.addWidget(&highBorderLabel);
    thirdLine.addWidget(&highBorderEdit);
    mainLayout.addLayout(&thirdLine);
    fourthLine.addWidget(&okButton);
    fourthLine.addWidget(&closeButton);
    mainLayout.addLayout(&fourthLine);

    if (dialog.exec() && proc != nullptr) {
        proc->setContrastBorders(contrastBritnessLow, contrastBritnessHigh);
    }
}
