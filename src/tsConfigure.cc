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
#include "tsConfigure.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;


map<string,string>             tsConfigure::contents;
vector<tsConfigure::tsCustoms>     tsConfigure::custom;



void tsConfigure::setDefaults()
{
  contents.clear();
  contents["SORT"]   = "alpha";     // alphabetical sort. alternative: origo
  contents["LON" ]   = "20";        // lon for origo
  contents["LAT" ]   = "66";        // lat  --"--
  contents["ALL"]    = "0";         // station list is not style depended
  contents["SAVEONQUIT"] = "1";     // save settings on quit
  contents["SHOWNORMAL"] = "1";     // show pos names in DIANA
  contents["SHOWSELECT"] = "1";     // show selected pos names in DIANA
  contents["TIMEMARK"]   = "1";     // show now in Diagram
  contents["VISIBLESTART"] ="0";
  contents["VISIBLELENGTH"]="0";
  contents["LOCKHOURSTOMODEL"]="0";
  contents["SHOWGRIDLINES"]="1";

}

void tsConfigure::fetchSection(string token)
{
  to_upper(token);

  if ( find_first(token,"STYLE")) {
    sec =  STYLE;
    its++;
  }
  else if( find_first(token,"LIST")) {
    sec = LIST;
    itl++;
  }
  else
    sec =  PUBLIC;
}


bool tsConfigure::splitToken(const string& token,string& key,string& cont)
{
  cont ="";
  if(!find_first(token,"="))
    return false;

  vector<string> vtmp;
  split(vtmp,token,is_any_of("="));

  key = to_upper_copy(vtmp[0]);

  if(vtmp.size() ==2)
    cont = vtmp[1];
  trim(key);
  trim(cont);
  return true;
}

void tsConfigure::stripComments(string& token)
{
  if(find_first(token,"#")) {
    int c = token.find_first_of("#",0);
    int k=token.length() -  c;
    token.erase(c,k);
  }
  trim(token);
}


bool tsConfigure::read(const string& fname, string instancename)
{
  ostringstream ost;
  ost << fname;
	if(instancename.size() > 0)
	  ost << "-" << instancename;


  ifstream in(ost.str().c_str());

  setDefaults();

  if(!in)
    return false;

  string token;
  its = -1;
  itl = -1;

  sec = PUBLIC;

  while(in) {
    getline(in,token);
    stripComments(token);

    if(token.empty())
      continue;
    if(find_first(token,"<") && find_first(token,">")) {
      fetchSection(token);
      continue;
    }
    setToken(token);
  }

  in.close();

  return true;

}

void tsConfigure::setToken(string token)
{
  string cont,key;

  if(sec == LIST )
    setList(token);

  if(!splitToken(token,key,cont))
    return;

  if(sec == STYLE )
    setStyle(key,cont);
  else
    contents[key] = cont;
}

void tsConfigure::set(string key, const string token)
{
  contents[to_upper_copy(key)] = token;
}

void tsConfigure::set(string key, const int token)
{
  ostringstream ost;
  ost << token;
  set(key,ost.str());
}

void tsConfigure::set(string key, const float token)
{
  ostringstream ost;
  ost << token;
  set(key,ost.str());
}

void tsConfigure::set(string key, const bool token)
{
  contents[to_upper_copy(key)] = ( token ? "1" : "0");
}



void tsConfigure::setStyle(string& /*key*/, string& /*cont*/)
{

}


void tsConfigure::setList(string token)
{
  string cont,key;
  if(splitToken(token,key,cont)) {
    if( key == "NAME" ) {
      if(!cont.empty())
        custom[itl].name = cont;
      else {
        ostringstream ost;
        ost << "Untitled" <<  itl;
        custom[itl].name = ost.str();
      }
    }
  }
  else
    custom[itl].list.push_back(token);
}

vector<string> tsConfigure::getList(string search)
{
  for(unsigned int i=0; i< custom.size();i++)
    if(custom[i].name == search)
      return custom[i].list;
  vector<string> empty;
  return  empty;
}

vector<string> tsConfigure::getCustoms()
{
  vector<string> names;
  for(unsigned int i=0; i< custom.size();i++)
    names.push_back(custom[i].name);
  return names;
}


bool tsConfigure::get(const string& key, string& cont)
{
  if(contents.count(key)) {
    cont = contents[key];
    return true;
  }
  return false;
}


bool tsConfigure::get(const string& key, int& cont)
{
  if(contents.count(key)) {
    cont = atoi(contents[key].c_str());
    return true;
  }
  return false;
}

bool tsConfigure::get(const string& key, float& cont)
{
  if(contents.count(key)) {
    cont = atof(contents[key].c_str());
    return true;
  }
  return false;
}

bool tsConfigure::get(const string& key, bool& cont)
{
  if(contents.count(key)) {
    int a = atoi(contents[key].c_str());
    cont = bool(a);
    return true;
  }
  return false;
}



bool tsConfigure::save(string fname)
{
  ofstream out(fname.c_str());
  map<string,string>::iterator itr = contents.begin();

  if(!out)
    return false;

  out << "## Auto-generated config file for tseries do not edit!"
      <<  endl;

  for(;itr != contents.end();itr++)
    out << itr->first << "=" << itr->second << endl;

  for( unsigned int i=0; i< custom.size(); i++) {
    out << "<LIST>" << endl
        << "name="  << custom[i].name << endl;
    for(unsigned int j=0; j< custom[i].list.size(); j++ )
      out << custom[i].list[j] << endl;

  }


  out.close();
  return true;
}
