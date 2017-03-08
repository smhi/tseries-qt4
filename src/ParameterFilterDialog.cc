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

#include "ParameterFilterDialog.h"
#include <QStringList>
#include <QLayout>
#include <QLabel>
#include <iostream>

using namespace std;


ParameterFilterDialog::ParameterFilterDialog(const std::set<std::string>& originalFilter,
    const std::vector<std::string>& allParameters, QWidget* parent)
  : QDialog(parent)
  , filter(originalFilter)
  , parameters(allParameters)
{

  QVBoxLayout*  mainLayout = new QVBoxLayout(this);
  QHBoxLayout*  buttonLayout  = new QHBoxLayout();

  QLabel * title = new QLabel(tr("Filter marked parameters"));

  filterList  = new QListWidget(this);


  doneButton = new QPushButton(tr("done"),this);
  cancelButton = new QPushButton(tr("cancel"),this);

  connect(doneButton,SIGNAL(clicked()),this,SLOT(accept()));
  connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));

  mainLayout->addWidget(title);
  mainLayout->addWidget(filterList);
  buttonLayout->addWidget(cancelButton);
  buttonLayout->addWidget(doneButton);
  mainLayout->addLayout(buttonLayout);


  set<string> doublettes;

  for ( unsigned int i=0; i<parameters.size();i++) {

      if(doublettes.count(parameters[i]))
        continue;

      doublettes.insert(parameters[i]);

      bool isFiltered = filter.count(parameters[i]);
      QListWidgetItem* item = new QListWidgetItem(QString(parameters[i].c_str()));
      item->setCheckState( isFiltered ?   Qt::Checked : Qt::Unchecked);
      filterList->addItem(item);
  }

}


std::set<std::string>  ParameterFilterDialog::result()
{

  set<string> newFilter;
  int row=0;
  //for(int row=0;row<filterList->count(); row++) {
  while( QListWidgetItem * item = filterList->item(row++) ) {
    if(item->checkState()== Qt::Checked) {
      string filteredToken = item->text().toStdString();
      newFilter.insert(filteredToken);
    }
  }
  return newFilter;
}





