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
#include "tsSessionOptions.h"
#include <set>

#define SO_MAX_MODELS 20

using namespace std;
using namespace miutil;

const vector<ParId> emptyvec;
const std::string emptystr;



Model SessionOptions::getmodel(const int idx)
{
  return (idxOk(idx) ? mdata[idx].model : M_UNDEF );
}

const std::string& SessionOptions::getmodelname(const int idx)
{
  return (idxOk(idx) ? mdata[idx].modelname : emptystr );
}

const vector<ParId>& SessionOptions::paramVector(const int idx)
{
  return (idxOk(idx) ? mdata[idx].parameters : emptyvec );
}

const vector<ParId> SessionOptions::distinctParamVector(const int idx)
{
  if(!idxOk(idx))
    return emptyvec;
  vector<ParId> inlist=mdata[idx].parameters;
  set<string> doublettblocker;
  for (unsigned j=0; j<inlist.size();j++) {
    if(doublettblocker.count(inlist[j].toString()))
      inlist.erase(inlist.begin(),inlist.begin()+j);
    else
      doublettblocker.insert(inlist[j].toString());
  }
  return inlist;
}


int SessionOptions::addModel(const Model mid, const std::string name)
{
  if (numModels() < SO_MAX_MODELS){
    modeldata md;
    md.model= mid;
    md.modelname= name;
    mdata.push_back(md);
    return (mdata.size()-1);
  }
  return(-1);
}

bool SessionOptions::addParam(const ParId parid, const int idx)
{
  if (idxOk(idx)) {
    mdata[idx].parameters.push_back(parid);
    return(true);
  }
  return(false);
}

void SessionOptions::getTinterval(miTime& sta, miTime& sto)
{
  sta = start;
  sto = stop;
}

void SessionOptions::setTinterval(miTime& sta, miTime& sto)
{
  if (sto >= sta) {
    start = sta;
    stop = sto;
  }
}
