#ifndef GRAPHICWIDGET_H
#define GRAPHICWIDGET_H

#include "PlotSettings.h"

#include <QWidget>
#include <QPoint>

class QPaintEvent;
class QPainter;
class QRubberBand;
class QKeyEvent;
class QResizeEvent;
class QCloseEvent;
class QShowEvent;
class QWheelEvent;
class QMouseEvent;

// Виджет, где будет отрисовываться график
class GraphicWidget: public QWidget
{
    Q_OBJECT

private:
    PlotSettings settings;        //< настройка для различных масштабов
    QRubberBand* rubber;        //< "резиновая лента"
    bool rubberBandIsShown;        //< флажок попадания курсора в "резиновую ленту"
    QPoint origin;                //< начальные координаты выделяемой области

private:
    void drawGrid(QPainter *painter);
    void drawCurves(QPainter* painter);
    QPointF initXY(double sx, double sy);

protected:
    void paintEvent(QPaintEvent* events);
    void keyPressEvent(QKeyEvent* events);
    void wheelEvent(QWheelEvent* events);
    void mousePressEvent(QMouseEvent* events);
    void mouseMoveEvent(QMouseEvent* events);
    void mouseReleaseEvent(QMouseEvent* events);
    void resizeEvent(QResizeEvent* events) { QWidget::resizeEvent(events); update(); }
    void closeEvent(QCloseEvent* events) { QWidget::closeEvent(events); }
    void showEvent(QShowEvent* events) { QWidget::showEvent(events); }

public:
    GraphicWidget(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~GraphicWidget();
    void setPlotSettings(const PlotSettings& sts) { settings = sts; settings.adjust(); update(); }
    void zoom(double delta) { settings.scale(delta, delta); settings.adjust(); update(); }

    QVector< QVector<QPointF>> curve_vecs; // вектор кривых
    QVector<QColor> curveColor; // вектор цветов кривых

    void addCurve(QVector<QPointF>& curve_vec, QColor colorTrack=Qt::blue);
    void clearCurves();

    void save(const QString &fileName, const char* format, int width=0, int height=0);

};

#endif // GRAPHICWIDGET_H
