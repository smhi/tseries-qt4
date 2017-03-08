#ifndef PARAMETERFILTERDIALOG_H_
#define PARAMETERFILTERDIALOG_H_
/*
  Tseries - A Free Meteorological Timeseries Viewer

  $Id: qtsFilterManager.h 266 2010-09-17 12:56:23Z audunc $

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

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <vector>
#include <set>

class ParameterFilterDialog  : public QDialog
{
  Q_OBJECT
private:
  QListWidget * filterList;
  QPushButton * doneButton;
  QPushButton * cancelButton;
  std::set<std::string>  filter;
  std::vector<std::string> parameters;

public:
  ParameterFilterDialog(const std::set<std::string>& originalFilter, const std::vector<std::string>& allParameters, QWidget* parent);
  std::set<std::string>  result();
};

#endif /* PARAMETERFILTERDIALOG_H_ */
