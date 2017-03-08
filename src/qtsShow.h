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
#ifndef _qtsShow_h
#define _qtsShow_h

#include <QWidget>
#include "tsDrawArea.h"
#include <puTools/miTime.h>

class qtsShow : public QWidget {
  Q_OBJECT

private:
  tsDrawArea* drawArea;
  pets2::ptQCanvas canvas;

protected:
  void resizeEvent(QResizeEvent*);
  void paintEvent(QPaintEvent*);

public:
  qtsShow(tsRequest*,DatafileColl*, SessionManager*, QWidget* parent=0);

  void refresh(bool readData = false);

  void paintOn(QPaintDevice* device);

  void setTimemark(const miutil::miTime& time, const std::string& name="");
  void clearTimemarks(const std::string& name="");
  void setProgintervall(int mi,int ma) { drawArea->setProgintervall(mi,ma);}
  void setShowObservations(bool showObs) {drawArea->setShowObservations(showObs);}
  void getTimeRange(int& t,int& f) { drawArea->getTimeRange(t,f);}
  void setObservationStartTime(miutil::miTime st) {drawArea->setObservationStartTime(st);}
  miutil::miTime getObservationStartTime() const { return drawArea->getObservationStartTime(); }
  void setShowGridLines( bool s ){ drawArea->setShowGridLines(s); }

public Q_SLOTS:
  void post_dataLoad(tsDrawArea::DataloadRequest);

Q_SIGNALS:
  void refreshFinished();
  void newTimeRange(int total, int fcast);
  void dataread_started();
  void dataread_ended();
};

#endif
