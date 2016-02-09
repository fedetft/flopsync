/***************************************************************************
 *   Copyright (C) 2016 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "window.h"
#include <cmath>
#include <cstdlib>
#include <qwt/qwt_symbol.h>
#include <qwt/qwt_legend.h>


using namespace std;

//
// class Window
//

Window::Window()
{
    this->setWindowTitle("WandStem energy viewer");
    this->resize(600,400);

    plot=new QwtPlot(this);
    plot->setTitle("Current draw");
    plot->setCanvasBackground(Qt::white);
    plot->setAxisScale(QwtPlot::yLeft,0.,20.);
    plot->setAxisScale(QwtPlot::xBottom,0.,showRange);

    grid=new QwtPlotGrid();
    grid->setPen(Qt::gray,1);
    grid->attach(plot);

    curve=new QwtPlotCurve();
    curve->setTitle("Current");
    curve->setPen(Qt::black,1);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased,true);
    curve->attach(plot);

    timer=new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(redraw()));
    serial=new QAsyncSerial("/dev/ttyUSB0",115200);
    connect(serial,SIGNAL(received(double)),this,SLOT(update(double)));
    timer->start(50); //20fps
    this->show();
}

Window::~Window()
{
    delete serial;
}

void Window::redraw()
{
    xData.clear();
    yData.clear();
    xData.reserve(maxSize);
    yData.reserve(maxSize);
    double x=tStart;
    for(auto it : rolling)
    {
        xData.push_back(x+=(1./sampleFreq));
        yData.push_back(it);
    }
    //This is not the fastest method. Although it is zero-copy, replot is
    //forced to scan the data twice, once to find the boundaries, and the second
    //to plot it. A better way for a live update plot would be subclassing
    //QwtSeriesData<T>
    curve->setRawSamples(xData.data(),yData.data(),xData.size());
    if(rolling.size()==maxSize)
    {
        double tEnd=tStart+static_cast<double>(maxSize)/sampleFreq;
        plot->setAxisScale(QwtPlot::xBottom,tEnd-showRange,tEnd);
    }
    plot->replot();
}

void Window::update(double value)
{
    rolling.push_back(value);
    if(rolling.size()>maxSize)
    {
        rolling.pop_front();
        tStart+=1./sampleFreq;
    }
}
