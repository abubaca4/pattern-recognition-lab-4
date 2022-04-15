#include "SelectingGraphicsScene.h"

SelectingGraphicsScene::SelectingGraphicsScene(QObject *parent):
    QGraphicsScene(parent),
    isSelectionVisiable(false),
    selecting(false)
{};

void SelectingGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
        if (!isSelectionVisiable){
            selecting = true;
            selectionStart = event->scenePos();
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
