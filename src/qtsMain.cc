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
#include "qtsMain.h"

#include <QTimerEvent>
#include <QPainter>
#include <QPixmap>
#include <QCloseEvent>
#include <QFileInfo>
#include <QFontDialog>
#include <QPrintDialog>
#include <QStringList>
#ifdef HAVE_QTSVG
#include <QSvgGenerator>
#endif
#include <QDesktopServices>
#include <QUrl>

#include "config.h"
#include "ParameterFilterDialog.h"
#include "PopupCalendar.h"

#include <coserver/ClientSelection.h>
#include <coserver/QLetterCommands.h>
#include <puTools/ttycols.h>
#include <puTools/miDate.h>
#include <puTools/miStringFunctions.h>
#include <tsData/FimexStream.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>

#include "tseries.xpm"


using namespace std;
using namespace miutil;

const std::string thisTM = "MARKEDTIME";
const std::string dianaTM = "DIANATIME";

namespace /* anonymous */ {

inline QString asQString(bool b)
{
  return b ? "true" : "false";
}

} // namespace

qtsMain::qtsMain(std::string l, const QString& name)
  : lang(l)
{
  // Added to avoid unnessecary updates when connected to diana
  // and diana is in automatic update mode
  currentTime = miTime::nowTime();

  latlond = false;
  makeMenuBar();

  work = new qtsWork(this,lang.c_str());
  setCentralWidget(work);

  makeConnectButtons();
  pluginB->setClientName(name);

  work->latlonInDecimalToggled(latlond);

  toggleLockHoursToModel(lockHoursToModel);
  toggleShowGridlines(showGridLines);

  printer=0;

  setWindowIcon(QPixmap(tseries_xpm));

  restoreLog();
  setRemoteParameters();
  setTimemark(currentTime);

  connect(work, SIGNAL(selectionTypeChanged()), this,
      SLOT(selectionTypeChanged()));
  connect(work, SIGNAL(coordinatesChanged()), this, SLOT(coordinatesChanged()));

  connect(work, SIGNAL(fimexPoslistChanged()), this, SLOT(fimexPoslistChanged()));

  connect(work,SIGNAL(fimexPositionChanged(const QString&)), this, SLOT(fimexPositionChanged(const QString&)));

  // milliseconds
  int updatetimeout = (1000 * 60) * 2;

  updateTimer   = startTimer(updatetimeout);
  progressTimer = startTimer(500);
}

void qtsMain::makeMenuBar()
{
  makeFileMenu();
  makeSettingsMenu();
  makeHelpMenu();
}

void qtsMain::makeHelpMenu()
{
  menu_help = menuBar()->addMenu(tr("Help"));

  showHelpAct = new QAction(tr("Manual"), this);
  showHelpAct->setShortcut(tr("F1"));
  showHelpAct->setStatusTip(tr("Show manual"));
  connect(showHelpAct, SIGNAL(triggered()), this, SLOT( showHelp() ));
  menu_help->addAction(showHelpAct);


  showNewsAct = new QAction(tr("News"), this);
  showNewsAct->setStatusTip(tr("Show changelog"));
  connect(showNewsAct, SIGNAL(triggered()), this, SLOT(showNews()));
  menu_help->addAction(showNewsAct);


  aboutAct = new QAction(tr("About"), this);
  connect(aboutAct, SIGNAL(triggered()), this, SLOT( about() ));
  menu_help->addAction(aboutAct);
}

void qtsMain::makeFileMenu()
{
  menu_file = menuBar()->addMenu(tr("File"));

  printAct = new QAction(tr("Print"), this);
  printAct->setShortcut(tr("Ctrl+P"));
  printAct->setStatusTip(tr("Print diagram"));
  connect(printAct, SIGNAL(triggered()), this, SLOT( print() ));
  menu_file->addAction(printAct);

  rasterAct = new QAction(tr("Save Image"), this);
  connect(rasterAct, SIGNAL(triggered()), this, SLOT( raster() ));
  menu_file->addAction(rasterAct);

  filterAct = new QAction(tr("Change filter"), this);
  connect(filterAct, SIGNAL(triggered()), this, SLOT( manageFilter() ));
  menu_file->addAction(filterAct);

  filterParametersAct = new QAction(tr("Change Observation filter"), this);
  connect(filterParametersAct, SIGNAL(triggered()), this,
      SLOT( manageParameterFilter() ));
  menu_file->addAction(filterParametersAct);


  filterFimexAct = new QAction(tr("Change Fimex filter"), this);
    connect(filterFimexAct, SIGNAL(triggered()), this,
        SLOT( manageFimexFilter() ));
    menu_file->addAction(filterFimexAct);


  observationStartAct = new QAction(tr("Change Observation start date"), this);
  connect(observationStartAct, SIGNAL(triggered()), this,
      SLOT( changeObservationStart() ));
  menu_file->addAction(observationStartAct);


  // -------------------

  menu_file->addSeparator();

  quitAct = new QAction(tr("quit"), this);
  quitAct->setShortcut(tr("Ctrl+Q"));
  quitAct->setStatusTip(tr("Quit program"));
  connect(quitAct, SIGNAL(triggered()), this, SLOT( quit() ));
  menu_file->addAction(quitAct);
}

void qtsMain::makeSettingsMenu()
{
  menu_setting = menuBar()->addMenu(tr("Preferences"));

  // -------------------

  readLogAct = new QAction(tr("Reset Preferences"), this);
  connect(readLogAct, SIGNAL(triggered()), this, SLOT( readLog() ));
  menu_setting->addAction(readLogAct);

  writeLogAct = new QAction(tr("Save Preferences"), this);
  connect(writeLogAct, SIGNAL(triggered()), this, SLOT( writeLog() ));
  menu_setting->addAction(writeLogAct);

  menu_setting->addSeparator();

  // --------------------

  sOnQuitAct = new QAction(tr("Save at exit"), this);
  sOnQuitAct->setCheckable(true);

  bool isOn;
  config.get("SAVEONQUIT", isOn);
  sOnQuitAct->setChecked(isOn);
  connect(sOnQuitAct, SIGNAL(toggled(bool)), this, SLOT( setSaveOnQuit(bool) ));
  menu_setting->addAction(sOnQuitAct);
  menu_setting->addSeparator();

  // ---------------------

  config.get("SHOWNORMAL", snormal);
  normalAct = new QAction(tr("Show positions (DIANA)"), this);
  normalAct->setCheckable(true);
  normalAct->setChecked(snormal);
  connect(normalAct, SIGNAL(toggled(bool)), this,
      SLOT(toggleNormalNames(bool)));
  menu_setting->addAction(normalAct);

  config.get("SHOWSELECT", sselect);
  selectAct = new QAction(tr("Show active position (DIANA)"), this);
  selectAct->setCheckable(true);
  selectAct->setChecked(sselect);
  connect(selectAct, SIGNAL(toggled(bool)), this,
      SLOT(toggleSelectedNames(bool)));
  menu_setting->addAction(selectAct);

  config.get("SHOWICON", sicon);
  iconAct = new QAction(tr("Show icons (DIANA)"), this);
  iconAct->setCheckable(true);
  iconAct->setChecked(sicon);
  connect(iconAct, SIGNAL(toggled(bool)), this, SLOT(toggleIcon(bool)));
  menu_setting->addAction(iconAct);

  sposition = true; //no logging
  positionAct = new QAction(tr("Send positions (DIANA)"), this);
  positionAct->setCheckable(true);
  positionAct->setChecked(sposition);
  connect(positionAct, SIGNAL(toggled(bool)), this,
      SLOT(togglePositions(bool)));
  menu_setting->addAction(positionAct);

  menu_setting->addSeparator();

  // ------------------------

  config.get("TIMEMARK", tmark);
  tmarkAct = new QAction(tr("Show timemark"), this);
  tmarkAct->setCheckable(true);
  tmarkAct->setChecked(tmark);
  connect(tmarkAct, SIGNAL(toggled(bool)), this, SLOT(toggleTimemark(bool)));
  menu_setting->addAction(tmarkAct);

  config.get("SHOWGRIDLINES", showGridLines);
  showGridLinesAct = new QAction(tr("Show Gridlines"), this);
  showGridLinesAct->setCheckable(true);
  showGridLinesAct->setChecked(showGridLines);
  connect(showGridLinesAct, SIGNAL(toggled(bool)), this,
      SLOT(toggleShowGridlines(bool)));
  menu_setting->addAction(showGridLinesAct);

  config.get("LATLONDEC", latlond);
  latlonAct = new QAction(tr("Lat/Lon in decimal"), this);
  latlonAct->setCheckable(true);
  latlonAct->setChecked(latlond);
  connect(latlonAct, SIGNAL(toggled(bool)), this, SLOT(toggleLatLon(bool)));
  menu_setting->addAction(latlonAct);

  menu_setting->addSeparator();





  config.get("LOCKHOURSTOMODEL", lockHoursToModel);
  lockHoursToModelAct = new QAction(tr("Lock Hours to Model"), this);
  lockHoursToModelAct->setCheckable(true);
  lockHoursToModelAct->setChecked(lockHoursToModel);
  connect(lockHoursToModelAct, SIGNAL(toggled(bool)), this,
      SLOT(toggleLockHoursToModel(bool)));
  menu_setting->addAction(lockHoursToModelAct);


  // ------------------------

  fontAct = new QAction(tr("Font"), this);
  connect(fontAct, SIGNAL(triggered()), this, SLOT(chooseFont()));
  menu_setting->addAction(fontAct);
  menu_setting->addSeparator();

  // ------------------------

  menu_lang = menu_setting->addMenu(tr("Languages"));
  findLanguages();
}

void qtsMain::makeConnectButtons()
{
  connect(work, SIGNAL(refreshStations()), this, SLOT(refreshDianaStations()));

  pluginB = work->sideBar()->pluginButton();
  targetB = work->sideBar()->targetButton();

  connect(targetB, SIGNAL(pressed()), this, SLOT(sendTarget()));
  connect(targetB, SIGNAL(released()), this, SLOT(clearTarget()));

  connect(pluginB, SIGNAL(receivedMessage(int, const miQMessage&)),
      SLOT(processLetter(int, const miQMessage&)));
  connect(pluginB, SIGNAL(disconnected()), SLOT(cleanConnections()));

  // makeConnectButtons is called after makeMenuBar
  menu_setting->addAction(pluginB->getMenuBarAction());
}

void qtsMain::closeEvent(QCloseEvent*)
{
  quit();
}

void qtsMain::quit()
{
  bool soq;
  config.get("SAVEONQUIT", soq);

  if (soq)
    writeLog();
  qApp->quit();
}

void qtsMain::raster()
{
  std::string format = "PNG";
  std::string fname = "./" + work->file("png");

  QString fpath = fname.c_str();
  QString fcaption = "save file dialog";
#ifdef HAVE_QTSVG
  const QString fpattern = "Pictures (*.png *.xpm *.bmp *.pdf *.ps *.eps *.svg);;All (*.*)";
#else // !HAVE_QTSVG
  const QString fpattern = "Pictures (*.png *.xpm *.bmp *.pdf *.ps *.eps);;All (*.*)";
#endif // !HAVE_QTSVG

  QString s = QFileDialog::getSaveFileName(this, fcaption, fpath, fpattern);
  if (s.isNull())
    return;

  QFileInfo finfo(s);
  fpath = finfo.absolutePath();

  qtsShow* w = work->Show();
  QImage* image = 0;
  QPaintDevice* device = 0;
  if (s.endsWith(".pdf") || s.endsWith(".ps") || s.endsWith(".eps")) {
    QPrinter* printer = new QPrinter(QPrinter::ScreenResolution);
    if (s.endsWith(".pdf"))
      printer->setOutputFormat(QPrinter::PdfFormat);
    else
      printer->setOutputFormat(QPrinter::PostScriptFormat);
    printer->setOutputFileName(s);
    printer->setFullPage(true);
    printer->setPaperSize(QSizeF(w->width(), w->height()), QPrinter::DevicePixel);

    // FIXME copy from bdiana
    // According to QTBUG-23868, orientation and custom paper sizes do not
    // play well together. Always use portrait.
    printer->setOrientation(QPrinter::Portrait);

    device = printer;
#ifdef HAVE_QTSVG
  } else if (filename.endsWith(".svg")) {
    QSvgGenerator* generator = new QSvgGenerator();
    generator->setFileName(filename);
    generator->setSize(w->size());
    generator->setViewBox(QRect(0, 0, w->width(), w->height()));
    generator->setTitle(tr("tseries image"));
    generator->setDescription(tr("Created by tseries %1.").arg(PVERSION));

    // FIXME copy from bdiana
    // For some reason, QPrinter can determine the correct resolution to use, but
    // QSvgGenerator cannot manage that on its own, so we take the resolution from
    // a QPrinter instance which we do not otherwise use.
    QPrinter sprinter;
    generator->setResolution(sprinter.resolution());

    device = generator;
#endif // HAVE_QTSVG
  } else {
    image = new QImage(w->size(), QImage::Format_ARGB32_Premultiplied);
    image->fill(Qt::transparent);
    device = image;
  }

  w->paintOn(device);

  if (image)
    image->save(s);

  delete device;
}

void qtsMain::print()
{
  if (!printer)
    printer = new QPrinter();

  QString fname = QString::fromStdString(work->file("ps"));
  QString ofn = printer->outputFileName();

  if (ofn.isNull()) {
    QFileInfo p(fname);
    printer->setOutputFileName(p.absoluteFilePath());
  } else {
    QFileInfo p(ofn);
    printer->setOutputFileName(p.path() + "/" + fname);
  }

  printer->setOutputFormat(QPrinter::NativeFormat);
  printer->setOrientation(QPrinter::Landscape);

  QPrintDialog *dialog = new QPrintDialog(printer, this);
  dialog->setWindowTitle(tr("Print Diagram"));

  if (dialog->exec() != QDialog::Accepted)
    return;

  QApplication::setOverrideCursor(Qt::WaitCursor);
  work->Show()->paintOn(printer);
  QApplication::restoreOverrideCursor();

  // reset number of copies (saves a lot of paper)
  printer->setNumCopies(1);
}

void qtsMain::about()
{
  QMessageBox::about(
      this,
      tr("About T-series"),
      tr("T-series: Time series viewer\nVersion: %1\nCopyright: met.no 2016")
      .arg(VERSION));
}

void qtsMain::writeLog()
{
  tsSetup setup;

  config.set("SIZEX", this->width());
  config.set("SIZEY", this->height());
  config.set("POSX", this->x());
  config.set("POSY", this->y());

  config.set("SHOWNORMAL", snormal);
  config.set("SHOWSELECT", sselect);
  config.set("SHOWICON", sicon);
  config.set("TIMEMARK", tmark);
  config.set("LATLONDEC", latlond);
  config.set("FONT", std::string(qApp->font().toString().toStdString()));
  config.set("LOCKHOURSTOMODEL", lockHoursToModel);
  config.set("SHOWGRIDLINES", showGridLines);

  {
    std::ostringstream p;
    const QStringList peers = pluginB->getSelectedClientNames();
    for (int i=0; i<peers.count(); ++i)
      p << ' ' << peers.at(i).toStdString();
    config.set("COSERVER_PEERS", p.str());
  }

  if ((not lang.empty()))
    config.set("LANG", lang);

  work->collectLog();

  config.save(setup.files.configure);
}

void qtsMain::restoreLog()
{
  int sx, sy, px, py;
  std::string f;

  if (config.get("SIZEY", sy) && config.get("SIZEX", sx))
    this->resize(sx, sy);

  if (config.get("POSX", px) && config.get("POSY", py))
    this->move(px, py);

  if (config.get("FONT", f)) {
    QFont font;
    if (font.fromString(f.c_str()))
      qApp->setFont(font);
  }

  if (config.get("COSERVER_PEERS", f)) {
    const std::vector<std::string>  cp = miutil::split(f, 0, " ");
    QStringList peers;
    for (size_t i=0; i<cp.size(); ++i)
      peers << QString::fromStdString(cp[i]);
    pluginB->setSelectedClientNames(peers);
  }

  work->restoreLog();
}

void qtsMain::readLog()
{
  tsSetup setup;
  config.read(setup.files.configure);
  restoreLog();
}

void qtsMain::setSaveOnQuit(bool)
{
  bool soq;
  config.get("SAVEONQUIT", soq);
  config.set("SAVEONQUIT", !soq);
}

void qtsMain::toggleNormalNames(bool isOn)
{
  snormal = isOn;
  sendNamePolicy();
}

void qtsMain::toggleSelectedNames(bool isOn)
{
  sselect = isOn;
  sendNamePolicy();
}

void qtsMain::toggleIcon(bool isOn)
{
  sicon = isOn;
  sendNamePolicy();
}

void qtsMain::togglePositions(bool isOn)
{
  sposition = isOn;
  if (!isDianaConnected())
    return;

  if (sposition)
    refreshDianaStations();

  miQMessage m(sposition ? qmstrings::showpositions : qmstrings::hidepositions);
  if (work->getSelectionType() == qtsWork::SELECT_BY_FIMEX) {
    m.addDataDesc(DATASET_FIMEX);
  } else {
    m.addDataDesc(DATASET_TSERIES + work->lastList());
  }
  sendLetter(m);
}

void qtsMain::toggleTimemark(bool isOn)
{
  tmark = isOn;
  setTimemark(miTime::nowTime());
}

void qtsMain::toggleLatLon(bool isOn)
{
  latlond = isOn;
  if (work)
    work->latlonInDecimalToggled(latlond);
}

void qtsMain::toggleShowGridlines(bool isOn)
{
  showGridLines = isOn;
  if (work)
    work->setShowGridLines( showGridLines );
}


void qtsMain::toggleLockHoursToModel(bool isOn)
{
  lockHoursToModel = isOn;
  if (work)
    work->toggleLockHoursToModel(lockHoursToModel);
}

void qtsMain::setTimemark(miTime mark)
{
  if (work) {
    if (tmark)
      work->Show()->setTimemark(mark, thisTM);
    else
      work->Show()->clearTimemarks(thisTM);
  }
}

// DIANA

void qtsMain::setDianaTimemark(miTime mark)
{
  if (work) {
    if (isDianaConnected())
      work->Show()->setTimemark(mark, dianaTM);
    else
      work->Show()->clearTimemarks(dianaTM);
  }
}

// send one image to diana (with name)
void qtsMain::sendImage(const QString& name, const QImage& image)
{
  if (!isDianaConnected())
    return;
  if (image.isNull())
    return;

  QByteArray *a = new QByteArray();
  QDataStream s(a, QIODevice::WriteOnly);
  s << image;

  miQMessage m(qmstrings::addimage);
  m.addDataDesc("name").addDataDesc("image");

  QStringList values;
  values << name;

  QString img;
  int n = a->count();
  for (int i = 0; i < n; i++) {
    img += " " + QString::number(int((*a).data()[i]));
  }
  values << img;

  m.addDataValues(values);
  sendLetter(m);
}

void qtsMain::refreshDianaStations()
{
  if (!isDianaConnected() || !sposition)
    return;

  QString prevModel = currentModel;

  if (work->getSelectionType() == qtsWork::SELECT_BY_FIMEX) {
    sendNewPoslist();
    sendNamePolicy();
    disablePoslist(prevModel);
    enableCurrentPoslist();
    return;
  }

  const QString ll = work->lastList();
  if (currentModel == ll)
    return;

  currentModel = ll;

  if (!sendModels.count(currentModel))
    sendNewPoslist();

  sendNamePolicy();
  enableCurrentPoslist();
  disablePoslist(prevModel);
}

void qtsMain::disablePoslist(const QString& prev)
{
  if (prev == NOMODEL_TSERIES)
    return;
  if (!isDianaConnected())
    return;

  miQMessage m(qmstrings::hidepositions);
  m.addDataDesc(DATASET_TSERIES + prev);
  sendLetter(m);
}

void qtsMain::enableCurrentPoslist()
{
  if (!isDianaConnected())
    return;
  miQMessage m(qmstrings::showpositions);
  if (work->getSelectionType() == qtsWork::SELECT_BY_FIMEX) {
    m.addDataDesc(DATASET_FIMEX);
  } else {
    m.addDataDesc(DATASET_TSERIES + currentModel);
  }
  sendLetter(m);
}

void qtsMain::sendNewPoslist()
{
  sendModels.insert(currentModel);
  if (!isDianaConnected())
    return;
  miQMessage m = work->getStationList();
  m.addCommon("normal", (snormal ? "true" : "false"))
      .addCommon("selected", sselect ? "true" : "false");
  sendLetter(m);
}

void qtsMain::sendTarget()
{
  if (!isDianaConnected())
    return;

  miQMessage m = work->target();
  sendLetter(m);

  miQMessage m2(qmstrings::showpositions);
  m2.addDataDesc(TARGETS_TSERIES);
  sendLetter(m2);
}


void qtsMain::clearFimexList()
{
  if (!isDianaConnected())
    return;

  miQMessage m(qmstrings::hidepositions);
  m.addDataDesc(DATASET_FIMEX);
  sendLetter(m);
}


void qtsMain::clearTarget()
{
  if (!isDianaConnected())
    return;

  miQMessage m(qmstrings::hidepositions);
  m.addDataDesc(TARGETS_TSERIES);
  sendLetter(m);
}

// called when client-list changes

void qtsMain::processConnect(int diana_id)
{
  if (dianaconnected.find(diana_id) != dianaconnected.end()) {
    // already connected, strange
    return;
  }
  dianaconnected.insert(diana_id);

  tsSetup s;
  cerr << ttc::color(ttc::Blue) << "< CONNECTING TO: " << s.server.client
       << " > " << ttc::reset << endl;

  QImage sImage(s.files.std_image.c_str());
  QImage fImage(s.files.fin_image.c_str());
  QImage iImage(s.files.icon_image.c_str());
  QImage nImage(s.files.new_station_image.c_str());
  QImage aImage(s.files.active_image.c_str());

  // FIXME these send messages to all diana clients
  sendImage(IMG_STD_TSERIES,  sImage);
  sendImage(IMG_FIN_TSERIES,  fImage);
  sendImage(IMG_NEW_TSERIES,  nImage);
  sendImage(IMG_ICON_TSERIES, iImage);
  sendImage(IMG_ACTIVE_TSERIES, aImage);

  sendNamePolicy();
  selectionTypeChanged();
  refreshDianaStations();

  setRemoteParameters();
}

void qtsMain::setRemoteParameters()
{
  const bool dc = isDianaConnected();
  targetB->setEnabled(dc);
  normalAct->setEnabled(dc);
  selectAct->setEnabled(dc);
  iconAct->setEnabled(dc);

  if (!dc) {
    currentModel = NOMODEL_TSERIES;
    sendModels.clear();
  }
}

void qtsMain::sendLetter(const miQMessage& qmsg)
{
  pluginB->sendMessage(qmsg); // send to all
}

void qtsMain::sendNamePolicy()
{
  if (!isDianaConnected())
    return;

  miQMessage m(qmstrings::showpositionname);
  m.addDataDesc("normal").addDataDesc("selected").addDataDesc("icon");

  QStringList values;
  values << asQString(snormal);
  values << asQString(sselect);
  values << asQString(sicon);
  m.addDataValues(values);

  sendLetter(m);
}

// processes incoming miMessages

void qtsMain::processLetter(int from, const miQMessage& letter)
{
#ifdef DEBUG
  cerr << "Message: " << letter << endl;
#endif

  const QString& cmd = letter.command();
  if (cmd == qmstrings::removeclient || cmd == qmstrings::newclient) {
    tsSetup s;
    if (letter.getCommonValue("type") == QString::fromStdString(s.server.client)) {
      const int diana_id = letter.getCommonValue("id").toInt();
      if (cmd == qmstrings::newclient)
        processConnect(diana_id);
      else
        cleanConnection(diana_id);
    }
  } else if (cmd == qmstrings::positions) {
    if (letter.getCommonValue("dataset") == "diana") {
      if (letter.countDataRows()) {
        const int i_lon = letter.findDataDesc("lon"), i_lat = letter.findDataDesc("lat");
        const float lon = letter.getDataValue(0, i_lon).toFloat();
        const float lat = letter.getDataValue(0, i_lat).toFloat();
        work->changePositions(lon, lat);
      }
    }
  } else if (cmd == qmstrings::selectposition) {
    if (letter.countDataRows()) {
      const int i_station = letter.findDataDesc("station");
      if (i_station >= 0)
        work->changeStation(letter.getDataValue(0, i_station));
    }
  } else if (cmd == qmstrings::timechanged) {
    // Added to avoid invalid updates from diana when diana
    // is in automatic update mode
    const miTime timeFromDiana(letter.getCommonValue("time").toStdString());
    if (timeFromDiana != currentTime) {
      currentTime = timeFromDiana;
      setDianaTimemark(currentTime);
    }
  }
}

void qtsMain::showHelp()
{
  tsSetup setup;
  QDesktopServices::openUrl(QUrl( setup.doc.docURL.c_str()));
}

void qtsMain::showNews()
{
  tsSetup setup;
  QDesktopServices::openUrl(QUrl( setup.doc.newsURL.c_str()));
}



void qtsMain::timerEvent(QTimerEvent* e)
{
  if (e->timerId() == updateTimer)
    if (work) {
      work->updateStreams();
      setTimemark(miTime::nowTime());
    }

  if (e->timerId() == progressTimer)
    if(work)
      work->updateProgress();
}

void qtsMain::cleanConnection(int diana_id)
{
  if (dianaconnected.find(diana_id) == dianaconnected.end()) {
    // not connected, strange
    return;
  }
  dianaconnected.erase(diana_id);
  if (!isDianaConnected()) {
    cout << ttc::color(ttc::Red) << "< DISCONNECTING >" << ttc::reset << endl;

    setRemoteParameters();
    setDianaTimemark(miTime::nowTime());
  }
}

void qtsMain::cleanConnections()
{
  if (dianaconnected.empty())
    return;
  dianaconnected.clear();
  cout << ttc::color(ttc::Red) << "< DISCONNECTING >" << ttc::reset << endl;

  setRemoteParameters();
  setDianaTimemark(miTime::nowTime());
}

void qtsMain::changeObservationStart()
{
  miTime start = work->getObservationStartTime();
  int year = start.year();
  int month=start.month();
  int day = start.day();
  QDate qstart(year,month,day);

  PopupCalendar * calendar = new PopupCalendar(this,qstart);

  if (calendar->exec()) {
    qstart=calendar->result();
    qstart.getDate(&year,&month,&day);
    start.setTime(year,month,day,0,0,0);
    work->setObservationStartTime(start);
  }
}


void qtsMain::manageParameterFilter()
{
  set<string>    blacklist= work->getKlimaBlacklist();
  vector<string> allklimaParameters = work->getAllKlimaParameters();

  ParameterFilterDialog * filterdialog = new ParameterFilterDialog(blacklist,allklimaParameters, this);

  if (filterdialog->exec()) {
    blacklist=filterdialog->result();
    work->setKlimaBlackList(blacklist);
  }
}


void qtsMain::manageFimexFilter()
{
  set<string>    fimexParameterFilter = pets::FimexStream::getParameterFilter();
  vector<string> allFimexParameters   = pets::FimexStream::getAllParameters();

  ParameterFilterDialog * filterdialog = new ParameterFilterDialog(fimexParameterFilter,allFimexParameters, this);

  if (filterdialog->exec()) {
    fimexParameterFilter=filterdialog->result();
    pets::FimexStream::setParameterFilter(fimexParameterFilter);
    work->refresh(true);
  }
}


void qtsMain::manageFilter()
{
  set<std::string> p = work->fullPosList();
  set<std::string> f = work->Filter();
  set<std::string> o = work->createFilter(true);

  qtsFilterManager * fm = new qtsFilterManager(p, f, o, this);

  if (fm->exec()) {
    // clean out the old filters from diana
    if (isDianaConnected()) {
      set<QString>::iterator itr = sendModels.begin();
      while (itr != sendModels.end()) {
        const set<QString>::iterator ite = itr++; // set::erase invalidates iterator
        if (ite->contains(TS_MINE))
          sendModels.erase(*ite);
      }
      if (currentModel.contains(TS_MINE))
        currentModel = NOMODEL_TSERIES;
    }
    // new filter
    work->newFilter(fm->result());
  }
}

void qtsMain::chooseFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok, qApp->font(), this);
  if (ok) {
    // font is set to the font the user selected
    qApp->setFont(font);
  } else {
    // the user cancelled the dialog; font is set to the initial
    // value: do nothing
  }
}

void qtsMain::findLanguages()
{
  QDir d;

  tsSetup setup;
  std::string dlang = (setup.path.lang.empty() ? "./" : setup.path.lang[0]);

  d.setPath(dlang.c_str());
  QStringList f = d.entryList(QStringList("tseries_??.qm"));

  languageGroup = new QActionGroup(this);
  connect(languageGroup, SIGNAL( triggered(QAction*) ), this,
      SLOT( toggleLang(QAction*) ));

  for (QStringList::Iterator it = f.begin(); it != f.end(); ++it) {
    QString s = *it;
    s.replace("tseries_", "");
    s.replace(".qm", "");

    QAction * action = new QAction(s, languageGroup);
    action->setCheckable(true);
    action->setChecked(s.toStdString() == lang);
    languageGroup->addAction(action);
    menu_lang->addAction(action);
  }

  QAction * action = new QAction("en", languageGroup);
  action->setCheckable(true);
  action->setChecked(lang.empty() || lang == "en");
  languageGroup->addAction(action);
  menu_lang->addAction(action);
}

void qtsMain::toggleLang(QAction* action)
{
  const QString qlang = action->text();
  lang = qlang.toStdString();

  QMessageBox::information(
      this,
      tr("Language Changed"),
      tr("tseries must be restarted to reset the language to: [%1]").arg(qlang));
}

void qtsMain::selectionTypeChanged()
{
  if (work->getSelectionType() == qtsWork::SELECT_BY_WDB) {
    togglePositions(false);
    sendTarget();
    clearFimexList();
  } else if (work->getSelectionType() == qtsWork::SELECT_BY_FIMEX) {
    clearTarget();
    togglePositions(true);
  } else {
    togglePositions(true);
    clearTarget();
    clearFimexList();
  }
}

void qtsMain::coordinatesChanged()
{
  sendTarget();
}


void qtsMain::fimexPoslistChanged()
{
  refreshDianaStations();
}


void qtsMain::fimexPositionChanged(const QString& qname)
{
  if (!isDianaConnected())
    return;

  miQMessage m(qmstrings::changeimageandtext);
  m.addCommon("", DATASET_FIMEX);
  m.addDataDesc("name").addDataDesc("image");

  QStringList values;
  values << qname << IMG_ACTIVE_TSERIES;
  m.addDataValues(values);

  if (!lastFimexPosition.isEmpty()) {
    QStringList last;
    last << lastFimexPosition << IMG_STD_TSERIES;
    m.addDataValues(last);
  }
  lastFimexPosition = qname;

  sendLetter(m);
}
