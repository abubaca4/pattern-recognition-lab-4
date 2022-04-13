#include "SelectingGraphicsScene.h"

SelectingGraphicsScene::SelectingGraphicsScene(QObject *parent):
    QGraphicsScene(parent),
    is_selection_visiable(false),
    selecting(false)
{};

void SelectingGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
        if (!is_selection_visiable){
            selecting = true;
            selection_start = event->scenePos();
        }
        break;

    case Qt::RightButton:
        if (is_selection_visiable){
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
        selection_end = event->scenePos();
        emit SelectionChanged(true);
    }
}

void SelectingGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (selecting){
            selecting = false;
            selection_end = event->scenePos();
            emit SelectionChanged(true);
            emit SelectionEnd();
        }
    }
}
