#include "ClearLineEdit.h"

#include <QStyle>

#include "clear.xpm"
#include "search_small.xpm"

ClearLineEdit::ClearLineEdit(QWidget *parent)
: QLineEdit(parent)
{
  QPixmap clearpixmap(clear_xpm);
  QPixmap searchpixmap(search_small_xpm);

  clearButton = new QToolButton(this);
  clearButton->setIcon(QIcon(clearpixmap));
  clearButton->setIconSize(clearpixmap.size());
  clearButton->setCursor(Qt::ArrowCursor);
  clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  clearButton->setToolTip(  tr("clear search field") );
  clearButton->hide();
  connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));

  searchButton = new QToolButton(this);
  searchButton->setIcon(QIcon(searchpixmap));
  searchButton->setIconSize(searchpixmap.size());
  searchButton->setCursor(Qt::ArrowCursor);
  searchButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  searchButton->setToolTip(  tr("search positions on the web") );
  searchButton->hide();

  connect(searchButton, SIGNAL(clicked()), this, SLOT( prepareSearch()));

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  setStyleSheet(QString("QClearLineEdit { padding-right: %1px; } ").arg(clearButton->sizeHint().width() +
      searchButton->sizeHint().width() + frameWidth + 1));
  QSize msz = minimumSizeHint();
  setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2 + 2),
      qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));


  connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateButtons(const QString&)));

}

void ClearLineEdit::resizeEvent(QResizeEvent *)
{
  QSize csz = clearButton->sizeHint();
  QSize hsz = searchButton->sizeHint();
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  clearButton->move(rect().right() - frameWidth - csz.width(),
      (rect().bottom() + 1 - csz.height())/2);

  searchButton->move(rect().right() - frameWidth - hsz.width()  - csz.width(),
        (rect().bottom() + 1 - hsz.height())/2);

}

void ClearLineEdit::updateButtons(const QString& text)
{
  clearButton->setVisible(!text.isEmpty());
  searchButton->setVisible(!text.isEmpty());
}


void ClearLineEdit::prepareSearch()
{
 emit search(text());

}



