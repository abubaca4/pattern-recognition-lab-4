#include "SelectingGraphicsScene.h"

SelectingGraphicsScene::SelectingGraphicsScene(QObject *parent):
    QGraphicsScene(parent),
    isSelectionVisiable(false),
    selecting(false),
    selectionT(Manual)
{};

void SelectingGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
        switch (selectionT) {
        case Manual:
            if (!isSelectionVisiable){
                selecting = true;
                selectionStart = event->scenePos();
            }
            break;

        case borders:
            if (detectionBorders.size()){
                qreal minLen = std::numeric_limits<qreal>::max();
                for (auto &i: detectionBorders){
                    qreal len = lengthPtP(event->scenePos().x(), event->scenePos().y(), (i[0] + i[2])/2,  (i[1] + i[3])/2);
                    if (len < minLen){
                        selectionStart = QPoint(i[0], i[1]);
                        selectionEnd = QPoint(i[2], i[3]);
                        minLen = len;
                    }
                }
                emit SelectionChanged(true);
                emit SelectionEnd();
            }
            break;
        default:
            break;
        }
        break;

    case Qt::RightButton:
        if (isSelectionVisiable){
            emit SelectionChanged(false);
        }
        break;

    default:
        break;
    }
}

void SelectingGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (selecting) {
        selectionEnd = event->scenePos();
        emit SelectionChanged(true);
    }
}

void SelectingGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (selecting){
            selecting = false;
            selectionEnd = event->scenePos();
            emit SelectionChanged(true);
            emit SelectionEnd();
        }
    }
}

inline qreal SelectingGraphicsScene::lengthPtP(int x1, int y1, int x2, int y2){
    return qSqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}
