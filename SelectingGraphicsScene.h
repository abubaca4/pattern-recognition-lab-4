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

public:
    bool is_selection_worked, selecting;
    QPointF selection_start, selection_end;
};

#endif // SELECTINGGRAPHICSSCENE_H
