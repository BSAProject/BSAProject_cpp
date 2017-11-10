#include "PlotSettings.h"

PlotSettings::PlotSettings(): minX(-50.), minY(-4.), maxX(50.), maxY(45.) //Было:  maxY(45.)
  , mTickLabelTypeX(mtNumber), mTickLabelTypeY(mtNumber), autoAdjust(true), autoAdjustY(true), autoAdjustY2(true),
    MarginX(20), MarginY(20),
    axisX2IsVisible(false), axisY2IsVisible(false), axisXColor(Qt::black), axisX2Color(Qt::black), titleColor(Qt::black)
{
    numXTicks = 8;
    numYTicks = 8;
}

// Увеличение/уменьшение значения minX, maxX, minY, maxY на интервал между 2-мя отметками
void PlotSettings::scroll(double dx, double dy)
{
    double stepX = spanX() / numXTicks;
    minX += dx * stepX;
    maxX += dx * stepX;

    if(axisX2IsVisible)
    {
        double stepX2 = spanX2() / numX2Ticks;
        minX2 += dx * stepX2;
        maxX2 += dx * stepX2;
    }

    double stepY = spanY() / numYTicks;
    minY += dy * stepY;
    maxY += dy * stepY;

    if(axisY2IsVisible)
    {
        double stepY2 = spanY2() / numY2Ticks;
        minY2 += dy * stepY2;
        maxY2 += dy * stepY2;
    }

    adjust();
}

// Увеличение/уменьшение значения minX, maxX, minY, maxY на интервал между 2-мя отметками
void PlotSettings::scale(double delta_x, double delta_y)
{
    if((minX == maxX || minY == maxY) && delta_x < 0 && delta_y < 0) return;

    double stepX = spanX() / numXTicks;
    minX -= delta_x * stepX;
    maxX += delta_x * stepX;

    if(axisY2IsVisible)
    {
        double stepX2 = spanX2() / numX2Ticks;
        minX2 -= delta_x * stepX2;
        maxX2 += delta_x * stepX2;
    }

    double stepY = spanY() / numYTicks;
    minY -= delta_y * stepY;
    maxY += delta_y * stepY;

    if(axisY2IsVisible)
    {
        double stepY2 = spanY2() / numY2Ticks;
        minY2 -= delta_y * stepY2;
        maxY2 += delta_y * stepY2;
    }

}

// Округление значений minX, minY, maxX, maxY
void PlotSettings::adjust()
{
    minXopt = minX;
    maxXopt = maxX;
    minYopt = minY;
    maxYopt = maxY;

    if(axisX2IsVisible)
    {
        maxX2opt = maxX2;
        minX2opt = minX2;
    }

    if(axisY2IsVisible)
    {
        maxY2opt = maxY2;
        minY2opt = minY2;
    }

    adjustAxis(minXopt, maxXopt, numXTicks);

    if(autoAdjustY)
        adjustAxis(minYopt, maxYopt, numYTicks);

    if(axisX2IsVisible)
        adjustAxis(minX2opt, maxX2opt, numX2Ticks);

    if(axisY2IsVisible)
        adjustAxis(minY2opt, maxY2opt, numY2Ticks);

    if(autoAdjust)
    {
        minX = minXopt;
        maxX = maxXopt;
        minY = minYopt;
        maxY = maxYopt;

        if(axisX2IsVisible)
        {
            maxX2 = maxX2opt;
            minX2 = minX2opt;
        }

        if(axisY2IsVisible)
        {
            maxY2 = maxY2opt;
            minY2 = minY2opt;
        }

    }
}

// Округление значений заданной точки point
void PlotSettings::adjust(QPointF& point)
{
    double mn_x = minX, mn_y = minY;
    int ticks_x = numXTicks, ticks_y = numYTicks;
    double mx_x = point.x(), mx_y = point.y();

    adjustAxis(mn_x, mx_x, ticks_x);
    adjustAxis(mn_y, mx_y, ticks_y);

    point.setX(mx_x);
    point.setY(mx_y);
}

// Преобразование параметров в удобные значения
void PlotSettings::adjustAxis(double& min, double& max, int& numTicks)
{
    const int MinTicks = 5;
    double grossStep = (max - min) / MinTicks;
    double step = pow(10.0, floor(log10(grossStep)));

    if(5 * step < grossStep)
        step *= 5;
    else if(2 * step < grossStep)
        step *= 2;

    numTicks = int(ceil(max / step) - floor(min / step));
    if(numTicks < MinTicks)
        numTicks = MinTicks;

    min = floor(min / step) * step;
    max = ceil(max / step) * step;
}

// Уставновка параметров осей
void PlotSettings::setAxis(const char* nameA, double minA, double maxA,  const char* titleA, bool axisVisible, QColor colorA)
{
    if(!strcmp(nameA, "X"))
    {
        minX = minA;
        maxX = maxA;
        axisXColor = colorA;
        axisXtitle = titleA;
    }
    else if(!strcmp(nameA, "Y"))
    {
        minY = minA;
        maxY = maxA;
        axisYtitle = titleA;
    }
    else if(!strcmp(nameA, "X2"))
    {
        minX2 = minA;
        maxX2 = maxA;
        axisX2Color = colorA;
        axisX2title = titleA;
        axisX2IsVisible = axisVisible;
    }
    else if(!strcmp(nameA, "Y2"))
    {
        minY2 = minA;
        maxY2 = maxA;
        axisY2title = titleA;
        axisY2IsVisible = axisVisible;
    }
}
