#ifndef CLEARLINEEDIT_H
#define CLEARLINEEDIT_H

#include <QLineEdit>
#include <QToolButton>

class ClearLineEdit : public QLineEdit
{
  Q_OBJECT
private:
  QToolButton *clearButton;
  QToolButton *searchButton;

public:
    ClearLineEdit(QWidget *parent = 0);

protected:
    void resizeEvent(QResizeEvent *);

private slots:
  void updateButtons(const QString &text);
	void prepareSearch();
	
signals:
  void search(QString);

};

#endif




