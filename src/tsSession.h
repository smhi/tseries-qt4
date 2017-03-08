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

#ifndef _SessionManager_h
#define _SessionManager_h

#include "tsSessionOptions.h"
#include "tsRequest.h"

#include <puTools/miTime.h>

#include <pets2/ptGlobals.h>
#include <tsData/ptParameterDefinition.h>
#include <pets2/ptStyle.h>

#include <string>
#include <vector>
#include <map>

// Should have a list of required models for a station
// for each style type
class SessionManager {
public:
  enum DiagramTab { ADD_TO_STATION_TAB, ADD_TO_WDB_TAB, ADD_TO_FIMEX_TAB};
private:
  ParameterDefinition pdef;
  struct modeldata{       // name of menu and id to models
    std::string modelname;
    Model modelid;
  };
  std::vector<modeldata> models;

  struct pardata{
    int midx;                  // model index
    std::vector<ParId> params; // the parameters
  };
  struct styledata{
    std::string stylename;      // name of style
    std::string stylefile;      // name of stylefile
    pets2::ptStyle  style;      // pets-style
    bool modelchoice;                // whether there is a choice of models
    std::vector<int> modelidx;       // model indices for modelchoice
    std::vector<ParId> params;       // id without model
    std::vector<pardata> fullparams; // full id with model
    DiagramTab diagramtab;
  };
  std::vector<styledata> styles;

  bool checkEnvironment(std::string& t);
public:

  // get defined stylenames, return number of style
  int getStyleTypes(std::vector<std::string>& stylenam, SessionManager::DiagramTab tab = ADD_TO_STATION_TAB);
  // get a PETS style by name
  const pets2::ptStyle& getStyle(const std::string, SessionManager::DiagramTab tab = ADD_TO_STATION_TAB);
  // get a PETS style by index
  const pets2::ptStyle& getStyle(int idx);
  // get the PETS style index - WDBadd
  int getStyleIndex(const std::string name, SessionManager::DiagramTab tab = ADD_TO_STATION_TAB);

  // make a sessionoption with given style,model and run
  bool getShowOption(SessionOptions&, int, Model, Run);
  bool getShowOption(SessionOptions&,const tsRequest *, SessionManager::DiagramTab tab = ADD_TO_STATION_TAB);


  // get a list of available models for a given styleindex
  int getModels(const std::string& s,
      std::map<std::string,Model>& modid,
      std::vector<std::string>& modname,
      SessionManager::DiagramTab tab = ADD_TO_STATION_TAB);


  // get a list of available runs for style/model
  int getRuns(const int sidx, const int midx,
      std::vector<Run>& runid,
      std::vector<std::string>& runname);

  // read diagram-sessions from file
  void readSessions(const std::string& fname,const std::string& stylepath, bool verbose = false);
  // for test-purposes
  void makeTestSessions();
};

#endif
