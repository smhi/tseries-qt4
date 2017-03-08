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

#include "qtsTimeControl.h"

#include <puTools/miStringFunctions.h>

#include <iostream>
#include <sstream>

using namespace std;

TimeControl::TimeControl(QWidget* parent)
  : QFrame( parent), totalrange(300), fcastrange(300),obsrange(0), lockHours(false)
{
  QGridLayout* timeLayout = new QGridLayout(this);
  setFrameStyle(QFrame::Panel | QFrame::Sunken);

  QLabel  *lab1 = new QLabel(tr("Start:"),this);
  QLabel  *lab2 = new QLabel(tr("Hours:"),this);

  startSlider   = new QSlider( Qt::Horizontal,this);
  stopSlider    = new QSlider( Qt::Horizontal,this);

  startSlider->setTickPosition(QSlider::TicksBelow);
  startSlider->setTickInterval(24);
  startSlider->setRange(0,300);
  startSlider->setValue(0);
  stopSlider->setTickPosition(QSlider::TicksBelow);
  stopSlider->setTickInterval(24);
  stopSlider->setRange(0,300);
  stopSlider->setValue(300);

  startLabel    = new QLabel("0",this);
  stopLabel     = new QLabel("300",this);


  connect( startSlider, SIGNAL( valueChanged(int)),SLOT(startchanged(int)));
  connect( stopSlider, SIGNAL(  valueChanged(int)),SLOT(stopchanged(int)));


  timeLayout->addWidget(lab1,0,0);
  timeLayout->addWidget(lab2,1,0);

  timeLayout->addWidget ( startSlider,0,1,1,3 );
  timeLayout->addWidget ( stopSlider ,1,1,1,3 );

  timeLayout->addWidget(startLabel,0,4);
  timeLayout->addWidget(stopLabel, 1,4);
}


std::string TimeControl::getTimecontrolLog()
{
  ostringstream ost;
  ost << totalrange << "," << fcastrange << "," << startSlider->value() << "," << stopSlider->value();
  return ost.str();
}


void TimeControl::setTimecontrolFromlLog(  std::string logString)
{
  vector<std::string> logEntries=miutil::split(logString, ",");
  if(logEntries.size() < 4) return;

  int t=atoi(logEntries[0].c_str());
  int f=atoi(logEntries[1].c_str());
  int s=atoi(logEntries[2].c_str());
  int l=atoi(logEntries[3].c_str());

  setTimeRange(t,f);
  setLengthSlider(l);
  setStartSlider(s);
}

void TimeControl::setLengthSlider(int v)
{
  cerr << "set stop slider with value " << v << endl;
  stopSlider->setValue(v);
}

void TimeControl::setStartSlider(int v)
{
  cerr << "set start slider with value " << v << endl;

  if(v < startSlider->minimum() || v > startSlider->maximum()) {
    cerr << v << " is not between " << startSlider->minimum() << " and " << startSlider->maximum() << endl;

    return;

  }
  startSlider->setValue(v);
}

int TimeControl::getLengthValue()
{
  return stopSlider->value();
}

int TimeControl::getStartValue()
{
  return startSlider->value();
}

void TimeControl::startchanged(int v)
{
  startLabel->setNum(v-obsrange);
  minmaxSlot();
}

void TimeControl::stopchanged(int v)
{
  stopLabel->setNum(v);
  minmaxSlot();
}

void TimeControl::setTimeRange(int total, int fcast)
{
  if(!total && !fcast)
    return;
  if( total == totalrange && fcast == fcastrange )
      return;
  totalrange=total;
  fcastrange=fcast;

  int newobs=totalrange - fcastrange;
  int activeposition= startSlider->value()+newobs - obsrange;
  obsrange=newobs;

  int oldfcastrange = stopSlider->value();

  if(oldfcastrange > totalrange || lockHours) oldfcastrange = fcastrange;

  startSlider->setRange(0,totalrange);
  stopSlider->setRange(0,totalrange);
  stopSlider->setRange(0,totalrange);
  stopSlider->setValue(oldfcastrange);

  startSlider->setValue(activeposition);
}


void TimeControl::minmaxSlot()
{
  int istart= startSlider->value();
  int istop=  stopSlider->value();

  istop+=istart;

  emit   minmaxProg(istart,istop);
}
