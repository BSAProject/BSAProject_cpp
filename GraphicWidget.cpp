#include "GraphicWidget.h"

#include <QPainter>
#include <QRect>
#include <QString>
#include <QRubberBand>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <algorithm>

#include <QGridLayout>
#include <QPalette>

#include <QDateTime>

using namespace std;

GraphicWidget::GraphicWidget(QWidget *parent, Qt::WindowFlags flags): QWidget(parent, flags), rubberBandIsShown(false),
                rubber(new QRubberBand(QRubberBand::Rectangle, this))
{

    // Инициализация необходимых параметров
    setPlotSettings(PlotSettings());

}

GraphicWidget::~GraphicWidget()
{}

// округление до заданной точности
double roundTo(double inpValue, int inpCount)
{
    double outpValue;
    double tempVal;
    tempVal=inpValue*pow(10,inpCount);
    if (double(int(tempVal))+0.5==tempVal)
    {
        if (int(tempVal)%2==0)
            {outpValue=double(floor(tempVal))/pow(10,inpCount);}
        else
            {outpValue=double(ceil(tempVal))/pow(10,inpCount);}
    }
    else
    {
        if (double(int(tempVal))+0.5>tempVal)
            {outpValue=double(ceil(tempVal))/pow(10,inpCount);}
        else
            {outpValue=double(floor(tempVal))/pow(10,inpCount);}
    }
    return(outpValue);
}

// Отрисовка сетки
void GraphicWidget::drawGrid(QPainter *painter)
{
    // точность подписей разметки
    int baseX = std::max(-log10(settings.spanX() / settings.numXTicks) + 1, 0.0);
    int baseY = std::max(-log10(settings.spanY() / settings.numYTicks) + 1, 0.0);

    // очистка превышения
    painter->eraseRect(0, 0, settings.MarginX + 5*baseY, this->rect().height()); // ось Y слева от прямоугольника с графиком
    painter->eraseRect(0, this->rect().height() - settings.MarginY * (1 + !settings.axisX2title.isEmpty()), this->rect().width(), this->rect().height()); // ось X внизу
    painter->eraseRect(this->rect().width() - settings.MarginX*settings.axisY2IsVisible + 5*baseY, 0, this->rect().width(), this->rect().height()); // для оси Y2 справа

    QRect rect(settings.MarginX + 5*baseY, settings.MarginY * (1 + settings.axisX2IsVisible + !settings.axisX2title.isEmpty()), this->rect().width() - settings.MarginX*(1+settings.axisY2IsVisible), this->rect().height() - settings.MarginY * (2 + settings.axisX2IsVisible + !settings.axisXtitle.isEmpty() + !settings.axisX2title.isEmpty()));
    if(!rect.isValid()) return;

    QRect boundString;
    double great_max = max(settings.numXTicks, settings.numYTicks) + 1;

    bool flagX = settings.autoAdjust || roundTo(settings.minX, baseX) == roundTo(settings.minXopt, baseX);
    bool flagX2 = settings.axisX2IsVisible && (settings.autoAdjust || roundTo(settings.minX2, baseX) == roundTo(settings.minX2opt, baseX));

    QDateTime curDateTime;
    curDateTime.setTimeSpec(Qt::UTC);

    for(register int i=0, j=0, k=0, l=0; i<=great_max; ++i, ++j, ++k, ++l)
    {
        if(j <= settings.numXTicks - (!flagX ? 1 : 0))                //< отрисовка вдоль оси X
        {
            int x = rect.left() + (j * (rect.width() - 1) / settings.numXTicks * settings.spanXopt() / settings.spanX()) - (j == 0 ? 0.0 : 1.0) * (settings.minX - settings.minXopt) / settings.spanX() * (rect.width() - 1);
            double label = settings.minXopt + (j * settings.spanXopt() / settings.numXTicks);

            // текст метки
            QString s_label;
            if(settings.mTickLabelTypeX == PlotSettings::mtNumber)
                s_label = QString::number(label, 'f', baseX);
            else if(settings.mTickLabelTypeX == PlotSettings::mtDateTime)
            {
                curDateTime.setTime_t(label);
                s_label = curDateTime.toString(settings.mDateTimeFormatX);
            }

            // рисуем сетку
            painter -> setPen(Qt::black);
            if( j == 0 || ( j == ( settings.numXTicks - (!flagX ? 1 : 0) ) ) )
                painter -> setPen(Qt::black);
            else
                painter -> setPen(Qt::DotLine);

            painter -> drawLine(x, rect.top(), x, rect.bottom());

            // рисуем метку
            int flags = Qt::AlignHCenter | Qt::AlignTop;
            boundString = painter -> boundingRect(boundString, flags, s_label);

            if(flagX || (j != 0))
                painter -> drawText(x - boundString.width()/2, rect.bottom() + settings.MarginY/4,
                                    boundString.width(), boundString.height(), flags, s_label);
        }
        if(k <= settings.numYTicks)                //< отрисовка вдоль оси Y
        {
            if(settings.mTickLabelTypeY == PlotSettings::mtList)
            {
                if(settings.mListY.count() > k)
                {
                    int y = rect.bottom() - ((settings.mListY.begin() + k).key() * (rect.height() - 1) / settings.maxY);

                    // рисуем метку
                    int flags = Qt::AlignRight | Qt::AlignTop;
                    boundString = painter -> boundingRect(boundString, flags, (settings.mListY.begin() + k).value());
                    painter -> drawText(rect.left() - (settings.MarginX + 5*0), y - boundString.height()/2,
                                        boundString.width(), boundString.height(), flags, (settings.mListY.begin() + k).value());
                }

            }
            else
            {
                int y = rect.bottom() - (k * (rect.height() - 1) / settings.numYTicks);
                double label = settings.minY + (k * settings.spanY() / settings.numYTicks);

                QString s_label(QString::number(label, 'f', baseY));

                // рисуем сетку
                painter -> setPen(Qt::black);

                if(!(k == 0 || (settings.axisX2IsVisible && k == settings.numYTicks)))
                    painter -> setPen(Qt::DotLine);

                painter -> drawLine(rect.left(), y, rect.right(), y);

                // рисуем метку
                int flags = Qt::AlignRight | Qt::AlignTop;
                boundString = painter -> boundingRect(boundString, flags, s_label);
                painter -> drawText(rect.left() - (settings.MarginX + 5*baseY), y - boundString.height()/2,
                                    boundString.width(), boundString.height(), flags, s_label);
            }
        }
        if(settings.axisY2IsVisible && l <= settings.numY2Ticks)                //< отрисовка вдоль оси Y2
        {
            if(settings.mTickLabelTypeY == PlotSettings::mtList)
            {
                if(settings.mListY2.count() > k)
                {
                    int y = rect.bottom() - ((settings.mListY2.begin() + k).key() * (rect.height() - 1) / settings.maxY2);

                    painter -> drawLine(rect.right(), y, rect.right() - 5, y);

                    // рисуем метку
                    int flags = Qt::AlignRight | Qt::AlignTop;
                    boundString = painter -> boundingRect(boundString, flags, (settings.mListY2.begin() + k).value());
                    painter -> drawText(rect.right(), y - boundString.height()/2,
                                        boundString.width(), boundString.height(), flags, (settings.mListY2.begin() + k).value());
                }

            }
            else
            {
                int y = rect.bottom() - (k * (rect.height() - 1) / settings.numY2Ticks);
                double label = settings.minY2 + (k * settings.spanY2() / settings.numY2Ticks);

                QString s_label(QString::number(label, 'f', baseY));

                // рисуем сетку
                painter -> setPen(Qt::black);

                if(!(k == 0 || (settings.axisX2IsVisible && k == settings.numYTicks)))
                    painter -> setPen(Qt::DotLine);

                painter -> drawLine(rect.left(), y, rect.right(), y);

                // рисуем метку
                int flags = Qt::AlignRight | Qt::AlignTop;
                boundString = painter -> boundingRect(boundString, flags, s_label);
                painter -> drawText(rect.left() - (settings.MarginX + 5*baseY), y - boundString.height()/2,
                                    boundString.width(), boundString.height(), flags, s_label);
            }
        }
        if(settings.axisX2IsVisible && i <= settings.numX2Ticks - (!flagX2 ? 1 : 0)) //< отрисовка вдоль оси X2
        {
            int x = rect.left() + (i * (rect.width() - 1) / settings.numX2Ticks * settings.spanX2opt() / settings.spanX2()) - (i == 0 ? 0.0 : 1.0) * (settings.minX2 - settings.minX2opt) / settings.spanX2() * (rect.width() - 1);
            double label = settings.minX2opt + (i * settings.spanX2opt() / settings.numX2Ticks);

            QString s_label;
            if(settings.mTickLabelTypeX == PlotSettings::mtNumber)
                s_label = QString::number(label, 'f', baseX);
            else if(settings.mTickLabelTypeX == PlotSettings::mtDateTime)
            {
                curDateTime.setTime_t(label);
                s_label = curDateTime.toString(settings.mDateTimeFormatX);
            }

            painter -> setPen(Qt::black);
            painter -> drawLine(x, rect.top(), x, rect.top() + 5);

            int flags = Qt::AlignHCenter | Qt::AlignTop;
            boundString = painter -> boundingRect(boundString, flags, s_label);

            if(i == 0)
                painter -> setPen(Qt::black);
            else
                painter -> setPen(QPen(settings.axisX2Color, 1, Qt::DotLine));

            painter -> drawLine(x, rect.top(), x, rect.bottom());

            painter->setPen(settings.axisX2Color);
            if(flagX2 || (i != 0))
                painter -> drawText(x - boundString.width()/2, rect.top() - settings.MarginY,
                                    boundString.width(), boundString.height(), flags, s_label);

            painter->setPen(Qt::black);
        }
    }
    painter -> drawRect(rect.adjusted(0, 0, -1, -1));

    if(!settings.title.isEmpty())
    {
        int flags = Qt::AlignHCenter | Qt::AlignTop;
        boundString = painter -> boundingRect(boundString, flags, settings.title);
        painter->setPen(settings.titleColor);
        painter -> drawText((this->rect().width() - boundString.width()) / 2, 0, boundString.width(), boundString.height(), flags, settings.title);
    }

    if(!settings.axisXtitle.isEmpty())
    {
        int flags = Qt::AlignHCenter | Qt::AlignTop;
        boundString = painter -> boundingRect(boundString, flags, settings.axisXtitle);
        painter->setPen(settings.axisXColor);
        painter -> drawText((this->rect().width() - boundString.width()) / 2, rect.bottom() + settings.MarginY, boundString.width(), boundString.height(), flags, settings.axisXtitle);
    }

    if(settings.axisX2IsVisible && !settings.axisX2title.isEmpty())
    {
        int flags = Qt::AlignHCenter | Qt::AlignTop;
        boundString = painter -> boundingRect(boundString, flags, settings.axisX2title);
        painter->setPen(settings.axisX2Color);
        painter -> drawText((this->rect().width() - boundString.width()) / 2, settings.MarginY, boundString.width(), boundString.height(), flags, settings.axisX2title);
    }
}

// Инициализация координат - преобразование из координат графика (sx,sy) в экранные (x,y)
QPointF GraphicWidget::initXY(double sx, double sy)
{
    // точность подписей разметки
    int baseY = std::max(-log10(settings.spanY() / settings.numYTicks) + 1, 0.0);

    QRect rect(settings.MarginX + 5*baseY, settings.MarginY * (1 + settings.axisX2IsVisible + !settings.axisX2title.isEmpty()), this->rect().width() - settings.MarginX*(1+settings.axisY2IsVisible), this->rect().height() - settings.MarginY * (2 + settings.axisX2IsVisible + !settings.axisXtitle.isEmpty()+ !settings.axisX2title.isEmpty()));
    double dx, dy;

    // Вычисление смещений вдоль осей
    dx = sx - settings.maxX;
    dy = sy - settings.minY;

    // Вычисление экранных координат
    double x = rect.right() + (dx * (rect.width() - 1) / (settings.spanX()));
    double y = rect.bottom() - (dy * (rect.height() - 1) / settings.spanY());

    return QPointF(x, y);
}

// Отрисовка графика
void GraphicWidget::drawCurves(QPainter* painter)
{
    foreach (QVector<QPointF> curve_vec, curve_vecs) {
        painter -> setPen(curveColor[curve_vecs.indexOf(curve_vec)]);

        QPolygonF polyline(curve_vec.size());
        for(register int i=0; i<curve_vec.size(); ++i)
            polyline[i] = initXY(curve_vec[i].x(), curve_vec[i].y());

        painter -> drawPolyline(polyline);
        painter->setPen(Qt::black);
    }
}

// Отрисовка графика
void GraphicWidget::paintEvent(QPaintEvent* events)
{
    QPainter painter(this);

    drawCurves(&painter);
    drawGrid(&painter);

    QWidget::paintEvent(events);
}

// Нажатие на кнопки клавиатуры
void GraphicWidget::keyPressEvent(QKeyEvent* events)
{
    switch(events -> key())
    {
    case Qt::Key_Plus:
        zoom(-1.0);
    break;
    case Qt::Key_Minus:
        zoom(1.0);
    break;
    case Qt::Key_Left:
        settings.scroll(-1.0, 0.0);
        update();
    break;
    case Qt::Key_Right:
        settings.scroll(1.0, 0.0);
        update();
    break;
    case Qt::Key_Up:
        settings.scroll(0.0, 1.0);
        update();
    break;
    case Qt::Key_Down:
        settings.scroll(0.0, -1.0);
        update();
    break;
    case Qt::Key_X:
        if(events -> modifiers() & Qt::ALT) close();
    break;
    case Qt::Key_Return:
        if(events -> modifiers() & Qt::ALT)
        {
            if(!isMaximized()) showMaximized();
            else showNormal();
        }
    break;
    default:
        QWidget::keyPressEvent(events);
    }
}

// Изменение масштаба при движении колесика
void GraphicWidget::wheelEvent(QWheelEvent* events)
{
    int numDegrees = events -> delta() / 8;
    double numTicks = numDegrees / 15.0;

    zoom(numTicks);
    update();
}

// Нажатие на кнопку - изменение масштаба
void GraphicWidget::mousePressEvent(QMouseEvent* events)
{
    QWidget::mousePressEvent(events);

    QRect r;
    switch(events -> button())
    {
    case Qt::LeftButton:                //< если нажата левая кнопка мыши
        origin = events -> pos();
        rubberBandIsShown = true;
        setCursor(Qt::CrossCursor);
        r = QRect(origin, QSize());
        rubber -> setGeometry(r);
        rubber -> show();
        break;
    case Qt::RightButton:                //< если нажата правая кнопка мыши
        break;
    default:
        break;
    }
}

// Переопределение функции передвижения мыши
void GraphicWidget::mouseMoveEvent(QMouseEvent* events)
{
    if(rubberBandIsShown)
    {
        rubber -> setWindowOpacity(0.0);
        rubber -> setGeometry(QRect(origin, events -> pos()).normalized());
    }

    update();
}

// Возвращение прежнего вида курсору и изменение масштаба
void GraphicWidget::mouseReleaseEvent(QMouseEvent* events)
{
    if(events -> button() == Qt::LeftButton && rubberBandIsShown)
    {
        rubberBandIsShown = false;
        unsetCursor();

        QRect rect = rubber -> geometry().normalized();
        if(rect.width() < 10 || rect.height() < 10)
        {
            double dx = rect.width() / settings.spanX();
            double dy = rect.height() / settings.spanY();
            settings.scroll(dx, dy);

            update();
            return;
        }
        PlotSettings prevSettings = settings;

        double dx = prevSettings.spanX() / width();
        double dy = prevSettings.spanY() / height();

        settings.minX = prevSettings.minX + dx * rect.left();
        settings.maxX = prevSettings.minX + dx * rect.right();
        settings.minY = prevSettings.maxY - dy * rect.bottom();
        settings.maxY = prevSettings.maxY - dy * rect.top();

        if(settings.axisX2IsVisible)
        {
            settings.minX2 = prevSettings.minX2 + dx * rect.left();
            settings.maxX2 = prevSettings.minX2 + dx * rect.right();
        }

        if(settings.axisY2IsVisible)
        {
            settings.minY2 = prevSettings.maxY2 - dy * rect.bottom();
            settings.maxY2 = prevSettings.maxY2 - dy * rect.top();
        }

        settings.adjust();

        rubber -> hide();
        update();
    }
}

// сохранение в JPG файл
void GraphicWidget::save(const QString &fileName, const char* format, int width, int height)
{
    GraphicWidget *graph = new GraphicWidget;
    graph->setGeometry(QRect(0,0, width+(1 + settings.axisY2IsVisible)*settings.MarginX, height+settings.MarginY * (2 + settings.axisX2IsVisible + !settings.axisXtitle.isEmpty()+ !settings.axisX2title.isEmpty())));

    graph->curve_vecs.swap(curve_vecs);
    graph->curveColor.swap(curveColor);

    graph->setPlotSettings(settings);
    graph->update();

    graph->grab().save(fileName, format);

    curve_vecs.swap(graph->curve_vecs);
    curveColor.swap(graph->curveColor);

    delete graph;
}

void GraphicWidget::addCurve(QVector<QPointF>& curve_vec, QColor colorTrack)
{
    curve_vecs.push_back(curve_vec);
    curveColor.push_back(colorTrack);
}

void GraphicWidget::clearCurves()
{
    curve_vecs.clear();
    curveColor.clear();
}

