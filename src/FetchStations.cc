#include "FetchStations.h"
#include "tsSetup.h"
#include <puTools/miStringFunctions.h>
#include <iostream>
#include <sstream>
#include <QDomDocument>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>



FetchStations::FetchStations(QString baseurl, QString lang) : base_url(baseurl)
{
  manager = new QNetworkAccessManager(this);
  connect(manager, SIGNAL(finished(QNetworkReply*)),
      this, SLOT(replyFinished(QNetworkReply*)));

  feature_class_name="feature_class";

  if(lang=="no" || lang=="nn")
    feature_class_name = QString("feature_class_%1").arg(lang);

}



void FetchStations::getData(QString name)
{
  search_name = name;
  QString url;
  tsSetup s;

  if (s.fimex.xmlSyntax != "metno") {
    url = base_url;
  }
  else {
    url = base_url+name+"*";
  }
  manager->get(QNetworkRequest(QUrl(url)));

}


void FetchStations::replyFinished(QNetworkReply* reply)
{
  if(reply->error())
    return;

  QByteArray data = reply->readAll();
  QDomDocument document;
  QString errorMsg;
  int errorLine, errorColumn;
  std::vector<std::string> result;
  tsSetup s;

  if(document.setContent ( data, true, &errorMsg, &errorLine, &errorColumn) ) {

    QDomNodeList nodes;
    if (s.fimex.xmlSyntax != "metno") {
      nodes = document.elementsByTagName("StationList");
    }
    else {
      nodes = document.elementsByTagName("doc");
    }
    for(int i = 0; i < nodes.count(); i++)
    {
      QDomNode element = nodes.at(i);
      if(element.isElement())
      {
        std::string name="";
        float latitude=-99999,longitude=-99999;


        QDomNode entry = element.toElement().firstChild();
        std::ostringstream feature;

        while(!entry.isNull()) {
          /* The SMHI(mora) format 
          <StationPlace lat="55.3376" lon="13.3563" hgt="5.0" barhgt="7.0">
            <Wmo>
              <Name>Smygehuk</Name>
              <Blk>2</Blk><Num>638</Num>
              <Typ>0</Typ>
              <Tdesc>surface</Tdesc>
              <Vfrom>1951-01-01T00:00:00.000Z</Vfrom>
              <Vto>1988-06-30T23:59:59.000Z</Vto>
              <Pfrom>1951-01-01T00:00:00.000Z</Pfrom>
              <Pto>1988-06-30T23:59:59.000Z</Pto>
            </Wmo>
            <Clim>
              <Name>Smygehuk</Name>
              <Num>53200</Num>
              <Vfrom>1930-07-02T06:00:00.000Z</Vfrom>
              <Vto>1988-07-01T06:00:00.000Z</Vto>
              <Pfrom>1930-07-02T06:00:00.000Z</Pfrom>
              <Pto>1988-07-01T06:00:00.000Z</Pto>
              <Landskap>Skåne</Landskap>
              <Lan>Skåne län</Lan>
            </Clim>
            <Ikv>
              <Name>Smygehuk</Name>
              <Num>1035</Num>
              <Param>4</Param>
              <Pdesc>Lufttemperatur</Pdesc>
              <Vfrom>1951-01-01T00:00:00.000Z</Vfrom>
              <Vto>1988-06-30T23:59:59.000Z</Vto>
              <Pfrom>1951-01-01T00:00:00.000Z</Pfrom>
              <Pto>1988-06-30T23:59:59.000Z</Pto>
            </Ikv>
          </StationPlace>
          */
          if (s.fimex.xmlSyntax != "metno") {
            if (QString("StationPlace") == entry.toElement().tagName()) {
              // SMHI format
              longitude = entry.toElement().attributeNode("lon").value().toFloat();
              latitude = entry.toElement().attributeNode("lat").value().toFloat();
              name.clear();
              // First level of child nodes, Wmo, Clim, Ikv
              QDomNodeList child_nodes = entry.childNodes();
              bool name_found = false;
              for(int i = 0; i < child_nodes.count(); i++)
              {
                if (name_found) break;
                QDomNode child_element = child_nodes.at(i);
                if(child_element.isElement())
                {
                  // Next level of child nodes, Name, Num osv
                  // Dont use Ikv station type, just skip it
                  if (child_element.toElement().tagName().toStdString() == "Ikv")
                    continue;
                  QDomNodeList element_nodes = child_element.childNodes();
                  for(int i = 0; i < element_nodes.count(); i++)
                  {
                    QDomNode the_element = element_nodes.at(i);
                    if(the_element.isElement())
                    {
                      QString tag_name = the_element.toElement().tagName();
                      if (tag_name == QString("Name")) {
                        QString tmp_name = the_element.toElement().text();
                        // We dont need more info yet.
                        QRegExp search_name_reg(search_name);
                        search_name_reg.setPatternSyntax(QRegExp::Wildcard);
                        if (tmp_name.contains(search_name_reg)) {
                          name_found = true;
                          // "." not allowed in names, replace it
                          if (tmp_name.count(QString("."))) {
                            tmp_name.replace(QString("."),QString(" "));
                          }
                          name = tmp_name.toStdString();
                          boost::algorithm::trim(name);
                          std::string uppername = miutil::to_upper_latin1(name);
                          name = uppername;
                          break;
                        }
                        else {
                          name_found = false;
                          name.clear();
                          break;
                        }
                      }
                    }
                  }
                }
              }
            } // End StationPlace
          } // End mora_format
          else {
          // Met.no format
            QString value  = entry.toElement().text();
            QString key    = entry.toElement().attributeNode("name").value();
            if(key=="name")
              name = value.toStdString();
            else if(key=="lat")
              latitude=value.toFloat();
            else if(key=="long")
              longitude = value.toFloat();
            else if(key==feature_class_name) {
              feature << " ( " <<  value.toStdString() << " )";
            }
            std::cerr << key.toStdString() << " : " << value.toStdString() << std::endl;
          } // end met.no format
          // Add to result
          if(longitude > -99999 && latitude > -99999 && !name.empty()) {
            std::ostringstream ost;
            ost  << name << feature.str() << "|" << latitude << ":" << longitude;
            result.push_back(ost.str());
          }
          entry = entry.nextSibling();
        }       
      }
    }
  }
  emit searchResult(result);
}

