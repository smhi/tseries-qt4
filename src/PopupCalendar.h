/*
 * PopupCalendar.h
 *
 *  Created on: Jan 4, 2012
 *      Author: juergens
 */

#ifndef POPUPCALENDAR_H_
#define POPUPCALENDAR_H_

#include <QDialog>
#include <QPushButton>
#include <QCalendarWidget>

class PopupCalendar : public QDialog {
  Q_OBJECT
private:
  QCalendarWidget * calendar;
  QPushButton * cancelButton;
public:
  PopupCalendar(QWidget* parent, QDate initial_value);
  QDate result() { return calendar->selectedDate(); }
};


#endif /* POPUPCALENDAR_H_ */
