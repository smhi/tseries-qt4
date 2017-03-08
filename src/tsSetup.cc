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

#include <puMet/symbolMaker.h>
#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

#include "config.h"

using namespace std;
using namespace boost;

vector<tsSetup::dsStruct>  tsSetup::streams;

tsSetup::klstruct          tsSetup::klima;
tsSetup::morastruct        tsSetup::mora;
tsSetup::wdbstruct         tsSetup::wdb;
tsSetup::fistruct          tsSetup::files;
tsSetup::svstruct          tsSetup::server;
tsSetup::ptstruct          tsSetup::path;
tsSetup::gustruct          tsSetup::gui;
tsSetup::distruct          tsSetup::diana;
tsSetup::dostruct          tsSetup::doc;
bool                       tsSetup::Initialised = false;
string                     tsSetup::lang;
tsSetup::fimexstruct       tsSetup::fimex;
tsSetup::logstruct         tsSetup::loglevel;
tsSetup::disablestruct     tsSetup::disabled;


symbolMaker tsSetup::wsymbols;


tsSetup::tsSetup() : sec(PUBLIC) , line(0)
{
  if(!Initialised) {
    // giving standard values...
    idx=0;
    ids=0;
    wdb.readtime=2000;
    wdb.maxRecord=20;
    klima.baseQuery="?ct=text/plain&del=semicolon&nmt=0";
    klima.url="http://klapp.oslo.dnmi.no/metnopub/production/metno"+klima.baseQuery;
    klima.maxDistance=50;
    klima.maxObservationLength=300;

    loglevel.data="WARN";
    loglevel.diagram="WARN";
    loglevel.tseries="WARN";

    doc.docURL  = "https://wiki.met.no/tseries/doc";
    doc.newsURL = "https://wiki.met.no/tseries/news";
    fimex.externalPosService = "http://halo-search.met.no/collection1/select?q=name:";
    fimex.xmlSyntax = "metno";
  }
}

// General:

std::string tsSetup::getenvAsString(std::string env)
{
  ostringstream ost;
  ost << getenv(env.c_str());
  return ost.str();
}



string tsSetup::inSection()
{
  switch(sec) {
  case PUBLIC:
    return  ", in <public>";
  case FILES:
    return  ", in <files>";
  case STREAMS:
    return  ", in <streams>";
  case SERVER:
    return ", in <server>";
  case PATH:
    return ", in <path>";
  case GUI:
    return ", in <gui>";
  case DIANA:
    return ", in <diana>";
  case DOC:
    return ", in <doc>";
  case LOGLEVEL:
    return ", in <loglevel>";
  case KLIMA:
    return ", in <klima>";
  case KLIMAPARAMETER:
    return ", in <klimaparameter>";
  case KLIMANORMAL:
    return ", in <klimanormal>"; 
  case MORA:
    return ", in <mora>";
  case MORAPARAMETER:
    return ", in <moraparameter>";
  case MORANORMAL:
    return ", in <moranormal>"; 
  case WDB:
    return ", in <wdb>";
  case WDBPARAMETER:
    return  ", in <wdbparameter>";
  case WDBVECTORFUNCTIONS:
    return  ", in <wdbVectorFunctions>";
  case INFIMEX:
    return ", in <fimex>";
  case FIMEXPARAMETER:
    return ", in <fimexparameter>";
  }
  return ",in <unknown>";


};

void tsSetup::warn(string& k, warning w)
{
  cerr << "Warning! ";

  switch(w) {
  case wKEY:
    cerr <<"unexpected Key: "   << k << inSection();
    break;
  case wTOKEN:
    cerr <<"unexpected Token: " << k << inSection();
    break;
  case wSECTION:
    cerr <<"unknown Section: " << k;
    break;
  case wFILE:
    cerr <<"Setupfile: " << k <<" doesn\'t exist" << endl;
    return;
  }
  cerr << " in " << fname << " at line " << line << endl;
}


void tsSetup::fetchSite(string token)
{
  to_upper(token);
  erase_all(token,"[");
  erase_all(token,"]");
  trim(token);
  actualSites.clear();

  if(token.empty())
    return;

  vector<string> tokens = tokenize(token,":");

  for(unsigned int i=0;i<tokens.size();i++){
    actualSites.insert(tokens[i]);
  }
}


void tsSetup::fetchSection(string token)
{
  to_upper(token);

  if( find_first(token,"PUBLIC"))
    sec =  PUBLIC;
  else if( find_first(token,"FILES")) {
    sec =  FILES;
  }
  else if( find_first(token,"STREAMS"))
    sec = STREAMS;
  else if( find_first(token,"SERVER"))
    sec = SERVER;
  else if( find_first(token,"PATH"))
    sec = PATH;
  else if( find_first(token,"GUI"))
    sec = GUI;
  else if( find_first(token,"DIANA"))
    sec = DIANA;
  else if( find_first(token,"LOGLEVEL"))
    sec = LOGLEVEL;
  else if( find_first(token,"DOC"))
    sec = DOC;
  else if( find_first(token,"KLIMAPARAMETER"))
    sec = KLIMAPARAMETER;
  else if( find_first(token,"KLIMANORMAL"))
    sec = KLIMANORMAL;
  else if( find_first(token,"KLIMA"))
    sec = KLIMA;
  else if( find_first(token,"MORAPARAMETER"))
    sec = MORAPARAMETER;
  else if( find_first(token,"MORANORMAL"))
    sec = MORANORMAL;
  else if( find_first(token,"MORA"))
    sec = MORA;
  else if( find_first(token,"WDBPARAMETER"))
    sec = WDBPARAMETER;
  else if ( find_first(token,"WDBVECTORFUNCTIONS"))
    sec = WDBVECTORFUNCTIONS;
  else if( find_first(token,"WDB"))
    sec = WDB;
  else if( find_first(token,"FIMEXPARAMETER"))
    sec = FIMEXPARAMETER;
  else if (find_first(token,"FIMEX"))
    sec = INFIMEX;
  else
    warn(token,wSECTION);

}


std::string tsSetup::timetrans(std::string tstr)
{
  miutil::miTime now=miutil::miTime::nowTime();

  std::string transtime = now.format(tstr);

  cerr << "created time: " << transtime << endl;

  return transtime;

}

void tsSetup::setup(int& to, const string& from)
{
  to = atoi(from.c_str());
}
void tsSetup::setup(float& to, const string& from)
{
  to = atof(from.c_str());
}
void tsSetup::setup(string& to, const string& from)
{
  to = from;
}

bool tsSetup::splitToken(const string& token,string& key,string& content, bool upper)
{
  content ="";
  if(!find_first(token,"="))
    return false;

  vector<string> vtmp;
  split(vtmp,token,is_any_of("="));

  key = ( upper ? to_upper_copy(vtmp[0]) : vtmp[0] );

  if(vtmp.size() ==2)
    content = vtmp[1];

  trim(key);
  trim(content);

  return true;
}
std::vector<std::string> tsSetup::tokenize(std::string token,std::string delimiters)
{
  vector<string> result;
  split(result,token,is_any_of(delimiters));
  for(unsigned int i=0;i<result.size();i++)
    trim(result[i]);

  return  result;
}


void tsSetup::stripComments(string& token)
{
  if(find_first(token,"#")) {
    int c = token.find_first_of("#",0);
    int k=token.length() -  c;
    token.erase(c,k);
  }
  trim(token);
}


bool tsSetup::read(const string& f, string s)
{
  site  = to_upper_copy(s);

  path.home      = getenvAsString("HOME");
  lookup["HOME"] = path.home;

  files.filter = path.home + "/.tseries/tseries.filter";
  files.wdbBookmarks=path.home + "/.tseries/bookmarks.wdb";
  files.fimexBookmarks=path.home + "/.tseries/bookmarks.fimex";

  string package_version=PVERSION;

  if(!readsetup(f))
    if(!readsetup( path.home+"/.tseries/tseries.ctl"))
      if(!readsetup("/etc/tseries/"+package_version+"/tseries.ctl" ))
        if(!readsetup("/etc/tseries/tseries.ctl"))
          if(!readsetup("/usr/local/etc/tseries/tseries.ctl")) {
            cerr << "NO setup found!" << endl;
            return false;
          }
  return true;
}

bool tsSetup::readsetup(string fname)
{
  ifstream in(fname.c_str());

  cerr << "Trying setupfile  : ............. " << fname;

  if(!in) {
    cerr << " - Not found!" << endl;
    return false;
  }
  cerr << " - OK!" <<  endl;

  string token;

  while(in) {
    getline(in,token);
    line++;
    stripComments(token);

    if(token.empty())
      continue;

    if(token.size() < 2)
      continue;

    if(token[0] =='<' && token[token.size()-1] == '>' ) {
      fetchSection(token);
      continue;
    }
    if(token[0] =='[' && token[token.size()-1] == ']' ) {
      fetchSite(token);
      continue;
    }

    setSimpleToken(token);
  }

  in.close();

  cerr << "WEATHERSYMBOLS AT:" << files.weatherSymbols << endl;
  if(!Initialised)
    wsymbols.readSymbols(files.weatherSymbols);

  Initialised = true;

  return true;

}

bool tsSetup::checkLookup(string& t)
{
  if(!miutil::contains(t,"$("))
    return false;

  int start,stop;

  start = t.find("$(",0) + 2;
  stop  = t.find(")",start);

  if(stop < start ) {
    warn(t,wTOKEN);
    return false;
  }

  string s = t.substr(start, stop-start);
  string r = string("$(") + s + string(")");
  string n;
  to_upper(s);

  if( lookup.count(s) > 0 )
    n = lookup[s];

  replace_first(t,r,n);
  return true;
}



bool tsSetup::checkEnvironment(string& t)
{
  if(!find_first(t,"${"))
    return false;

  int start,stop;

  start = t.find("${",0) + 2;
  stop  = t.find("}",start);

  if(stop < start ) {
    warn(t,wTOKEN);
    return false;
  }

  string s = t.substr(start, stop-start);
  string r = string("${") + s + "}";
  string p = string("$(") + s + ")";

  to_upper(s);

  string n = getenvAsString(s);
  if(!n.empty()) {

    replace_first(t,r,n);
    return true;
  }


  replace_first(t,r,p);

  return checkLookup(t);
}


void tsSetup::setSimpleToken(string token)
{

  if(!actualSites.empty())
    if(!actualSites.count(site))
      return;

  if(sec==WDBVECTORFUNCTIONS) {
    wdb.vectorFunctions.push_back(token);
    return;
  }

  bool upper=(sec != WDBPARAMETER && sec != KLIMAPARAMETER && sec != KLIMANORMAL && sec != MORAPARAMETER && sec != MORANORMAL && sec != INFIMEX && sec != FIMEXPARAMETER);

  string content,key;


  if(!splitToken(token,key,content,upper))
    return;

  while(checkLookup(content))
    ;

  while(checkEnvironment(content))
    ;

  if ( key == "NOWTIME") {
    lookup[key] = timetrans(content);
    return;
  }

  lookup[key] = content;

  switch(sec) {
  case  PUBLIC:
    setPublic(key,content);
    break;
  case   FILES:
    setFiles(key,content);
    break;
  case  STREAMS:
    setStreams(key,content);
    break;
  case  SERVER:
    setServer(key,content);
    break;
  case GUI:
    setGui(key,content);
    break;
  case DIANA:
    setDiana(key,content);
    break;
  case DOC:
    setDoc(key,content);
    break;
  case KLIMAPARAMETER:
    setKlimaParameter(key,content);
    break;
  case KLIMANORMAL:
    setKlimaNormal(key,content);
    break;
  case KLIMA:
    setKlima(key,content);
    break;
  case MORAPARAMETER:
    setMoraParameter(key,content);
    break;
  case MORANORMAL:
    setMoraNormal(key,content);
    break;
  case MORA:
    setMora(key,content);
    break;
  case LOGLEVEL:
    setLoglevel(key,content);
    break;
  case PATH:
    setPath(key,content);
    break;
  case WDB:
    setWdb(key,content);
    break;
  case WDBPARAMETER:
    setWdbParameter(key,content);
    break;
  case INFIMEX:
    setFimex(key,content);
    break;
  case FIMEXPARAMETER:
    setFimexParameter(token);
    break;
  case WDBVECTORFUNCTIONS:
    // ignore here, handled above
    break;
  }
}

bool tsSetup::setBool(string token)
{
  to_upper(token);
  trim(token);

  return (token == "TRUE");
}



// DIRECT SET: INSERT NEW STUFF HERE!!!!--------------------------



void tsSetup::setPublic(string& key, string& content)
{
  if(key == "LANG")
    setup(lang,content);

  if(key == "DISABLEWDB")
    disabled.wdb = setBool(content);
  if(key == "DISABLEHDF")
    disabled.hdf = setBool(content);
  if(key == "DISABLEFIMEX")
    disabled.fimex = setBool(content);

  if(key == "DISABLEMORA")
    disabled.mora = setBool(content);

  if(key == "DISABLEKLIMA")
    disabled.klima = setBool(content);


}


void tsSetup::setFimexParameter(string& token)
{
  // allow a token over several lines ...
  if(find_first(token,"="))
    fimex.parameters.push_back(token);
  else {
    // ... but don't let them crash us for that
    if(fimex.parameters.size())
      fimex.parameters.back() += token;
  }
}

void tsSetup::setFimex(string& key, string& content)
{
  if(key == "FimexStreamTypes") {
    vector<string> tmpTypes = tokenize(content,":");
    for(unsigned int i=0; i < tmpTypes.size();i++) {
      fimex.streamtypes.insert(tmpTypes[i]);
    }
  } else if(key == "externalPositionService") {
    setup(fimex.externalPosService,content);
  } else if(key == "xmlSyntax") {
    setup(fimex.xmlSyntax,content);
  } else if (key == "FimexFilters"){
    vector<string> tmpPar = tokenize(content,":");
    for(unsigned int i=0; i < tmpPar.size();i++) {
      fimex.filters.push_back(tmpPar[i]);
    }
  }
}




void tsSetup::setKlima(string& key, string& content)
{
  if(key == "URL" ){
    setup(klima.url,content+klima.baseQuery);
  } else if(key == "MAXDISTANCE") {
    setup(klima.maxDistance,content);
  } else if(key == "MAXOBSERVATIONLENGTH") {
    setup(klima.maxObservationLength,content);
  } else {
    cerr << "warn here" << endl;
    warn(key,wKEY);
  }
}

void tsSetup::setMora(string& key, string& content)
{
  if(key == "URL" ){
    setup(mora.url,content);
  } else if(key == "MONTHNORMALREPORT") {
    setup(mora.monthlynormalreport,content);
  } else if(key == "STATIONREPORT") {
    setup(mora.stationreport,content);
  } else if(key == "DATAREPORT") {
    setup(mora.datareport,content);    
  } else if(key == "MAXDISTANCE") {
    setup(mora.maxDistance,content);
  } else if(key == "MAXOBSERVATIONLENGTH") {
    setup(mora.maxObservationLength,content);
  } else {
    cerr << "warn here" << endl;
    warn(key,wKEY);
  }
}

void tsSetup::setLoglevel(std::string& key, std::string& content)
{
  if(key == "TSERIES" ){
    setup(loglevel.tseries,content);
  } else if(key == "DATA") {
    setup(loglevel.data,content);
  } else if(key == "DIAGRAM") {
    setup(loglevel.diagram,content);
  } else {
    cerr << "warn here" << endl;
    warn(key,wKEY);
  }






}




void tsSetup::setFiles(string& key, string& content)
{
  if(key == "DEFS" )
    setup(files.defs,content);
  else if(key == "CONFIGURE")
    setup(files.configure,content);
  else if(key == "WEATHERSYMBOLS")
    setup(files.weatherSymbols,content);
  else if(key == "STDIMAGE")
    setup(files.std_image,content);
  else if(key == "NEWIMAGE")
    setup(files.new_station_image,content);
  else if(key == "FINIMAGE")
    setup(files.fin_image,content);
  else if(key == "ICONIMAGE")
    setup(files.icon_image,content);
  else if(key == "ACTIVEIMAGE")
    setup(files.active_image,content);
  else if(key == "BASEFILTER")
    setup(files.baseFilter,content);
  else if(key == "COMMONBOOKMARKS")
    setup(files.commonBookmarks,content);
  else if(key == "WDBBOOKMARKS")
    setup(files.wdbBookmarks,content);
  else if(key == "FIMEXBOOKMARKS")
    setup(files.fimexBookmarks,content);
  else
    warn(key,wKEY);
}


/////////////////////////////////////

void tsSetup::setStreams(string& key, string& content)
{


  if (find_first(key,"COLLECTIONNAME")) {
    streams.push_back(dsStruct());
    idx=streams.size()-1;
    ids=0;

    setup(streams[idx].collectionName,content);
    return;
  }

  if(streams.empty()) return;

  if (key == "INITIALOPEN") {
    setup(streams[idx].InitialOpen,content);
    return;
  }
  else if (key == "PREFERREDDIAGRAM") {
    setup(streams[idx].preferredStyle,content);
    return;
  }
  else if (key == "DATAFILE" ) {
    streams[idx].data.push_back(sStruct());
    ids=streams[idx].data.size()-1;
    setup(streams[idx].data[ids].name,content);
    return;
  }

  if(streams[idx].data.empty())
    return;

  if(key == "DATADESCRIPTION" )
    setup(streams[idx].data[ids].descript,content);
  else if(key == "DATATYPE" )
    setup(streams[idx].data[ids].type,content);
  else if(key == "CONTENTS"){
    setup(streams[idx].data[ids].contents,content);
  }
  else if (key == "DATACONFIG") {
    setup(streams[idx].data[ids].config,content);
  }
  else
    warn(key,wKEY);

}

void tsSetup::setWdbParameter(string& key, string& content)
{
  wdb.parameters[key]=content;
}

void tsSetup::setKlimaParameter(string& key, string& content)
{
  klima.parameters[key]=content;
}
void tsSetup::setKlimaNormal(string& key, string& content)
{
  klima.normals[key]=content;
}

void tsSetup::setMoraParameter(string& key, string& content)
{
  mora.parameters[key]=content;
}
void tsSetup::setMoraNormal(string& key, string& content)
{
  mora.normals[key]=content;
}



void tsSetup::setWdb(string& key, string& content)
{
  if(key == "HOST" )
    setup(wdb.host,content);
  else if(key == "USER")
    setup(wdb.user,content);
  else if(key=="READTIME") {
    int rtime;
    setup(rtime,content);
    wdb.readtime=rtime;
  } else if(key == "BUSYMOVIE") {
    setup(wdb.busyMovie,content);
  }else if (key == "MAXRECORD") {
    setup(wdb.maxRecord,content);
  }else
    warn(key,wKEY);
}



void tsSetup::setServer(string& key, string& content)
{
  if(key == "CLIENT" )
    setup(server.client,content);
  else if (key == "COMMAND" )
    setup(server.command,content);
  else if (key == "NAME" )
    setup(server.name,content);
  else
    warn(key,wKEY);

}

void tsSetup::setGui(string& key, string& content)
{
  if(key == "ORIGOLON")
    setup(gui.origoLon,content);
  else if (key == "ORIGOLAT")
    setup(gui.origoLat,content);
  else if (key == "STYLE")
    setup(gui.style,content);
  else
    warn(key,wKEY);
}


void tsSetup::setPath(string& key, string& content)
{
  if(key == "WORK" )
    setup(path.work,content);
  else if(key == "STYLES")
    setup(path.styles,content);
  else if(key == "IMAGES" )
    setup(path.images,content);
  else if(key == "ETC" )
    setup(path.etc,content);
  else if(key == "TMP" )
    setup(path.tmp,content);
  else if(key == "SAVES")
    setup(path.saves,content);
  else if(key == "DOC")
    setup(path.doc,content);
  else if(key == "LANG")
    path.lang = tokenize(content,":");
  else
    warn(key,wKEY);
}

void tsSetup::setDiana(string& key, string& content)
{
  if(key == "NAME" )
    setup(diana.name,content);
  else if (key == "COMMAND" )
    setup(diana.command,content);
  else if (key == "WORKDIR" )
    setup(diana.workdir,content);
  else if (key == "ARGS" )
    setup(diana.args,content);
  else
    warn(key,wKEY);
}


void tsSetup::setDoc(string& key, string& content)
{
  if(key == "DOCURL" )
    setup(doc.docURL,content);
  else if (key == "NEWSURL" )
    setup(doc.newsURL,content);
  else
    warn(key,wKEY);
}

void tsSetup::overrideToken(std::string line )
{
  vector<string> vl = tokenize(line,":");

  if(vl.size() < 2 ) {
    cerr << "Override token failed with " << line << " no section found! Usage [section:key=token]" << endl;
    return;
  }
  string section= vl[0];
  string token  = vl[1];

  cout << "fetching section: -------- " << section << endl;

  fetchSection(section);

  cout << " set simple token: -------- " << token << endl;

  setSimpleToken(token);
}



