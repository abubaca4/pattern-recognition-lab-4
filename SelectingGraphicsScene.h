#ifndef SELECTINGGRAPHICSSCENE_H
#define SELECTINGGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QtMath>

#include <vector>
#include <array>
#include <limits>

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
    void mouseMove(int x, int y);

public:
    bool isSelectionVisiable, selecting;
    QPointF selectionStart, selectionEnd;
    std::vector<std::array<int, 4>> detectionBorders;

    enum selectionType{
        Manual,
        borders
    };

    selectionType selectionT;

private:
    static qreal lengthPtP(int x1, int y1, int x2, int y2);
};

#endif // SELECTINGGRAPHICSSCENE_H
