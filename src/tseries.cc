/*
  Tseries - A Free Meteorological Timeseries Viewer

  Copyright (C) 2006-2016 met.no

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


#include "tsSetup.h"
#include "tsConfigure.h"
#include "qtsMain.h"

#include <puTools/miCommandLine.h>
#include <puTools/miTime.h>
#include <puTools/miStringFunctions.h>

#include <QTranslator>
#include <QApplication>

#include <fstream>

using namespace std;
using namespace miutil;

int main(int argc, char **argv)
{
  vector<miCommandLine::option> o;
  o.push_back(miCommandLine::option( 's',"setup"    , 1 ));
  o.push_back(miCommandLine::option( 'h',"help"     , 0 ));
  o.push_back(miCommandLine::option( 'f',"file"     , 1 ));
  o.push_back(miCommandLine::option( 'S',"site"     , 1 ));
  o.push_back(miCommandLine::option( 'l',"lang"     , 1 ));
  o.push_back(miCommandLine::option( 'T',"title"    , 1 ));
  o.push_back(miCommandLine::option( 'I',"instancename", 1 ));
  o.push_back(miCommandLine::option( 'd',"define"   , 1 ));
  o.push_back(miCommandLine::option( 'H',"host"     , 1 ));
  o.push_back(miCommandLine::option( 'u',"user"     , 1 ));

  miCommandLine cl(o,argc, argv);

  // read setup

  string    site         = "FOU";
  string    setupfile    = "tseries.ctl";
  string    title        = "T-series";
  string    instancename = "";
  tsSetup     setup;
  tsConfigure config;
  std::string    lang;

  if(cl.hasFlag('h')){
    cerr << "Usage: "
        << argv[0]
                << "  -s setupfile "  << endl
                << "  -S site      "  << endl
                << "  -l lang      "  << endl
                << "  -T title     "  << endl
                << "  -I instancename" << endl
                << "  -H wdbhost   "  << endl
                << "  -u wdbuser   "  << endl
                << "  -d section1:key1=token1 section2:key2=token2 "
                << endl << endl;
    exit (0);
  }

  if(cl.hasFlag('S')) site         = cl.arg('S')[0];
  if(cl.hasFlag('s')) setupfile    = cl.arg('s')[0];
  if(cl.hasFlag('T')) title += " " + cl.arg('T')[0];
  if(cl.hasFlag('I')) instancename = (cl.arg('I')[0]);


  if(!setup.read(setupfile,site))
    exit(0);



  if(!setup.lang.empty())
    lang=setup.lang;

  config.read(setup.files.configure,instancename);
  config.get("LANG",lang);

  if(cl.hasFlag('d')) {
    vector<std::string> overridetokens=cl.arg('d');
    for (vector<std::string>::iterator it = overridetokens.begin(); it != overridetokens.end(); ++it)
      setup.overrideToken(*it);
  }

  if(cl.hasFlag('H')) setup.overrideToken("WDB:host="+cl.arg('H')[0]);
  if(cl.hasFlag('u')) setup.overrideToken("WDB:user="+cl.arg('u')[0]);


  if(cl.hasFlag('l')) lang=cl.arg('l')[0];


  QApplication a( argc, argv );
  QTranslator  myapp( 0 );
  QTranslator  qt( 0 );

  miTime defTime;
  defTime.setDefaultLanguage(lang.c_str());

  if(!setup.gui.style.empty())
    a.setStyle(setup.gui.style.c_str());


  if(!lang.empty()) {

    string qtlang   = "qt_"     +lang;
    string langfile = "tseries_"+lang;

    // translation file for application strings

    for(unsigned int i=0;i<setup.path.lang.size(); i++ )
      if( qt.load( qtlang.c_str(),setup.path.lang[i].c_str()))
        break;

    for(unsigned int i=0;i<setup.path.lang.size(); i++ )
      if( myapp.load( langfile.c_str(),setup.path.lang[i].c_str()))
        break;

    a.installTranslator( &qt    );
    a.installTranslator( &myapp );

  }


  qtsMain *main = new qtsMain(lang, instancename.c_str());

  main->setWindowTitle(title.c_str());
  main->show();

  return a.exec();
}
