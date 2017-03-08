#ifndef _FETCHSTATIONS_H
#define _FETCHSTATIONS_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QWidget>
#include <vector>

class FetchStations : public QWidget {
  Q_OBJECT
private:
  QNetworkAccessManager *manager;
  QString    base_url;
  QString    feature_class_name;
  QString    search_name;

public:
  FetchStations(QString baseurl, QString lang);

  void getData(QString name);

public slots:
  void replyFinished(QNetworkReply* reply);

signals:
  void searchResult(std::vector<std::string>);


};


#endif
