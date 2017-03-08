/*
 * PrepareDataThread.cc
 *
 *  Created on: Sep 19, 2013
 *      Author: juergens
 */


#include "PrepareDataThread.h"
#include <QMutex>
#include <exception>

using namespace std;
namespace pets {

void PrepareDataThread::setFimexParameters(pets::FimexStream* f, std::string pl, float lat,
    float lon,  std::vector<ParId>& ip, std::vector<ParId>& op)
{
  fimex= f;
  stationname=pl;
  latitude=lat;
  longitude=lon;
  inpar=ip;
  outpar=op;

}


void PrepareDataThread::run()
{
  cerr << "read thread started" << endl;
  QMutex mutex;
  mutex.lock();
  bool read_success=false;

  try {
      if(fimex->readData(stationname,latitude,longitude,inpar,outpar))
        read_success=true;

   } catch(std::exception& e) {
     std::cerr << "FIMEX::READDATA FAILED: " << e.what() << std::endl;
   }

   cerr << "read thread finished " << endl;
   mutex.unlock();
  emit post_dataLoad(read_success);
}


} /* namespace pets */
