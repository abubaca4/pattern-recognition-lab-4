#ifndef SELECTINGGRAPHICSSCENE_H
#define SELECTINGGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

class SelectingGraphicsScene: public QGraphicsScene
{
    Q_OBJECT

public:
    SelectingGraphicsScene(QObject *parent = nullptr);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

signals:
    void SelectionChanged(bool selection);
    void SelectionEnd();

public:
    bool isSelectionVisiable, selecting;
    QPointF selectionStart, selectionEnd;
};

#endif // SELECTINGGRAPHICSSCENE_H
