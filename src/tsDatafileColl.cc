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
#include "tsDatafileColl.h"
#include <tsData/ptHDFFile.h>
#include <tsData/ptAsciiStream.h>
#include <glob.h>
#include <time.h>
#include <string>
#include <sstream>
#include "tsSetup.h"
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#ifdef GRIBSTREAM
#include <tsData/ptGribStream.h>
#endif
#include <set>

using namespace std;
using namespace miutil;

// Modify to use timefilter, se GridCollection::makeGridIOinstances() in GridCollection.cc.


vector<string> FimexFileindex::findNewFiles()
{
  glob_t glob_result;
  // Init the filter with pattern and remove YYYY ....
  tf.initFilter(glob_string);
  glob(glob_string.c_str(),GLOB_NOMAGIC,NULL,&glob_result);
  vector<string> result;
  for(unsigned int i=0;i<glob_result.gl_pathc;++i){
    string filename= string(glob_result.gl_pathv[i]);
    if(known_files.count(filename))
      continue;
    known_files.insert(filename);
    result.push_back(filename);
  }
  globfree(&glob_result);
  return result;
}

bool Union(const dataset& d1, const dataset& d2)
{
  for (int i = 0; i < MAXDATASETS; i++)
    if (d1.isdata(i) && d2.isdata(i))
      return true;
  return false;
}

bool Union(const dataset& d1, const dataset& d2, dataset& result)
{
  result.clear();
  for (int i = 0; i < MAXDATASETS; i++)
    if (d1.isdata(i) && d2.isdata(i))
      result.setdata(i);

  return !result.empty();
}

DatafileColl::DatafileColl()
    : wdbStream(0)
    , klimaStream(0)
    , moraStream(0)
    , tolerance(1000.0)
    , verbose(false)
    , streams_opened(false)
    , fimex_streams_opened(false)
{
  openWdbStream();
  openKlimaStream();
  openMoraStream();
  initialiseFimexPositions();
  initialiseFimexParameters();
}

DatafileColl::~DatafileColl()
{
  closeStreams();
  closeWdbStream();
  closeKlimaStream();
  closeMoraStream();
}

int DatafileColl::addDataset(std::string name)
{
  int n = datasetname.size();
  if (n < MAXDATASETS) {
    datasetname.push_back(name);
    numStationsDS[n] = 0;
    return n;
  }
  return -1;
}

string DatafileColl::getCleanStreamType(const string& streamtype)
{
  string st = streamtype;
  const string::size_type n = st.find(":");
  if (n > 0 && n <st.size())
    st.erase(n, st.size()-n);

  return boost::trim_copy(st);
}


int DatafileColl::addStream(const std::string name, const std::string desc,
    const std::string streamtype, const int dset, const int numindset,
    const std::string sparid, const std::string fimexconfig)
{
  tsSetup setup;

  string cleanstreamtype= getCleanStreamType(streamtype);

  if (dset < (signed int) datasetname.size()) {

    if(setup.fimex.streamtypes.count(cleanstreamtype)) {

      FimexFileindex   findex;
      findex.model        = desc;
      findex.sType        = streamtype;
      findex.glob_string  = name;
      findex.configfile   = fimexconfig;
      fimexFileindex.push_back(findex);
      createFimexStreams(fimexFileindex.back());

    } else {

      DsInfo dsinfo;
      //datastreams.push_back(dsinfo);
      dsinfo.streamname = name;
      dsinfo.descript = desc;
      dsinfo.sType = streamtype;
      dsinfo.dataSet = dset;
      dsinfo.numindset = numindset;
      dsinfo.dataStream = 0;
      dsinfo.streamOpen = false;
      dsinfo.mtime = 0;

      const std::vector<std::string> sp = miutil::split(sparid, ":");
      int m = sp.size();
      ParId parid;
      if (m > MAXMODELSINSTREAM) m = MAXMODELSINSTREAM;
      dsinfo.numModels = m;
      for (int i = 0; i < m; i++) {
        parid = parDef.Str2ParId(sp[i]);
        dsinfo.modelList[i] = parid.model;
        dsinfo.runList[i] = parid.run;
      }

      datastreams.push_back(dsinfo);
    }
    //    return datastreams.size();
    return 1;
  } else {
    return -1;
  }
}

bool DatafileColl::openStreams(const std::string mod)
{
  if (verbose)
    cout << "- Open streams with model " << mod << endl;
  bool b = false;
  for (unsigned int i = 0; i < datastreams.size(); i++) {
    if (datastreams[i].streamOpen)
      continue;
    for (int m = 0; m < datastreams[i].numModels; m++) {
      if (datastreams[i].modelList[m] == mod) {
        b |= openStream(i);
        break;
      }
    }
  }
  return b;
}

bool DatafileColl::openStream(const int idx)
{
  ErrorFlag ef = OK;

  if (idx < 0 || idx >= (signed int) datastreams.size())
    return false;

  datastreams[idx].numModels = 0;
  // Dont reopen stream if type MORA
  if (datastreams[idx].sType != "MORA") {
    delete datastreams[idx].dataStream;
    datastreams[idx].dataStream = 0;
  }

  if (verbose)
    cout << "About to open stream:" << datastreams[idx].streamname << endl;

  if (datastreams[idx].sType == "HDF") {
    datastreams[idx].dataStream = new HDFFile(datastreams[idx].streamname);
  } else if (datastreams[idx].sType == "ASCII") {
    datastreams[idx].dataStream = new AsciiStream(datastreams[idx].streamname);
  }
#ifdef GRIBSTREAM
  else if (datastreams[idx].sType == "GRIB") {
    datastreams[idx].dataStream =
        new GribStream(datastreams[idx].streamname);
  }
#endif
  else if (datastreams[idx].sType == "MORA") {
    openMoraStream();
    datastreams[idx].dataStream = moraStream;
  }

  if (datastreams[idx].sType != "CUSTOMER") {
    if (datastreams[idx].sType != "MORA") 
      datastreams[idx].mtime = _modtime(datastreams[idx].streamname);
    else
      datastreams[idx].mtime = time(NULL);
    if (datastreams[idx].dataStream)
      datastreams[idx].streamOpen = datastreams[idx].dataStream->openStream(
          &ef);
    if (!datastreams[idx].streamOpen) {
      // error message
      cerr << "ERROR Datafilecollection: could not open stream: "
          << datastreams[idx].streamname << endl;
      return false;
    } else {
      // get model list from file
      int numm = 0;
      while (numm < MAXMODELSINSTREAM) {
        std::vector<std::string> dt(datastreams[idx].txtList[numm].begin(), datastreams[idx].txtList[numm].end());
        if (not datastreams[idx].dataStream->getModelSeq(numm,
            datastreams[idx].modelList[numm], datastreams[idx].runList[numm],
            datastreams[idx].idList[numm], dt))
          break;
        numm++;
      }
      datastreams[idx].numModels = numm;
#ifdef DEBUG
      cout << "FILECOLLECTION: numModels:"<<datastreams[idx].numModels<<endl;
      for (int k=0;k<datastreams[idx].numModels;k++)
        cout << "Model:"<<datastreams[idx].modelList[k]<<
        " Run:"<<datastreams[idx].runList[k]<<
        " Id:"<<datastreams[idx].idList[k]<<endl;
#endif
    }
  }
  streams_opened = true;
  return true;
}

bool DatafileColl::openStreams()
{
  bool ok = true;
  //ErrorFlag ef=OK;

  if (verbose)
    cout << "- Open streams.." << endl;
  for (unsigned int i = 0; i < datastreams.size(); i++) {
    openStream(i);
  }
  makeStationList();
  return ok;
}

void DatafileColl::closeStreams()
{
  for (unsigned int i = 0; i < datastreams.size(); i++) {
    if (datastreams[i].dataStream && datastreams[i].streamOpen) {
      datastreams[i].streamOpen = false;
      if (datastreams[i].sType == "MORA") {
        closeMoraStream();
      } else {
        delete datastreams[i].dataStream;
      }
    }
  }

  for (unsigned int i = 0; i < fimexStreams.size(); i++) {
    if (fimexStreams[i].dataStream ) {
      delete fimexStreams[i].dataStream;
      fimexStreams[i].dataStream=NULL;
    }
  }
}

bool DatafileColl::_isafile(const std::string& name)
{
  FILE *fp;
  if ((fp = fopen(name.c_str(), "r"))) {
    fclose(fp);
    return true;
  } else
    return false;
}

unsigned long DatafileColl::_modtime(std::string& fname)
{
  struct stat filestat;
  // first check if fname is a proper file
  if (_isafile(fname)) {
    _filestat(fname, filestat);
    return (unsigned long) filestat.st_mtime;
  } else
    return 1;
}

void DatafileColl::_filestat(std::string& fname, struct stat& filestat)
{
  stat(fname.c_str(), &filestat);
}

bool DatafileColl::check(vector<int>& idx)
{
  unsigned long mtime;
  bool changed = false;
  idx.clear();
  if (datastreams.size() > 0) {
    for (unsigned int i = 0; i < datastreams.size(); i++) {
      if (!datastreams[i].streamOpen)
        continue;
      if (datastreams[i].sType != "MORA") 
        mtime = _modtime(datastreams[i].streamname);
      else
        mtime = time(NULL);
      if (mtime > datastreams[i].mtime) {
        changed = true;
        idx.push_back(i);
      }
    }
  }
  return changed;
}

void DatafileColl::makeStationList()
{
  int nums, posidx, ns = 0;
  bool exists;
  miPosition st;
  vector<ExtStation>::iterator p;

  if (verbose)
    cout << "- Reading positions from streams.." << endl;
  stations.clear();
  pos_info.clear();
  for (unsigned int i = 0; i < datasetname.size(); i++)
    numStationsDS[i] = 0;

  unsigned int n = datastreams.size();
  if (n > 0) {
    for (unsigned int i = 0; i < n; i++) {
      if (datastreams[i].dataStream && datastreams[i].streamOpen) {
        nums = 0;
        while (datastreams[i].dataStream->getStationSeq(nums, st)) {
          // force upcase on all stations
          std::string uppername = miutil::to_upper_latin1(st.Name());
          st.setName(uppername);

          // Check if station already exists
          exists = findpos(st.Name(), posidx);
          if (posidx == ns)
            p = stations.end();
          else
            p = stations.begin() + posidx;

          if (!exists) {
            ExtStation estat;
            estat.station = st;
            estat.priority = 2;
            stations.insert(p, estat);
            pos_info[st.Name()] = st;
            p = stations.begin() + posidx;
            ns++;
          } else {
            // if incoming station has a valid dbkey...keep it
            if (st.DbKey() != 0 && p->station.DbKey() == 0)
              p->station = st;
          }
          // update number of stations in each dataset
          if (!(p->d.setdata(datastreams[i].dataSet)))
            numStationsDS[datastreams[i].dataSet]++;
          nums++;
        } // while getStationSeq
      } // if file open
    } // for numdatastreams
  } // if numdatastreams > 0
}

bool DatafileColl::getStreamInfo(int idx, std::string& name, std::string& desc,
    int& size, int& dset, int& nindset)
{
  struct stat fstat;
  if (idx >= 0 && (idx < (signed int) datastreams.size())) {
    name = datastreams[idx].streamname;
    desc = datastreams[idx].descript;
    dset = datastreams[idx].dataSet;
    nindset = datastreams[idx].numindset;
    if (_isafile(name)) {
      _filestat(name, fstat);
      size = fstat.st_size;
    } else
      size = 50000; // ..good as any
    return true;
  } else
    return false;
}

DataStream* DatafileColl::getDataStream(int idx)
{
  if ((idx >= 0) && (idx < (signed int) datastreams.size())) {
    bool b = true;
    if (!datastreams[idx].dataStream || !datastreams[idx].streamOpen) {
      b = openStream(idx);
    }
    if (b)
      return datastreams[idx].dataStream;
  }
  return 0;
}

// return number of positions in dataset dset.
// if dset < 0, return total number of positions in collection.
int DatafileColl::getNumPositions(int dset)
{
  if (dset < 0)
    return stations.size();
  else if (dset < (signed int) datasetname.size())
    return numStationsDS[dset];
  else
    return 0;
}

miPosition DatafileColl::getPositionInfo(std::string name)
{
  if (pos_info.count(name))
    return pos_info[name];
  miPosition empty;
  return empty;
}

bool DatafileColl::getPosition(int dset, int &idx, std::string& name, int &id,
    float& lat, float& lng, int &prio)
{
  int n = stations.size();
  if ((idx < 0) || (idx >= n))
    return false;

  if (dset >= 0) {
    if (dset >= (signed int) datasetname.size())
      return false;
    while (!(stations[idx].d.isdata(dset)) && (idx < n))
      idx++;
    if (!(stations[idx].d.isdata(dset)))
      return false;
  }

  name = stations[idx].station.Name();
  id = idx;
  lat = stations[idx].station.lat();
  lng = stations[idx].station.lon();
  prio = stations[idx].priority;
  idx++;
  return true;
}

bool DatafileColl::getPosition(int dset, int &idx, ExtStation** es)
{
  int n = stations.size();
  if ((idx < 0) || (idx >= n))
    return false;

  if (dset >= 0) {
    if (dset >= (signed int) datasetname.size())
      return false;

    while (!(stations[idx].d.isdata(dset)) && (idx < n))
      idx++;
    if (idx >= n)
      return false;
  }

  *es = &(stations[idx]);
  idx++;
  return true;
}

map<std::string, miCoordinates> DatafileColl::getPositions(const std::string mod)
{
  map<std::string, miCoordinates> result;

  dataset ds;

  int i, j, n = datastreams.size();

  for (i = 0; i < n; i++)
    for (j = 0; j < datastreams[i].numModels; j++) {
      if (mod == datastreams[i].modelList[j]) {
        ds.setdata(datastreams[i].dataSet);
      }
    }
  n = stations.size();
  for (i = 0; i < n; i++) {
    if (Union(ds, stations[i].d)) {
      result[stations[i].station.Name()] = stations[i].station.Coordinates();
    }
  }
  return result;
}

// Get list of indices for files which contain data for a
// specific model and run (returned in idx).
// Returnvalue is number of files found
int DatafileColl::findModel(const Model& mid, const Run& rid, int* idx, int max)
{
  int numi = 0;
  int n = datastreams.size();
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < datastreams[i].numModels; j++) {
      if ((mid == datastreams[i].modelList[j] || mid == M_UNDEF)
          && (rid == datastreams[i].runList[j] || rid == R_UNDEF
              || datastreams[i].runList[j] == R_UNDEF)) {
        if (numi < max) {
          idx[numi++] = i;
        }
      }
    }
  }
  return numi;
}

vector<std::string> DatafileColl::findRuns(const Model& mid)
{
  Run rid;
  vector<std::string> vrid;
  set<int> srid;
  int n = datastreams.size();
  for (int i = 0; i < n; i++)
    for (int j = 0; j < datastreams[i].numModels; j++) {
      if (mid == datastreams[i].modelList[j]) {
        rid = datastreams[i].runList[j];
        srid.insert(rid);
      }
    }
  set<int>::iterator itr = srid.begin();
  for (; itr != srid.end(); itr++)
    vrid.push_back(miutil::from_number(*itr));

  return vrid;
}

// Binary search for position by name
bool DatafileColl::findpos(const std::string& name, int& idx)
{
  std::string pname;
  int n = stations.size(), min = 0, max = n - 1, p;
  while (!(max < min)) {
    p = (min + max) / 2;
    pname = stations[p].station.Name();
    if (name == pname) {
      idx = p;
      return true;
    }
    if (name < pname)
      max = p - 1;
    else
      min = p + 1;
  }

  // pos not found, increase index by one for frontal insertion
  idx = max + 1;
  return false;
}

/////// Klima database -----------------------------

void DatafileColl::openKlimaStream()
{
  tsSetup setup;
  if (klimaStream == NULL)
    klimaStream = new pets::KlimaStream(setup.klima.url, setup.klima.parameters,
      setup.klima.normals,setup.klima.maxDistance);
}

void DatafileColl::closeKlimaStream()
{
  try {
    if (klimaStream != NULL)
      delete klimaStream;
  } catch (exception& e) {
    cerr << " Exception caught while trying to delete klimaStream " << e.what()
                        << endl;
  }
  klimaStream = NULL;
}

/////// SMHI mora database -----------------------------

void DatafileColl::openMoraStream()
{
  tsSetup setup;
  if (moraStream == NULL)
    moraStream = new pets::MoraStream(setup.mora.url, setup.mora.stationreport,
      setup.mora.datareport, setup.mora.monthlynormalreport, setup.mora.parameters,
      setup.mora.normals, setup.mora.maxDistance);
}

void DatafileColl::closeMoraStream()
{
  try {
    if (moraStream != NULL)
      delete moraStream;
  } catch (exception& e) {
    cerr << " Exception caught while trying to delete klimaStream " << e.what()
                        << endl;
  }
  moraStream = NULL;
}

/////// WDB ------------------------------------------

void DatafileColl::openWdbStream()
{
  wdbStreamIsOpen=false;
  try {

    tsSetup setup;

    wdbStream = new pets::WdbStream(setup.wdb.host, setup.wdb.parameters,
        setup.wdb.vectorFunctions, setup.wdb.user);

    set<string> providers = wdbStream->getDataProviders();
    wdbStreamIsOpen = !providers.empty();

  } catch (exception& e) {
    cerr << " Exception caught while trying to open WdbStream " << e.what()
                        << endl;
  }
}

void DatafileColl::closeWdbStream()
{
  try {
    delete wdbStream;
  } catch (exception& e) {
    cerr << " Exception caught while trying to delete WdbStream " << e.what()
                        << endl;
  }
  wdbStream = NULL;
}

set<string> DatafileColl::getWdbProviders()
{
  set<string> providers;
  try {
    providers = wdbStream->getDataProviders();
  } catch (exception& e) {
    cerr << "Exception in getWdbProviders(): " << e.what() << endl;
  }
  return providers;
}

set<miTime> DatafileColl::getWdbReferenceTimes(string provider)
{
  set<miTime> referenceTimes;
  try {

    wdbStream->setCurrentProvider(provider);
    referenceTimes = wdbStream->getReferenceTimes();

  } catch (exception& e) {
    cerr << "Exception in getWdbReferenceTimes(): " << e.what() << endl;
  }
  return referenceTimes;
}

pets::WdbStream::BoundaryBox DatafileColl::getWdbGeometry()
{
  pets::WdbStream::BoundaryBox boundaries;
  try {

    boundaries = wdbStream->getGeometry();
  } catch (exception& e) {
    cerr << "Exception in getGeometry(): " << e.what() << endl;
  }

  return boundaries;
}

/////// FIMEX ------------------------------------------

void DatafileColl::initialiseFimexPositions()
{
  pets::FimexPoslist fimexpositions;
  tsSetup setup;

  try {
    fimexpositions.addFile(setup.files.commonBookmarks);
  } catch (exception &e){
    cerr << "Exception caught from fimex position initialization: " << e.what() << endl;
  }

  try {
    fimexpositions.addFile(setup.files.fimexBookmarks);
  } catch (exception &e){
    cerr << "Exception caught from fimex position initialization: " << e.what() << endl;
  }

  pets::FimexStream::setCommonPoslist(fimexpositions);
}

void DatafileColl::initialiseFimexParameters()
{
  tsSetup setup;
  vector<pets::FimexParameter> fimexparameter;
  for(unsigned int i=0;i< setup.fimex.parameters.size();i++) {
    pets::FimexParameter par;
    try {
      par.setFromString(setup.fimex.parameters[i]);
      fimexparameter.push_back(par);
    } catch ( exception& e) {
      cerr << e.what() << endl;
    }
  }
  cerr << "Setting " << fimexparameter.size() << " Parameters to fimex" << endl;
  pets::FimexStream::setFimexParameters(fimexparameter);
  pets::FimexStream::addToAllParameters(setup.fimex.filters);
}


vector<std::string> DatafileColl::getFimexTimes(const std::string& model)
{
  vector<std::string> runtimes;
  cerr << "Indexing Model: " << model << endl;

  for (std::vector<FimexInfo>::iterator it = fimexStreams.begin(); it != fimexStreams.end(); ++it) {
    if (model == it->model) {
      cerr << "Indexing file " << it->streamname << endl;

      if (it->run.empty()) {

        try {
            miTime refTime;
            int index = -1;
            // Find actual timefilter
            for(size_t j=0; j<fimexFileindex.size(); j++) {
              if (model == fimexFileindex[j].model) {
                index = j;
                break;
              }
            }
            // Get refTime from filename..
            if (index != -1 && fimexFileindex[index].tf.getTime(it->streamname,refTime)) {
              it->run = refTime.isoTime(true,  false);
              fimex_streams_opened = true;
            } else {
              // Get from the stream...
              cerr << "Unable to get referencetime from streamname - opening it : " << it->streamname  << endl;
              boost::posix_time::ptime time = it->dataStream->getReferencetime();
              //std::string stime = boost::posix_time::to_simple_string(time);
              ostringstream stime;

              boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M");
              stime.imbue(locale(stime.getloc(), facet));
              stime << time;
              it->run = stime.str();
            }
          } catch (exception & e) {
            cerr << e.what() << endl;
            cerr << "using streamname instead of runtime to index model" << endl;
            string streamname =  it->streamname;
            string::size_type n = streamname.rfind("/");
            string::size_type p = streamname.rfind(".");

             if( p>n && p <streamname.size())
               streamname.erase(p,streamname.size()-p);

             if(n>0 && n < streamname.size() )
               streamname.erase(0,n+1);

             it->run = streamname;

          }

          if (it->dataStream->isOpen())
            fimex_streams_opened=true;

        }
        runtimes.push_back(it->run);

    }
  }

  return runtimes;
}


pets::FimexStream* DatafileColl::getFimexStream(const std::string& model, const std::string& run)
{
  for (std::vector<FimexInfo>::iterator it = fimexStreams.begin(); it != fimexStreams.end(); ++it) {
    if (model == it->model) {
      if (it->run.empty()) {
        getFimexTimes(model);
      }
      if (run == it->run) {
        return it->dataStream;
      }
    }
  }

  ostringstream ost;
  ost << "stream for " << model << " at " << run << " does not exist";
  throw pets::FimexstreamException(ost.str());
}


bool DatafileColl::updateFimexStreams(std::string currentModel)
{
  if(!fimex_streams_opened)
    return false;

  bool currentModelUpdated=false;
  // Just if the model matches the current model, we have to update
  // the active runtimes list in the sidebar...
  for(unsigned int i=0; i<fimexFileindex.size(); i++) {
    if(createFimexStreams(fimexFileindex[i])) {
      if(fimexFileindex[i].model == currentModel)
       currentModelUpdated = true;
    }
  }
  return currentModelUpdated;
}




bool DatafileColl::createFimexStreams(FimexFileindex& findex)
{

  FimexInfo finfo;
  finfo.model      = findex.model;
  finfo.sType      = findex.sType;
  finfo.configfile = findex.configfile;
  vector<string> streamfilenames = findex.findNewFiles();

  for( unsigned int i=0; i< streamfilenames.size();i++) {
    finfo.streamname = streamfilenames[i];
    cerr << "Added to fimexstreams" << finfo.streamname << " as " << finfo.sType << endl;
    fimexStreams.push_back(finfo);

    fimexStreams.back().dataStream = new pets::FimexStream(finfo.streamname, finfo.model, finfo.sType, finfo.configfile);
  }
  return bool ( streamfilenames.size());
}
