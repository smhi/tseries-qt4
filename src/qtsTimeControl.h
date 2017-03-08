/*
  Tseries - A Free Meteorological Timeseries Viewer

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Tseries is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Tseries is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef _qtstimeControl_h
#define _qtstimeControl_h

#include <QSlider>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <string>

class TimeControl : public QFrame {
  Q_OBJECT
private:
  QSlider* startSlider;
  QSlider* stopSlider;
  QLabel*  startLabel;
  QLabel*  stopLabel;
  int totalrange;
  int fcastrange;
  int obsrange;
  bool lockHours;

public:
  TimeControl(QWidget*);
  void setLengthSlider(int);
  void setStartSlider(int v);
  int getLengthValue();
  int getStartValue();
  void setTimeRange(int, int);
  std::string getTimecontrolLog();
  void setTimecontrolFromlLog(  std::string t);
  void toggleLockHoursToModel(bool lockHoursToModel) {lockHours = lockHoursToModel;}

Q_SIGNALS:
  void minmaxProg(int,int);

private slots:
  void minmaxSlot();
  void startchanged(int v);
  void stopchanged(int v);
};

#endif
