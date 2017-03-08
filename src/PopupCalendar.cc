/*
 * PopupCalendar.cc
 *
 *  Created on: Jan 4, 2012
 *      Author: juergens
 */

#include "PopupCalendar.h"
#include <QLayout>
#include <QLabel>

using namespace std;

PopupCalendar::PopupCalendar(  QWidget* parent, QDate initial_date )
  : QDialog(parent)
{

  QVBoxLayout*  mainLayout = new QVBoxLayout(this);

  QLabel * title = new QLabel(tr("change observation start date"));

  calendar = new QCalendarWidget(this);
  calendar->setSelectedDate(initial_date);
  connect(calendar,SIGNAL(selectionChanged()),this,SLOT(accept()));

  cancelButton = new QPushButton(tr("cancel"),this);

  connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));

  mainLayout->addWidget(title);
  mainLayout->addWidget(calendar);
  mainLayout->addWidget(cancelButton);

}



