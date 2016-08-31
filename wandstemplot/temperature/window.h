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

#ifndef WINDOW_H
#define WINDOW_H

#include <vector>
#include <fstream>
#include <list>
#include <QSplitter>
#include <QTimer>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_curve.h>
#include "QAsyncSerial.h"

/**
 * Main window.
 * Derives from QSplitter instead of QWidget for automatic resizing of child
 * widgets.
 */
class Window : public QSplitter
{
Q_OBJECT
public:

    Window();

    ~Window();

private slots:

    void update(QString data);

private:
    QwtPlot *plot;
    QwtPlotGrid *grid;
    QwtPlotCurve *curve;
    QAsyncSerial *serial;

    std::vector<double> xData;
    std::vector<double> yData;
    std::ofstream log;
};

#endif //WINDOW_H
