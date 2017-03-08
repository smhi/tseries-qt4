/*
  Tseries - A Free Meteorological Timeseries Viewer

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
#include "qtsSidebar.h"

#include <coserver/ClientSelection.h>

#include <QPixmap>
#include <QHBoxLayout>
#include <QRegExp>
#include <QVBoxLayout>


#include "tsSetup.h"
#include "ts_find.xpm"
#include "ts_filter.xpm"
#include "view-refresh.xpm"
#include "list-add.xpm"
#include "synop.xpm"
#include "media-record.xpm"
#include "expand.xpm"
#include "collapse.xpm"

#include <iostream>


using namespace miutil;
using namespace std;

qtsSidebar::qtsSidebar(QString language)
: QWidget()
{
  fimexRexordToggled = false;
  tsSetup s;

  wdbDisabled   = s.disabled.wdb;
  fimexDisabled = s.disabled.fimex;
  hdfDisabled   = s.disabled.hdf;


  tabs       = new QTabWidget(this);

  // The Station Tabulator
  stationtab = new StationTab(this);

  if(hdfDisabled) {
    stationtab->hide();
  } else {

    connect(stationtab,SIGNAL(changestyle( const QString&)),  this, SIGNAL(changestyle(const QString& )));
    connect(stationtab,SIGNAL(changemodel( const QString&)),  this, SIGNAL(changemodel(const QString& )));
    connect(stationtab,SIGNAL(changerun(    const QString&)), this, SIGNAL(changerun(  const QString& )));
    connect(stationtab,SIGNAL(changestation(const QString&)), this, SIGNAL(changestation(  const QString& )));
    stationIdx = tabs->addTab(stationtab,tr("Stations"));
  }

  // The WDB Tabulator


  wdbtab  = new CoordinateTab(this);

  if(wdbDisabled) {
    wdbtab->hide();
  } else {

    connect(wdbtab,SIGNAL(changestyle( const QString&)), this, SIGNAL(changeWdbStyle(const QString& )));
    connect(wdbtab,SIGNAL(changemodel( const QString&)), this, SIGNAL(changeWdbModel(const QString& )));
    connect(wdbtab,SIGNAL(changerun(   const QString&)), this, SIGNAL(changeWdbRun(  const QString& )));
    connect(wdbtab,SIGNAL(changelevel( const QString&)), this, SIGNAL(changeWdbLevel(const QString& )));
    connect(wdbtab,SIGNAL(changeCoordinates(float, float,QString)), this, SIGNAL(changeCoordinates(float,float,QString)));
    QString dbname= s.wdb.host.c_str();
    dbname.truncate( dbname.indexOf(".") );
    wdbIdx     = tabs->addTab(wdbtab,dbname);
  }


  fimextab = new FimexTab(this, language);
  if(fimexDisabled) {
    fimextab->hide();
  } else {

    connect(fimextab,SIGNAL(changestyle( const QString&)), this, SIGNAL(changeFimexStyle(const QString& )));
    connect(fimextab,SIGNAL(changemodel( const QString&)), this, SIGNAL(changeFimexModel(const QString& )));
    connect(fimextab,SIGNAL(changerun(   const QString&)), this, SIGNAL(changeFimexRun(  const QString& )));
    connect(fimextab,SIGNAL(changeCoordinates(float, float,QString)), this, SIGNAL(changeFimexCoordinates(float,float,QString)));
    connect(fimextab,SIGNAL(changePoslist()), this, SIGNAL(changeFimexPoslist()));
    connect(fimextab,SIGNAL(newPoslist()), this, SIGNAL(newFimexPoslist()));
    fimexIdx     = tabs->addTab(fimextab,"fields");
  }

  progressHeader = new QLabel(this);
  progressHeader->hide();

  progress = new QProgressBar(this);
  progress->setRange(0,100);
  progress->setTextVisible(true);
  QFont progressfont=progress->font();
  progressfont.setPointSize(progressfont.pointSize()-3);
  progress->setFont(progressfont);

  connect(tabs,SIGNAL(currentChanged(int)), this,SLOT(tabChanged(int)));

  // Control the start and length

  timecontrol= new TimeControl(this);
  connect(timecontrol,SIGNAL(minmaxProg(int,int)),this, SIGNAL(minmaxProg(int,int)));



  obsInfo = new QLabel(this);
  obsInfo->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  obsInfo->hide();
  QFont obsfont=obsInfo->font();
  obsfont.setPointSize(obsfont.pointSize()-3);
  obsInfo->setFont(obsfont);


  // connectbuttons are hosted here... but are used and
  // connected in qtsMain!

  QPixmap synop_pix(synop_xpm);
  QPixmap find_pix(ts_find_xpm);
  QPixmap filter_pix(ts_filter_xpm);
  QPixmap refresh_pix(view_refresh_xpm);
  QPixmap add_pix(list_add_xpm);
  QPixmap record_pix(media_record_xpm);
  QPixmap expand_pix(expand_xpm);
  QPixmap collapse_pix(collapse_xpm);

  pluginB = new ClientSelection("TSeries", this);
  pluginB->client()->setServerCommand(QString::fromStdString(s.server.command));
  pluginB->setClientName(QString::fromStdString(s.server.name));

  observationB = new QPushButton(synop_pix,"",this);
  observationB->setMaximumWidth(synop_pix.width());
  observationB->setCheckable(true);
  observationB->setToolTip(  tr("enable/disable observations") );

  connect(observationB,SIGNAL(toggled(bool)), this, SIGNAL(observationToggled(bool)));

  targetB = new QPushButton(find_pix,"",this);
  targetB->setMaximumWidth(find_pix.width());
  targetB->setToolTip(tr("Show position (DIANA)") );



  filterB = new QPushButton(filter_pix,"",this);
  filterB->setMaximumWidth(filter_pix.width());
  filterB->setCheckable(true);
  filterB->setToolTip(  tr("Position filter") );

  connect(filterB,SIGNAL(toggled(bool)), this, SIGNAL(filterToggled(bool)));


  addWdbBookmarkButton =  new QPushButton(add_pix, "",this);
  connect(addWdbBookmarkButton,SIGNAL(clicked()),wdbtab, SLOT(addBookmarkFolder()));


  addFimexBookmarkButton =  new QPushButton(add_pix, "",this);
  connect(addFimexBookmarkButton,SIGNAL(clicked()),fimextab, SLOT(addBookmarkFolder()));


  recordFimexButton =  new QPushButton(record_pix, "",this);
  recordFimexButton->setCheckable(true);
  connect(recordFimexButton,SIGNAL(toggled(bool)),fimextab, SLOT(recordToggled(bool)));
  connect(recordFimexButton,SIGNAL(toggled(bool)),this, SLOT(recordToggled(bool)));

  expandFimexButton   = new QPushButton(expand_pix, "",this);
  expandFimexButton->setToolTip(  tr("expand all") );
  collapseFimexButton = new QPushButton(collapse_pix, "",this);
  collapseFimexButton->setToolTip(  tr("collapse all") );

  connect(expandFimexButton,SIGNAL(clicked()),fimextab, SLOT(expandAll()));
  connect(collapseFimexButton,SIGNAL(clicked()),fimextab, SLOT(collapseAll()));





  cacheQueryButton  =  new QPushButton(refresh_pix, "",this);
  connect(cacheQueryButton,SIGNAL(clicked()),    this, SLOT(chacheQueryActivated()));

  // LAYOUT ---------------------------

  QVBoxLayout * vlayout = new QVBoxLayout(this);


  vlayout->addWidget(tabs);

  vlayout->addWidget(timecontrol);
  vlayout->addWidget(progressHeader);
  vlayout->addWidget(progress);
  vlayout->addWidget(obsInfo);

  // Buttons -------------------


  connectStatus = new QLabel(this);
  connectStatus->setMinimumSize(50,32);
  cacheQueryButton->setMinimumSize(32,32);
  busyLabel     = new QMovie(s.wdb.busyMovie.c_str());

  QHBoxLayout * blayout = new QHBoxLayout();
  blayout->addWidget(addWdbBookmarkButton);
  blayout->addWidget(addFimexBookmarkButton);
  blayout->addWidget(recordFimexButton);

  blayout->addWidget(collapseFimexButton);
  blayout->addWidget(expandFimexButton);
  blayout->addWidget(cacheQueryButton);
  blayout->addWidget(connectStatus);
  blayout->addStretch(2);
  blayout->addWidget(observationB);
  blayout->addWidget(filterB);
  blayout->addWidget(targetB);
  QToolButton* clientbutton = new QToolButton(this);
  clientbutton->setDefaultAction(pluginB->getToolButtonAction());
  blayout->addWidget(clientbutton);
  vlayout->addLayout(blayout);

  addWdbBookmarkButton->hide();
  addFimexBookmarkButton->hide();
  recordFimexButton->hide();
  collapseFimexButton->hide();
  expandFimexButton->hide();


  progress->hide();

  cacheQueryButton->hide();
}

void qtsSidebar::recordToggled(bool record)
{
  fimexRexordToggled = record;
  if(record)
    setObsInfo("<b><font color=red>Recording Positions</font></b>");
  else
    setObsInfo("");
}


void qtsSidebar::setTab(int tabIdx)
{
  if(tabIdx < tabs->count() && tabIdx >= 0 ) {
    tabs->setCurrentIndex(tabIdx);
  }
}

void qtsSidebar::setCoordinates(float lon, float lat)
{
  if(actualIndex==wdbIdx) {
    wdbtab->setCoordinates(lon, lat);
  } else {
    fimextab->setCoordinates(lon, lat);
  }
}


void qtsSidebar::newTimeRange(int total,int fcast)
{
  timecontrol->setTimeRange(total,fcast);
}

void qtsSidebar::searchStation(const QString& s)
{
  stationtab->searchStation(s);
}

void qtsSidebar::currentStationChanged ( QListWidgetItem * current, QListWidgetItem * previous )
{
  stationtab->currentStationChanged(current,previous);
}


void qtsSidebar::setObsInfo(QString s)
{
  obsInfo->setText(s);
  if(s.isEmpty())
    obsInfo->hide();
  else
    obsInfo->show();
}


QString  qtsSidebar::fillList(const vector<std::string>& v, const StationTab::lEntry l)
{
  QStringList qlist;
  for(unsigned int i=0;i<v.size();i++) {
    qlist << v[i].c_str();
  }

  if(l==StationTab::CMFIMEXSTYLE)
    return fimextab->setStyles(qlist);

  if(l==StationTab::CMFIMEXMODEL) {
    fimextab->setModels(qlist);
    return QString("");
  }

  if(l==StationTab::CMFIMEXRUN) {
    qSort(qlist.begin(), qlist.end(), qGreater<QString>());
    fimextab->setRuns(qlist);
    return QString("");
  }

  if(l==StationTab::CMWDBSTYLE)
    return wdbtab->setStyles(qlist);

  return stationtab->fillList(qlist,l);
}

///  Wdb ---------------------------


void qtsSidebar::tabChanged(int idx)
{
  actualIndex=idx;
  if(idx==wdbIdx){
    addWdbBookmarkButton->show();
    cacheQueryButton->show();
    addFimexBookmarkButton->hide();
    recordFimexButton->hide();
    collapseFimexButton->hide();
    expandFimexButton->hide();
    setObsInfo("");
    targetB->show();
    filterB->hide();
    emit changetype(tsRequest::WDBSTREAM);

  } else if (idx==stationIdx) {
    addWdbBookmarkButton->hide();
    addFimexBookmarkButton->hide();
    collapseFimexButton->hide();
    expandFimexButton->hide();
    cacheQueryButton->hide();
    recordFimexButton->hide();
    targetB->show();
    filterB->show();
    setObsInfo("");

    emit changetype(tsRequest::HDFSTREAM);
  } else if (idx==fimexIdx) {
    addFimexBookmarkButton->show();
    recordFimexButton->show();
    collapseFimexButton->show();
    expandFimexButton->show();
    addWdbBookmarkButton->hide();
    cacheQueryButton->hide();
    targetB->show();
    filterB->hide();
    recordToggled(fimexRexordToggled);
    Q_EMIT changetype(tsRequest::FIMEXSTREAM);
  }
}



void qtsSidebar::enableWdb(bool has_wdb)
{
  wdbtab->setEnabled(has_wdb);
  int wdbtabindex =  tabs->indexOf(wdbtab);
  tabs->setTabEnabled(wdbtabindex,has_wdb);
}


void qtsSidebar::enableFimex(bool has_fimex)
{
  fimextab->setEnabled(has_fimex);
  int fimextabindex =  tabs->indexOf(fimextab);
  tabs->setTabEnabled(fimextabindex,has_fimex);
}




void qtsSidebar::enableBusyLabel(bool enable)
{
  if(enable) {
    connectStatus->setMovie(busyLabel);
    busyLabel->start();
  } else {
    connectStatus->clear();
  }
}




bool qtsSidebar::restoreWdbFromLog(std::string mod, std::string sty, double lat, double lon, std::string run,std::string posname)
{
  if(!wdbtab->isEnabled()) {
    cout << "WDB-tab disabled log information discarded" << endl;
    return false;
  }

  wdbtab->setStyle( sty.c_str() );
  wdbtab->setModel( mod.c_str() );
  wdbtab->setRun(   run.c_str() );
  wdbtab->setCoordinates(lon,lat,posname.c_str());
  return true;
}

bool qtsSidebar::restoreFimexFromLog(std::string mod, std::string sty, std::string expanded)
{
  if(!fimextab)
    return false;

  fimextab->setStyle(sty.c_str());
  fimextab->setModel(mod.c_str());
  fimextab->setExpandedDirs(expanded);

  return true;
}


void qtsSidebar::enableCacheButton(bool enable, bool force, unsigned long querytime)
{

  if(wdbtab->getActiveCacheRequest() && !force) return;
  bool cacheenabled = cacheQueryButton->isEnabled();

  if  (!enable) {
    if (!cacheenabled ) return;
    cacheQueryButton->setToolTip( QString("WDB Query is fast enough ( %1 ms ) query caching disabled ").arg(querytime) );
  } else {
    cacheQueryButton->setToolTip( QString("WDB Query is slow ( %1 ms )- query caching enabled").arg(querytime) );
  }

  cacheQueryButton->setEnabled(enable);
}


void qtsSidebar::chacheQueryActivated()
{
  wdbtab->setActiveCacheRequest(true);

  cacheQueryButton->setToolTip( QString("WDB caching already done for this model"));
  cacheQueryButton->setEnabled(false);

  enableBusyLabel(true);
  Q_EMIT requestWdbCacheQuery();
}

void qtsSidebar::writeBookmarks()
{
  if(wdbtab)
    wdbtab->writeBookmarks();
  if(fimextab)
    fimextab->writeBookmarks();
}

void qtsSidebar::setProgress(int progr, std::string text)
{

  QString txt(text.c_str());

  progressHeader->setText(QString("<b>Leser Data for:  %1</b>").arg(txt.section(':',0,0)));
  progressHeader->show();


  progress->show();
  progress->setValue(progr);

  progress->setFormat(txt.section(':',1,1));
}

void qtsSidebar::endProgress()
{
  progress->reset();
  progress->hide();
  progressHeader->hide();
}
