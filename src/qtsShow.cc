/*
  Tseries - A Free Meteorological Timeseries Viewer

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Tseries

  Tseries is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Tseries is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Tseries; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "qtsShow.h"

qtsShow::qtsShow(tsRequest* tsr, DatafileColl* tsd, SessionManager* ses, QWidget* parent)
  : QWidget(parent)
  , canvas(this)
{
  drawArea = new tsDrawArea(tsr,tsd,ses,this);

  connect(drawArea,SIGNAL(post_dataLoad(tsDrawArea::DataloadRequest)),this,SLOT(post_dataLoad(tsDrawArea::DataloadRequest)));

  connect(drawArea,SIGNAL(dataread_started()),this, SIGNAL(dataread_started()));
  connect(drawArea,SIGNAL(dataread_ended())  ,this, SIGNAL(dataread_ended()));
}


void qtsShow::paintEvent(QPaintEvent*)
{
  drawArea->setDataloadRequest(tsDrawArea::dataload_paintGL);
  drawArea->prepare(false);

  pets2::ptQPainter painter(&canvas);
  //painter.qPainter().setRenderHint(QPainter::Antialiasing, true);
  drawArea->plot(painter);
}


void qtsShow::post_dataLoad(tsDrawArea::DataloadRequest dlRequest)
{
  switch(dlRequest) {
  case tsDrawArea::dataload_paintGL:
    update();
    break;
  case tsDrawArea::dataload_refresh:
    refresh(false);
    break;
  case tsDrawArea::dataload_hardcopy:
    // FIXME post_hardcopy();
    break;
  }
}

//  Set up the OpenGL view port, matrix mode, etc.
void qtsShow::resizeEvent(QResizeEvent*)
{
  canvas.update();
  drawArea->setViewport(&canvas);
}

void qtsShow::refresh(bool /*readData*/)
{
  drawArea->setDataloadRequest(tsDrawArea::dataload_refresh);
  drawArea->prepare(false);

  update();

  if(drawArea->newLength()) {
    int totall, fcastl;
    drawArea->getMaxIntervall(totall,fcastl);
    drawArea->resetNewLength();
    Q_EMIT newTimeRange(totall,fcastl);
  }

  Q_EMIT refreshFinished();
}


void qtsShow::paintOn(QPaintDevice* device)
{
  pets2::ptQCanvas pcanvas(device);
  drawArea->setViewport(&pcanvas);

  drawArea->setDataloadRequest(tsDrawArea::dataload_hardcopy);
  drawArea->prepare(true);

  pets2::ptQPainter ppainter(&pcanvas);
  drawArea->plot(ppainter);

  drawArea->setViewport(&canvas);
}

void qtsShow::setTimemark(const miutil::miTime& tim, const std::string& nam)
{
  drawArea->setTimemark(tim,nam);
  refresh(false);
}

void qtsShow::clearTimemarks(const std::string& nam)
{
  drawArea->clearTimemarks(nam);
  refresh(false);
}
