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
#include "tsRequest.h"

#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string.hpp>
#include <sstream>

using namespace std;
using namespace miutil;

bool tsRequest::setString(const std::string& i, std::string& o)
{
  if (o == i)
    return false;
  o = i;
  return true;
}


bool tsRequest::setRun(int i)
{
  if (run_ == i)
    return false;
  run_ = i;
  return true;
}

ostream& operator<<(ostream& out, const tsRequest& rhs)
{
  out <<           rhs.sty_
      << " : " <<  rhs.pos_
      << " : " <<  rhs.mod_
      << " "   <<  rhs.run_;
  return out;
};

std::string tsRequest::file(const std::string type) const
{
  std::string rst = mod_ + "_" + pos_;

  if(streamtype == WDBSTREAM)
    rst = wdbModel + "_" + wdbstationname;

  if(streamtype == FIMEXSTREAM)
    rst = fimexModel + "_" + fimexName;

  miutil::replace(rst, ".","");

  miutil::replace(rst, " ","_");
  miutil::replace(rst, "\xD8","OE");
  miutil::replace(rst, "\xC5","AA");
  miutil::replace(rst, "\xC6","AE");
  miutil::replace(rst, "\xDC","UE");
  miutil::replace(rst, "\xD6","OE");
  miutil::replace(rst, "\xC4","AE");

  miutil::replace(rst, "\xF8","oe");
  miutil::replace(rst, "\xE5","aa");
  miutil::replace(rst, "\xE6","ae");
  miutil::replace(rst, "\xFC","ue");
  miutil::replace(rst, "\xF6","oe");
  miutil::replace(rst, "\xE4","ae");

  rst = miutil::to_lower(rst);

  rst +="."+type;
  return rst;
}

/// WDB
bool tsRequest::setWdbPos(double lon,double lat)
{
  if( (fabs(lon-wdbLon) < 0.00001 ) && (fabs(lat-wdbLat) < 0.00001 )) return false;
  wdbLat=lat;
  wdbLon=lon;
  return true;
}

bool tsRequest::setWdbRun(miutil::miTime nrun)
{
  if(nrun == wdbRun) return false;
  wdbRun = nrun;
  return true;
}


void tsRequest::setType(tsRequest::Streamtype s)
{
  streamtype=s;
}



bool tsRequest::restoreWdbFromLog(std::string mod, std::string sty, double lat, double lon, miutil::miTime run,std::string posname)
{
  setWdbPos(lon,lat);
  setWdbStationName(posname);
  setWdbRun(run);
  setWdbModel(mod);
  setWdbStyle(sty);
  return true;
}



bool  tsRequest::setFimexLocation(double flat,double flon, std::string name)
{
  if(name == fimexName)
    return false;

  fimexLat = flat;
  fimexLon = flon;
  fimexName= name;
  return true;
}

bool tsRequest::getFimexLocation(double& lat, double& lon, std::string& name)
{
  lat = fimexLat;
  lon = fimexLon;
  name = fimexName;

  if(lat==0 && lon==0 && name =="")
    return false;
  return true;
}


std::string  tsRequest::getFimexInfo()
{
  ostringstream ost;
  ost << "fimexLat:   " <<  fimexLat     << endl
      << "fimexLat:   " <<  fimexLat     << endl
      << "fimexLon:   " <<  fimexLon     << endl
      << "fimexModel: " <<  fimexModel   << endl
      << "fimexStyle: " <<  fimexStyle   << endl
      << "fimexName:  " <<  fimexName    << endl
      << "fimexRun:   " <<  fimexRun     << endl;
  return ost.str();
}
