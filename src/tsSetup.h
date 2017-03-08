/*
  Tseries - A Free Meteorological Timeseries Viewer

  $Id$

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
#ifndef _tsSetup_h
#define _tsSetup_h

#include <puMet/symbolMaker.h>

#include <vector>
#include <map>
#include <set>

/// T-series setup read and storage
/** All Tseries setup is read in this class
 *  usually represented in the tseries.ctl file.
 *  The class has only one public function ( read() ) with the filename
 *  as input. All Parameters er stored in static structs.
 *  After generating a tsSetup with read() one can
 *  tsSetup setup
 *  std::string x=setup.path.work
 */


class tsSetup {
private:
  static bool           Initialised;
  std::string      site;
  std::set<std::string> actualSites;

  bool readsetup(std::string filename);

public:
  tsSetup();

  /// Function to create a setup object
  /** Takes in a filename and optional a switch.
   *  switches er defined in the setup-file as\n
   *  [MYSWITCH]\n
   *  and end with\n
   *  [ANOTHERSWITCH] or \n[]\n
   *  everything inside the switch is invisible to
   *  anybody except those who are using the specific switch\n
   *  Switches are free chooseable strings and can be combined.
   *  A section [ME:YOU] is valid for ME and YOU but no one else.
   *  By this, several people with different needs are able to
   *  share one setup-file without getting severe conflicts.
  */
  bool read(const std::string& filename, std::string s="");

  std::string getenvAsString(std::string);

  std::string getNowTimeAsString(std::string);

  // override with section:key=token
  void overrideToken(std::string token);

  // contents

  /// Struct containing a single PETS datastream
  struct sStruct{
    std::string  name;
    std::string  descript;
    std::string  type;
    std::string  contents;
    std::string  config;
  };

  /// Struct containing datastreams
  struct dsStruct {
    int  InitialOpen;
    std::string collectionName;
    std::string preferredStyle;
    std::vector<sStruct> data;
  };

  struct wdbstruct {
    std::string    host;
    std::string    user;
    std::string    busyMovie;
    unsigned long       readtime;            /// time to enable cache button (ms)
    std::vector<std::string> vectorFunctions;     ///< run vector transformations on these
    std::map<std::string,std::string> parameters; ///< translate parameters from wdb
    int                 maxRecord;           ///< size of record ringbuffer;
  };

  // define loglevels foe DATA DIAGRAM TSERIES
  struct logstruct {
    std::string data;
    std::string diagram;
    std::string tseries;
  };


  struct disablestruct {
    bool hdf;
    bool wdb;
    bool fimex;
    bool klima;
    bool mora;
    disablestruct() : hdf(false), wdb(false), fimex(false), klima(false), mora(false) {}
  };

  struct klstruct {
    std::string    url;
    std::string    baseQuery;
    int            maxDistance;
    int            maxObservationLength;
    std::map<std::string,std::string> parameters; ///< translate parameters from klima
    std::map<std::string,std::string> normals;    ///< translate monthly normals from klima
  };
  
  struct morastruct {
    std::string    url;
    std::string    baseQuery;
    std::string    stationreport;
    std::string    datareport;
    std::string    monthlynormalreport; ///<  computed only for climate stations
    int            maxDistance;
    int            maxObservationLength;
    std::map<std::string,std::string> parameters; ///< translate parameters from mora
    std::map<std::string,std::string> normals;    ///< translate monthly normals from mora
  };


  /// Struct containing filenames for tseries
  struct fistruct {
    std::string defs;
    std::string configure;
    std::string weatherSymbols;
    std::string std_image;
    std::string new_station_image;
    std::string fin_image;
    std::string icon_image;
    std::string active_image;
    std::string filter;
    std::string baseFilter;
    std::string wdbBookmarks;
    std::string fimexBookmarks;
    std::string commonBookmarks;
  };

  struct fimexstruct {
    std::vector<std::string>  parameters; // parsed in tsData
    std::set<std::string>     streamtypes;
    std::string               externalPosService; // where to search for stations (Halo/yr...)
    std::string               xmlSyntax; // How to parse the result from externalPosService, default metno
    std::vector<std::string>  filters;
  };


  /// Struct containing coserver-info
  struct svstruct {
    std::string name;
    std::string command;
    std::string client;
  };

  /// Struct containing path-names
  struct ptstruct {
    std::string styles;
    std::string work;
    std::string images;
    std::string etc;
    std::string tmp;
    std::string saves;
    std::string doc;
    std::string home;
    std::vector<std::string> lang;
  };

  /// Struct containing GUI-specific information
  struct gustruct {
    float origoLon;
    float origoLat;
    std::string style;
  };

  /// Struct containing information about DIANA
  struct distruct {
    std::string name;
    std::string command;
    std::string workdir;
    std::string args;
  };

  /// Struct containing documentation path etc.
  struct dostruct {
    std::string docURL;
    std::string newsURL;
  };

  static std::vector<dsStruct> streams; ///< Data streams

  static wdbstruct wdb;            ///< wdb information
  static fistruct files;           ///< All filenames
  static svstruct server;          ///< Server information
  static ptstruct path;            ///< General path info
  static gustruct gui;             ///< GUI info
  static distruct diana;           ///< DIANA information
  static dostruct doc;             ///< Documentation locations
  static klstruct klima;           ///< url and info for klapp connection
  static morastruct mora;           ///< url and info for klapp connection
  static std::string lang;    ///< Languages
  static fimexstruct fimex;        /// fimex stuff
  static symbolMaker wsymbols;
  static logstruct   loglevel;
  static disablestruct disabled;

private:
  enum { PUBLIC, FILES, STREAMS, SERVER, GUI, PATH, DIANA, DOC,WDB,WDBPARAMETER,WDBVECTORFUNCTIONS,
    KLIMA,KLIMAPARAMETER,KLIMANORMAL,MORA,MORAPARAMETER,MORANORMAL,INFIMEX,FIMEXPARAMETER,LOGLEVEL} sec;
  enum warning { wKEY, wTOKEN, wSECTION, wFILE };

  std::map<std::string,std::string> lookup;
  std::string fname;
  int line;
  int idx,ids;

  void fetchSite(std::string);
  void fetchSection(std::string);
  void setSimpleToken(std::string);
  bool checkLookup(std::string&);
  bool checkEnvironment(std::string&);
  void stripComments(std::string&);

  // change a string into a string containing nowtime
  std::string timetrans(std::string);

  std::string inSection();
  bool splitToken(const std::string&,std::string&, std::string&,bool upper=true);
  std::vector<std::string> tokenize(std::string token,std::string delimiters);

  void warn(std::string&,warning);

  void setLoglevel(std::string&, std::string&);
  void setPublic(std::string&, std::string&);
  void setFiles(std::string&, std::string&);
  void setStreams(std::string&, std::string&);
  void setServer(std::string&, std::string&);
  void setGui(std::string&, std::string&);
  void setPath(std::string&, std::string&);
  void setDiana(std::string&, std::string&);
  void setDoc(std::string&, std::string&);
  void setWdb(std::string&, std::string&);
  void setWdbParameter(std::string&, std::string&);
  void setKlimaParameter(std::string&, std::string&);
  void setKlimaNormal(std::string&, std::string&);
  void setKlima(std::string&, std::string&);
  void setMoraParameter(std::string&, std::string&);
  void setMoraNormal(std::string&, std::string&);
  void setMora(std::string&, std::string&);
  
  void setFimex(std::string&, std::string&);
  void setFimexParameter(std::string&);

  void setup(std::string&, const std::string&);
  void setup(int&,const std::string&);
  void setup(float&, const std::string&);
  bool setBool(std::string);


};

#endif
