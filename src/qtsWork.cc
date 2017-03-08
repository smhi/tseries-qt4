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

#include "qtsWork.h"
#include "tsConfigure.h"

#include "WdbCacheThread.h"

#include <tsData/FimexStream.h>
#include <puTools/miStringFunctions.h>
#include <coserver/QLetterCommands.h>

#include <QApplication>
#include <QSplitter>

#include <fstream>
#include <set>
#include <string>

using namespace std;
using namespace miutil;

namespace {
bool qStr2miStr(const QString& i, std::string& o)
{
  if (i.isEmpty()) {
    o = "";
    return false;
  } else {
    o = i.toStdString();
    return true;
  }
}

//! insert first lat, then lon
QStringList& operator<<(QStringList& s, const miCoordinates& co)
{
  return s << QString::number(co.dLat()) << QString::number(co.dLon());
}
} // namespace

const QString C_DATASET = "dataset";

const QString DATASET_TSERIES= "T-series ";
const QString DATASET_FIMEX=   "T-series Fimex";
const QString TARGETS_TSERIES= "TARGETS_TSERIES";
const QString IMG_STD_TSERIES= "IMG_STD_TSERIES";
const QString IMG_FIN_TSERIES= "IMG_FIN_TSERIES";
const QString IMG_NEW_TSERIES= "IMG_NEW_TSERIES";
const QString IMG_ACTIVE_TSERIES= "IMG_ACTIVE_TSERIES";
const QString IMG_ICON_TSERIES= "IMG_ICON_TSERIES";
const QString NOMODEL_TSERIES= "NONE";
const QString TS_MINE        = " -- ";

qtsWork::qtsWork(QWidget* parent, QString language)
: QWidget(parent) , activeRefresh(true)
{
  selectionType   = SELECT_BY_STATION;
  filterOn        = false;
  latlonInDecimal = false;

  reading_data=false;

  QSplitter   * splitter = new QSplitter(this);
  QHBoxLayout * hlayout  = new QHBoxLayout(this);
  oldModel = NOMODEL_TSERIES;

  sidebar = new qtsSidebar(language);
  show    = new qtsShow(&request,&data,&session);


  connect (show,SIGNAL(refreshFinished()),this,SLOT(refreshFinished()));

  connect (show,SIGNAL(dataread_started()),this,SLOT(dataread_started()));
  connect (show,SIGNAL(dataread_ended()),this,SLOT(dataread_ended()));


  //sidebar->setMinimumWidth(170);
  //sidebar->setMaximumWidth(255);


  connect( sidebar, SIGNAL( minmaxProg(int,int)),
      this,    SLOT(setProgintervall(int,int)));

  connect(show,SIGNAL(newTimeRange(int, int)), sidebar,SLOT(newTimeRange(int,int)));

  connect( sidebar, SIGNAL(  observationToggled(bool)), this, SLOT(observationToggled(bool)));
  connect( sidebar, SIGNAL(changeFimexPoslist()), this, SIGNAL(fimexPoslistChanged()));


  splitter->addWidget(show);

  splitter->addWidget(sidebar);

  splitter->setStretchFactor (0,3);
  splitter->setStretchFactor (1,1);


  hlayout->addWidget(splitter);



  connect(sidebar,SIGNAL(changestyle(const QString&)),
      this,SLOT(changeStyle(const QString&)));
  connect(sidebar,SIGNAL(changemodel(const QString&)),
      this,SLOT(changeModel(const QString&)));
  connect(sidebar,SIGNAL(changerun(const QString&)),
      this,SLOT(changeRun(const QString&)));
  connect(sidebar,SIGNAL(changestation(const QString&)),
      this,SLOT(changeStation(const QString&)));
  connect(sidebar,SIGNAL(filterToggled(bool)),
      this,SLOT(filterToggled(bool)));




  connect(sidebar,SIGNAL(changeFimexModel(const QString&)), this,SLOT(changeFimexModel(const QString& )));
  connect(sidebar,SIGNAL(changeFimexStyle(const QString&)), this,SLOT(changeFimexStyle(const QString& )));
  connect(sidebar,SIGNAL(changeFimexRun(const QString&)),   this,SLOT(changeFimexRun(  const QString& )));
  connect(sidebar,SIGNAL(changeFimexCoordinates(float, float,QString)),this,SLOT(changeFimexCoordinates(float,float,QString)));
  connect(sidebar,SIGNAL( newFimexPoslist() ),   this,SLOT( newFimexPoslist() ));

  Initialise(); // the none gui stuff...



  // WDB signals

  connect(sidebar,SIGNAL(changeWdbModel(const QString&)),  this,SLOT(changeWdbModel(const QString&)));


  connect(sidebar,SIGNAL(changetype(const tsRequest::Streamtype)), this,SLOT(changeType(const tsRequest::Streamtype)));
  connect(sidebar,SIGNAL(changeWdbStyle(const QString&)), this,SLOT(changeWdbStyle(const QString&)));
  connect(sidebar,SIGNAL(changeWdbRun(const QString&)),   this,SLOT(changeWdbRun(const QString&)));
  connect(sidebar,SIGNAL(changeCoordinates(float, float,QString)),this,SLOT(changeCoordinates(float,float,QString)));
  connect(sidebar,SIGNAL(requestWdbCacheQuery()),this,SLOT(requestWdbCacheQuery()));





  has_wdb_stream = data.has_wdb_stream();
  sidebar->enableWdb(has_wdb_stream);
  makeWdbModels();

}


void qtsWork::Initialise()
{
  session.readSessions(setup.files.defs,setup.path.styles);

  maxWDBreadTime = setup.wdb.readtime;

  data.setVerbose(false);

  for (unsigned int i=0; i<setup.streams.size(); i++) {
    data.addDataset(setup.streams[i].collectionName);

    for (unsigned int j=0; j<setup.streams[i].data.size(); j++) {
      data.addStream(setup.streams[i].data[j].name,      // streamname
          setup.streams[i].data[j].descript,  // description
          setup.streams[i].data[j].type,      // streamtype
          i, j,                               // dataset and
          // number in dataset
          setup.streams[i].data[j].contents, // models/runs
          setup.streams[i].data[j].config);  // fimexconfigfile
    }
  }

  //data.openStreams();
  //getStationList();

  makeStyleList();

  int t,f;
  show->getTimeRange(t,f);
  sidebar->newTimeRange(t,f);

  filter = createFilter();
}

miQMessage qtsWork::getStationList()
{
  if(selectionType == SELECT_BY_FIMEX)
    return getFimexStationList();

  miQMessage m(qmstrings::positions);
  m.addCommon(C_DATASET, DATASET_TSERIES)
      .addCommon("image", IMG_STD_TSERIES)
      .addCommon("icon", IMG_ICON_TSERIES)
      .addCommon("annotation", DATASET_TSERIES + lastList());
  m.setData((QStringList() << "name" << "lat" << "lon"),
            myStations);
  return m;
}

miQMessage qtsWork::getFimexStationList()
{
  miQMessage m(qmstrings::positions);
  m.addCommon(C_DATASET, DATASET_FIMEX)
      .addCommon("image", IMG_STD_TSERIES)
      .addCommon("icon", IMG_ICON_TSERIES)
      .addCommon("annotation", DATASET_FIMEX + lastList());
  m.setData((QStringList() << "name" << "lat" << "lon"),
            sidebar->getFimexPositions());
  return m;
}

QString qtsWork::lastList() const
{
  if (filterOn)
    return TS_MINE + model();
  else
    return model();
}

set<std::string> qtsWork::fullPosList()
{
  set<std::string> slist;
  ExtStation **es = new ExtStation*;

  int posc = 0;
  while (data.getPosition(-1,posc,es))
    slist.insert( (*es)->station.Name() );
  delete es;

  return slist;
}

void qtsWork::makeStationList(bool forced)
{
  const QString rmodel = model();
  if (rmodel.isEmpty())
    restoreModelFromLog();


  if(!forced)
    if (oldModel == rmodel)
      return;

  oldModel = rmodel;

  QStringList slist;
  myStations.clear();

  myList = data.getPositions(request.model());
  map<std::string, miCoordinates>::iterator itr = myList.begin();
  for (;itr!=myList.end();itr++) {
    const std::string& pos = itr->first;

    if(filterOn)
      if(!filter.empty())
        if(!filter.count(pos))
          continue;

    QString qpos = QString::fromStdString(pos);
    slist << qpos;
    myStations << (QStringList() << qpos << itr->second);
  }

  sidebar->fillStations(slist);

  emit(refreshStations());
}


bool qtsWork::makeStyleList()
{
  vector<std::string> stationStyles, wdbStyles, fimexStyles;

  session.getStyleTypes( stationStyles, SessionManager::ADD_TO_STATION_TAB);
  session.getStyleTypes( wdbStyles,     SessionManager::ADD_TO_WDB_TAB    );
  session.getStyleTypes( fimexStyles,   SessionManager::ADD_TO_FIMEX_TAB  );


  QString cstyle = sidebar->fillList( stationStyles, StationTab::CMSTYLE      );
  QString wstyle = sidebar->fillList( wdbStyles,     StationTab::CMWDBSTYLE   );
  QString fstyle = sidebar->fillList( fimexStyles,   StationTab::CMFIMEXSTYLE );

  makeFimexModels(fstyle);

  std::string st;
  qStr2miStr(cstyle,st);
  bool changed = request.setStyle(st);


  qStr2miStr(wstyle,st);
  request.setWdbStyle(st);

  qStr2miStr(fstyle,st);
  request.setFimexStyle(st);


  return (  makeModelList(st) || changed );

}

bool qtsWork::makeModelList(const std::string& st)
{

  vector<std::string>    modname;
  bool changed = false;

  int choice =  session.getModels(st, modelMap, modname);

  if(choice < 0 )
    if(modname.size() > 1 )
      modname.erase(modname.begin()+1,modname.end());

  QString qtmp = sidebar->fillList(modname,StationTab::CMMODEL);

  std::string tmp;
  qStr2miStr(qtmp,tmp);

  tmp = modelMap[tmp];

  QApplication::setOverrideCursor( Qt::WaitCursor );
  data.openStreams(tmp);
  QApplication::restoreOverrideCursor();

  changed = request.setModel(tmp);
  return (makeRunList(tmp) || changed);
}

bool qtsWork::makeRunList(const std::string& st)
{
  vector <std::string> runList;
  runList = data.findRuns(st);

  QString qtmp = sidebar->fillList(runList,StationTab::CMRUN);
  std::string tmp;
  qStr2miStr(qtmp,tmp);
  return request.setRun(atoi(tmp.c_str()));
}

bool qtsWork::makeRunList(const std::string& st,const std::string& ru)
{
  vector <std::string> runList;
  runList = data.findRuns(st);

  sidebar->fillList(runList,StationTab::CMRUN);
  if(runList.size()) {
    for(unsigned int i=0;i<runList.size();i++)
      if(ru == runList[i] ) {
        sidebar->set(ru,StationTab::CMRUN);
        return request.setRun(atoi(ru.c_str()));
      }


    sidebar->set(runList[0],StationTab::CMRUN);
    return request.setRun(atoi(runList[0].c_str()));
  }

  return false;
}


///////////////////////// SLOTS

void qtsWork::changeStyle(const QString& qstr)
{
  std::string st;
  if(qStr2miStr(qstr,st))
    changeStyle(st);

  makeStationList();
}

void qtsWork::changeModel(const QString& qstr)
{
  std::string st;
  if(qStr2miStr(qstr,st))
    changeModel(st);
  makeStationList();
}

void qtsWork::changePositions(float lon, float lat)
{
  if (selectionType == SELECT_BY_STATION)
    return;

  sidebar->setCoordinates(lon,lat);
}

void qtsWork::changeStation(const QString& qstr)
{
  if(selectionType==SELECT_BY_WDB)
    return;
  std::string st;
  if (qStr2miStr(qstr, st))
    changeStation(st);
}

void qtsWork::changeRun(const QString& qstr)
{
  std::string st;
  if(qStr2miStr(qstr,st))
    changeRun(st);
}


void qtsWork::changeStyle(const std::string& st)
{
  bool changed = request.setStyle(st);

  if(makeModelList(st) || changed)
    refresh(true);
}

void qtsWork::changeModel(const std::string& st)
{
  std::map<std::string,Model>::iterator itr = modelMap.begin();

  std::string tmp = modelMap[st];
  QApplication::setOverrideCursor( Qt::WaitCursor );
  data.openStreams(tmp);
  QApplication::restoreOverrideCursor();
  /*bool changed =*/ request.setModel(tmp);

  //if( makeRunList(tmp) || changed) {
  makeRunList(tmp);
  refresh(true);
  //}
}

void qtsWork::changeStation(const std::string& st)
{
  if (selectionType==SELECT_BY_STATION) {

    miPosition p=data.getPositionInfo(st);
    miCoordinates cor=p.Coordinates();
    checkObsPosition(cor);

    if(!request.setPos(st)) {
      std::string ST = miutil::to_upper_latin1(st);

      if(!request.setPos(ST))
        return;
    }
    refresh(true);
    // Set current station
    sidebar->searchStation(QString::fromLatin1(st.c_str()));

  } else if (selectionType==SELECT_BY_FIMEX) {
    sidebar->changeFimexPosition(QString::fromLatin1(st.c_str()));
  }
}

void qtsWork::checkPosition(std::string name)
{
  if (name.empty())
    return;
  miPosition p=data.getPositionInfo(name);
  if (p.Name()!=name)
    return;

  miCoordinates cor=p.Coordinates();
  checkObsPosition(cor);
  ostringstream ost;

  if(latlonInDecimal) {
    ost <<  "<b>Lat:</b> " << cor.dLat()<< " <b>Lon:</b> " << cor.dLon();
  } else {
    ost <<  "<b>Lat:</b> " << cor.sLat()<< " <b>Lon:</b> " << cor.sLon();
  }
  //  ost << " <b>Hoh:</b> " << p.height();

  sidebar->setStationInfo(ost.str().c_str());
}

void  qtsWork::setKlimaBlackList(std::set<std::string>& bl)
{
  data.setKlimaBlacklist(bl);
  refresh(true);
}

void qtsWork::checkObsPosition(miCoordinates cor)
{
  pets::KlimaStation s = data.getNearestKlimaStation(cor);

  if(!s.stationid) {
    // try mora station
    pets::MoraStation s;
    s = data.getNearestMoraStation(cor);
    if(s.name.empty()) {
      sidebar->setObsInfo("");
      return;
    }
    sidebar->setObsInfo(s.description().c_str());
    return; 
  }
  sidebar->setObsInfo(s.description().c_str());
}




void qtsWork::changeRun(const std::string& st)
{
  if(request.setRun(atoi(st.c_str())))
    refresh(true);
}

void qtsWork::refresh(bool readData)
{
 // QApplication::setOverrideCursor( Qt::WaitCursor );
  if (activeRefresh){
    show->refresh(readData);
  }


  if(request.type() == tsRequest::HDFSTREAM) {

    // check if any streams recently opened

    if (data.has_opened_streams() && readData){
      data.makeStationList();
      makeStationList();
    }

    checkPosition(request.posname());
  }
//  QApplication::restoreOverrideCursor();
}


void qtsWork::restoreLog()
{
  tsConfigure c;
  std::string mo,po,st;
  int ru;

  bool validR;
  if(!c.get("VALIDREQUEST",validR))
    return;
  if(!validR)
    return;



  c.get("REQUESTMODEL",mo);
  c.get("REQUESTPOS",po);
  c.get("REQUESTRUN",ru);
  c.get("REQUESTSTYLE",st);


  activeRefresh = false;


  sidebar->set(st,StationTab::CMSTYLE);
  changeStyle(st);

  map<std::string,std::string>::const_iterator itr = modelMap.begin();
  for(;itr!=modelMap.end();itr++)
    if(itr->second == mo) {
      sidebar->set(itr->first,StationTab::CMMODEL);
      break;
    }

  request.setModel(mo);
  QApplication::setOverrideCursor( Qt::WaitCursor );
  data.openStreams(mo);
  QApplication::restoreOverrideCursor();

  makeRunList(mo, miutil::from_number(ru));

  changeStation(po);

  activeRefresh = true;
  refresh(true);

  int tabindex;
  c.get("TABINDEX",tabindex);
  sidebar->setTab(tabindex);


  float lat,lon;
  std::string run;
  std::string posname;
  std::string observationfilter;

  c.get("OBSERVATIONFILTER",observationfilter);
  data.setObservationBlacklistFromString(observationfilter);

  c.get("WDBMODEL",mo);
  c.get("WDBSTYLE",st);
  c.get("WDBLAT",lat);
  c.get("WDBLON",lon);
  c.get("WDBRUN",run);
  c.get("WDBPOSNAME",posname);

  std::string timecontrol;
  c.get("TIMECONTROL",timecontrol);

  sidebar->setTimeControlFromLog(timecontrol);



  bool showobs=false;
  c.get("SHOWOBSERVATIONS",showobs);
  sidebar->setObservationsEnabled(showobs);
  show->setShowObservations(showobs);


  request.restoreWdbFromLog(mo,st,lat,lon,miTime(run),posname);
  sidebar->restoreWdbFromLog(mo,st,lat,lon,run,posname);

  std::string fimexmodel,fimexstyle,fimexexpand,fimexfilter;

  c.get("FIMEXMODEL",fimexmodel);
  c.get("FIMEXSTYLE",fimexstyle);
  c.get("FIMEXEXPANDEDDIRS",fimexexpand);
  c.get("FIMEXFILTER",fimexfilter);

  pets::FimexStream::setParameterFilterFromString(fimexfilter);

  if(!fimexmodel.empty())
     request.setFimexModel(fimexmodel);
  if(!fimexstyle.empty())
     request.setFimexStyle(fimexstyle);

  QString qstyle(fimexstyle.c_str());
  changeFimexStyle(qstyle);

  sidebar->restoreFimexFromLog(fimexmodel,fimexstyle,fimexexpand);
}


void qtsWork::collectLog()
{
  tsConfigure c;

  c.set("TABINDEX",sidebar->getTab());

  c.set("REQUESTMODEL",request.model());
  c.set("REQUESTPOS",request.pos());
  c.set("REQUESTRUN",request.run());
  c.set("REQUESTSTYLE",request.style());
  c.set("VALIDREQUEST",true);

  request.setType(tsRequest::WDBSTREAM);
  c.set("WDBMODEL",request.getWdbModel());
  c.set("WDBSTYLE",request.getWdbStyle());
  c.set("WDBLAT",float(request.getWdbLat()) );
  c.set("WDBLON",float(request.getWdbLon()) );
  c.set("WDBRUN",request.getWdbRun().isoTime());
  c.set("WDBPOSNAME",request.getWdbStationName());

  c.set("SHOWOBSERVATIONS",sidebar->getObservationsEnabled());
  c.set("TIMECONTROL",sidebar->getTimecontrolLog());
  c.set("OBSERVATIONFILTER",data.getObservationBlacklistAsString());


  c.set("FIMEXMODEL",request.getFimexModel());
  c.set("FIMEXSTYLE",request.getFimexStyle());
  c.set("FIMEXEXPANDEDDIRS",sidebar->getFimexExpanded());

  c.set("FIMEXFILTER",pets::FimexStream::getParameterFilterAsString());


  sidebar->writeBookmarks();
}

void qtsWork::restoreModelFromLog()
{
  tsConfigure c;
  std::string mo;
  c.get("REQUESTMODEL",mo);
  request.setModel(mo);
}


miQMessage qtsWork::target()
{
  miCoordinates coord;
  if (selectionType == SELECT_BY_STATION) {
    const std::string po = request.posname();
    changeStation(po);
    coord = myList[po];
  } else if (selectionType == SELECT_BY_FIMEX) {
    coord = sidebar->fimexCoordinates();
    // cerr << "select by fimex : " << co  << endl;
  } else {
    coord = sidebar->coordinates();
  }

  miQMessage m(qmstrings::positions);
  m.addDataDesc(TARGETS_TSERIES+";name").addDataDesc("lat").addDataDesc("lon").addDataDesc("image");
  m.addDataValues(QStringList() << "." << coord << IMG_FIN_TSERIES);
  return m;
}

void qtsWork::updateStreams()
{
  vector<int> idx;

  if (data.check(idx)){

    int size, dset, nindset;

    std::string filename, descrip;

    for (unsigned int i=0; i<idx.size();i++){
      data.getStreamInfo(idx[i], filename, descrip, size, dset, nindset);

      cout << "Reopening stream: " << filename << " : " << descrip << endl;

      data.openStream(idx[i]);
    }

    if (idx.size())
      data.makeStationList();
  }

  if(data.updateFimexStreams(request.getFimexModel())) {
    vector<std::string> times = data.getFimexTimes(request.getFimexModel());
    sidebar->fillList(times,StationTab::CMFIMEXRUN);
  }
}

void qtsWork::filterToggled(bool f)
{
  filterOn = f;
  makeStationList(true);
}

void qtsWork::latlonInDecimalToggled(bool f)
{
  latlonInDecimal = f;
  checkPosition(request.posname());
}


set<std::string> qtsWork::createFilter(bool orig)
{
  tsSetup s;
  set<std::string> fl;

  std::string fname =  s.files.baseFilter;

  if(!orig) {
    fname = s.files.filter;
    ifstream tst(fname.c_str());

    if(!tst)
      fname = s.files.baseFilter;

    tst.close();
  }


  ifstream in(fname.c_str());

  if(!in) {
    cerr << "NO filter " << endl;
    return fl;
  }


  std::string token;
  while(in) {
    getline(in,token);

    miutil::trim(token);
    if(token.substr(0,1) == "#")
      continue;
    if(!token.empty())
      fl.insert(token);
  }
  in.close();
  return fl;
}


void qtsWork::newFilter(const set<std::string>& f)
{
  filter = f;
  if(filterOn)
    makeStationList(true);

  tsSetup s;

  ofstream of( s.files.filter.c_str());

  if(!of) {
    cerr << "could not write filter to file " <<  s.files.filter << endl;
    return;
  }

  of << "# automatic generated file. Do not edit!" << endl;

  set<std::string>::iterator itr=filter.begin();

  for(;itr!=filter.end();itr++)
    of << *itr << endl;

  of.close();
}


/// WDB --------------------------------------------
//
void qtsWork::makeWdbModels()
{
  if(!has_wdb_stream) return;

  QStringList newmodels;
  set<string> providers = data.getWdbProviders();
  set<string>::iterator itr = providers.begin();
  for(;itr!=providers.end();itr++)
    newmodels << itr->c_str();
  sidebar->setWdbModels(newmodels);
}


void qtsWork::changeWdbModel(const QString& newmodel)
{
  if (!has_wdb_stream)
    return;

  set<miTime> wdbtimes = data.getWdbReferenceTimes(newmodel.toStdString());
  QStringList newruns;

  set<miTime>::reverse_iterator itr = wdbtimes.rbegin();
  for(;itr!=wdbtimes.rend();itr++)
    newruns << itr->isoTime().c_str();
  sidebar->setWdbRuns(newruns);


  pets::WdbStream::BoundaryBox bb = data.getWdbGeometry();
  sidebar->setWdbGeometry(bb.minLon,bb.maxLon,bb.minLat,bb.maxLat);
  if(request.setWdbModel(newmodel.toStdString()))
    refresh(true);
}

void qtsWork::changeWdbRun(const QString& nrun)
{
  if(!has_wdb_stream)
    return;

  miTime newrun= miTime(nrun.toStdString());

  if(request.setWdbRun(newrun))
    refresh(true);
}


void qtsWork::changeWdbStyle(const QString& nsty)
{
  if(!has_wdb_stream)
    return;

  if(request.setWdbStyle(nsty.toStdString()))
    refresh(true);
}

void qtsWork::changeType(const tsRequest::Streamtype s)
{
  switch(s) {
  case tsRequest::WDBSTREAM:
    selectionType=SELECT_BY_WDB;
    break;
  case tsRequest::FIMEXSTREAM:
    selectionType=SELECT_BY_FIMEX;
    break;
  default:
    selectionType=SELECT_BY_STATION;
    break;
  }

  request.setType(s);
  refresh(true);

  emit selectionTypeChanged();
}

void qtsWork::changeCoordinates(float lon, float lat,QString name)
{
  if (!has_wdb_stream)
    return;

  request.setWdbStationName(name.toStdString());
  if(request.setWdbPos(lon,lat)) {
    miCoordinates cor(lon,lat);
    checkObsPosition(cor);
    refresh(true);
  }
  emit coordinatesChanged();
}

void qtsWork::refreshFinished()
{
  if(selectionType==SELECT_BY_WDB ) {
    bool enableCacheButton =  (request.getWdbReadTime() > maxWDBreadTime );
    sidebar->enableCacheButton(enableCacheButton,false,request.getWdbReadTime());
  }
}

void qtsWork::requestWdbCacheQuery()
{
  if(!has_wdb_stream)
    return;

  vector<string> parameters=data.getWdbParameterNames();
  string  model  = request.getWdbModel();
  string  run    = request.getWdbRun().isoTime();
  string  height = "NULL";
  tsSetup setup;
  string  host   = setup.wdb.host;
  string  usr    = setup.wdb.user;

  WdbCacheThread *cachethread = new WdbCacheThread(host,usr);
  cachethread->setParameters(model,run,height,parameters);
  connect(cachethread,SIGNAL(finished()),this,SLOT(cacheRequestDone()));
  cachethread->start();
}

void   qtsWork::cacheRequestDone()
{
  sidebar->enableBusyLabel(false);
}

////////////////////////////////////////////////////////////



/// FIMEX --------------------------------------------
//

void qtsWork::makeFimexModels(const QString& activeStyle)
{
//  if(!has_fimex_stream)
//    return;

  std::string st;
  qStr2miStr(activeStyle,st);

  vector<std::string>    modname;

  int choice =  session.getModels(st, fimexModelMap, modname,SessionManager::ADD_TO_FIMEX_TAB  );

  if (choice < 0) {
    if(modname.size() > 1)
      modname.erase(modname.begin()+1, modname.end());
  }

  sidebar->fillList(modname,StationTab::CMFIMEXMODEL);
}



void qtsWork::changeFimexModel(const QString& newmodel)
{
  request.setFimexModel(newmodel.toStdString());
  vector<std::string> times = data.getFimexTimes(newmodel.toStdString());
  sidebar->fillList(times,StationTab::CMFIMEXRUN);
}

void qtsWork::changeFimexRun(const QString& nrun)
{
  if(request.setFimexRun(nrun.toStdString())) {
    refresh(true);
  }
}


void qtsWork::changeFimexStyle(const QString& nsty)
{
  try {
    makeFimexModels(nsty);
  } catch (exception& e) {
    cerr << "Exception while trying to make new modellist" << e.what() << endl;
  }

  if(request.setFimexStyle(nsty.toStdString()))
    refresh(true);
}

void qtsWork::changeFimexCoordinates(float lon, float lat,QString name)
{
  if(request.setFimexLocation(lat,lon,name.toStdString())) {
    miCoordinates cor(lon,lat);
    checkObsPosition(cor);
    refresh(true);
  }
  emit fimexPositionChanged(name);
}

void qtsWork::newFimexPoslist()
{
  vector<string> newfimexposlist = sidebar->allFimexPositions();

  pets::FimexStream::setCommonPoslistFromStringlist(newfimexposlist);
}

void qtsWork::dataread_started()
{
  cerr << "DATAREAD STARTED" << endl;
  reading_data = true;
}

void qtsWork::dataread_ended()
{
  reading_data = false;
  sidebar->endProgress();
}

void qtsWork::updateProgress()
{
  if(reading_data) {
    int newprogress = pets::FimexStream::getProgress();
    if (newprogress > 0 && newprogress < 100) {
      string progressText= pets::FimexStream::getProgressMessage();

      sidebar->setProgress(newprogress,progressText);
    }
  }
}

