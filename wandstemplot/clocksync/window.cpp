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
#include <iostream>
#include <sstream>
#include <qwt/qwt_symbol.h>
#include <qwt/qwt_legend.h>
#include <QRegExp>

using namespace std;

//
// class Window
//

Window::Window()
{
    this->setWindowTitle("Flopsync viewer");
    this->resize(600,400);

    plot=new QwtPlot(this);
    plot->setTitle("Synchronization error");
    plot->setCanvasBackground(Qt::white);
//     plot->setAxisScale(QwtPlot::yLeft,0.,25.);
//     plot->setAxisScale(QwtPlot::xBottom,0.,showRange);

    grid=new QwtPlotGrid();
    grid->setPen(Qt::gray,1);
    grid->attach(plot);

    curve=new QwtPlotCurve();
    curve->setTitle("Sync error");
    curve->setPen(Qt::black,1);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased,true);
    curve->attach(plot);

    serial=new QAsyncSerial("/dev/ttyUSB0",115200);
    connect(serial,SIGNAL(lineReceived(QString)),this,SLOT(update(QString)));
    this->show();
}

Window::~Window()
{
    delete serial;
}

void Window::update(QString data)
{
	cout<<data.toStdString()<<endl;
	
	QRegExp rx("e=(-?\\d+).*");
	if(rx.indexIn(data,0)!=-1)
	{
		stringstream ss(rx.cap(1).toStdString());
		double d;
		ss>>d;
		
		static double x=0.;
		xData.push_back(x++);
		yData.push_back(d/48e6);
	}
	
    //This is not the fastest method. Although it is zero-copy, replot is
    //forced to scan the data twice, once to find the boundaries, and the second
    //to plot it. A better way for a live update plot would be subclassing
    //QwtSeriesData<T>
    curve->setRawSamples(xData.data(),yData.data(),xData.size());
//     if(x<50.) plot->setAxisScale(QwtPlot::xBottom,0.,50.);
//     else plot->setAxisAutoScale(QwtPlot::xBottom,true);
    plot->replot();
}
