#ifndef PLOTSETTINGS_H
#define PLOTSETTINGS_H

#include <QPointF>
#include <cmath>

#include <QString>
#include <QColor>
#include <QMap>
#include <QFont>

// Абстрактный класс для задания диапазона значений по осям x и y
class PlotSettings
{
public:
    // По условному соглашению значение в numXTicks и numYTicks задается на единицу меньше; если numXTicks равно 5, будет на самом деле выводить 6 отметок по оси х.

    double minX;                        //< минимальное значение по оси абсцисс
    double maxX;                        //< максимальное значение по оси абсцисс
    int numXTicks;                        //< количество делений на оси абсцисс
    double minY;                        //< минимальное значение по оси ординат
    double maxY;                        //< максимальное значение по оси ординат
    int numYTicks;                        //< количество делений на оси ординат

    enum LabelType { mtNumber,    //< Tick coordinate is regarded as normal number and will be displayed as such.
                     mtDateTime, //< Tick coordinate is regarded as a date/time (seconds since 1970-01-01T00:00:00 UTC) and will be displayed and formatted as such. (for details, see \ref setDateTimeFormat)
                     mtList
                   };

    LabelType mTickLabelTypeX;
    LabelType mTickLabelTypeY;

    QString mDateTimeFormatX;
    QString mDateTimeFormatY;

    bool autoAdjust, autoAdjustY, autoAdjustY2; // автоматическое форматирование меток
    double minXopt, maxXopt, minYopt, maxYopt, minX2opt, maxX2opt, minY2opt, maxY2opt;
    QString title; // название графика
    int MarginX, MarginY; // размер свободного пространства вокруг диаграммы
    QMap<double, QString> mListY, mListY2; // список меток с привязкой к координате

    bool axisX2IsVisible, axisY2IsVisible;
    double minX2, minY2;                        //< минимальное значение по оси абсцисс
    double maxX2, maxY2;                        //< максимальное значение по оси абсцисс
    int numX2Ticks, numY2Ticks;                      //< количество делений на оси абсцисс

    QString axisXtitle, axisX2title, axisYtitle, axisY2title;
    QColor axisXColor, axisX2Color, titleColor, tickLabelYColor;
    QFont tickLabelYFont;

protected:
    void adjustAxis(double& min, double& max, int& numTicks);

public:
    PlotSettings();
    void scroll(double dx, double dy);
    void scale(double delta_x, double delta_y);
    void adjust();
    void adjust(QPointF& point);

    double spanX() const { return fabs(maxX - minX); }
    double spanY() const { return fabs(maxY - minY); }
    double spanX2() const { return fabs(maxX2 - minX2); }
    double spanY2() const { return fabs(maxY2 - minY2); }

    double spanXopt() const { return fabs(maxXopt - minXopt); }
    double spanYopt() const { return fabs(maxYopt - minYopt); }
    double spanX2opt() const { return fabs(maxX2opt - minX2opt); }
    double spanY2opt() const { return fabs(maxY2opt - minY2opt); }

    void setAxis(const char* nameA, double minA=-50., double maxA=50., const char* titleA="", bool axisVisible = true, QColor colorA=Qt::black);
};

#endif
