#include "WdbBookmarkTools.h"
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "directory.xpm"
#include "media-record.xpm"
#include "metno.xpm"
#include "trashcan.xpm"
#include "search_folder.xpm"
#include <QStringList>


using namespace std; 

WdbBookmarkTools::WdbBookmarkTools()
{
  maxRecords=30;
  record=0;
  folders.clear();
  directoryIcon.addPixmap(QPixmap(directory_xpm));
  recordIcon.addPixmap(QPixmap(media_record_xpm));
  metnoIcon.addPixmap(QPixmap(metno_xpm));
  trashIcon.addPixmap(QPixmap(trashcan_xpm));
  searchIcon.addPixmap(QPixmap(search_folder_xpm));


}

bool WdbBookmarkTools::read(std::string filename,bool ignoreFromSave)
{
  ifstream in(filename.c_str());
  if(!in ) {
    cerr << "Bookmarkfile " << filename << " does not exist - skipping " << endl;
    return false;
  }
  while(in) {
    string line;
    getline(in,line);
    string::size_type c = line.find_first_of("#",0);
    if(c != string::npos && c < line.length()) {
      int k=line.length() -  c;
      line.erase(c,k);
    }
    boost::algorithm::trim(line);
    if(!line.empty())
      addLine(line,ignoreFromSave);
  }
  return true;
}

void WdbBookmarkTools::addLine(string line,bool ignoreFromSave, bool reverse)
{
  vector<string> words;
  boost::split( words, line, boost::is_any_of("|") );
  if(words.size() < 2 ) return;
  string data  = words[1];
  string token = words[0];
  string folder;

  boost::split( words,token, boost::is_any_of(".") );

  QStandardItem *parentItem = model->invisibleRootItem();

  int size = words.size();
  int last = size-1;


  for (int col = 0; col < size; ++col) {
    // this is the item
    if(col == last) {
      int r=(reverse ? 0 : parentItem->rowCount());
      QStandardItem *childItem  = new QStandardItem(words[col].c_str());
      childItem->setData(data.c_str());
      childItem->setDropEnabled(false);
      parentItem->insertRow(r,childItem);

    } else {
      // old folder found - use it as the parent
      folder+=( col ? "." : "") + words[col];
      if(folders.count(folder)) {
        parentItem = model->itemFromIndex ( folders[folder] );

      } else {
        // create a new folder
        QStandardItem *childItem = createFolder(words[col],ignoreFromSave);

        parentItem->appendRow(childItem);
        parentItem      = childItem;
        folders[folder] = childItem->index();
        if(ignoreFromSave){
          ignores.insert(folder);
        }
      }
    }
  }
}

QStandardItem * WdbBookmarkTools::createFolder(string folder,bool ignoreFromSave)
{
  QStandardItem *childItem = new QStandardItem(folder.c_str());

  if(folder=="RECORD") {
    childItem->setIcon(recordIcon);
  } else if(folder=="SEARCH") {
      childItem->setIcon(searchIcon);
  } else if ( folder =="TRASH") {
    childItem->setIcon(trashIcon);
  } else {

    if(ignoreFromSave)
      childItem->setIcon(metnoIcon);
    else
      childItem->setIcon(directoryIcon);
  }

  if(ignoreFromSave){
    childItem->setEditable(false);
  }
  childItem->setDragEnabled(false);


  return childItem;
}



void WdbBookmarkTools::addFolder(string folder,bool ignoreFromSave)
{
  if(folders.count(folder))
    return;

  QStandardItem *parentItem = model->invisibleRootItem();
  QStandardItem *childItem  = createFolder(folder,ignoreFromSave);

  parentItem->appendRow(childItem);
  folders[folder] = childItem->index();

  if(ignoreFromSave) {
    ignores.insert(folder);
  }
}


void WdbBookmarkTools::write(string filename)
{

  cerr << "writing bookmarks to " << filename << endl;
  ofstream out(filename.c_str());
  if(!out) {
    cerr << "Unable to write stationlist to " << filename << endl;
    return;
  }


  QStandardItem *parentItem = model->invisibleRootItem();
  if(!parentItem->hasChildren()) return;

  for(int i=0;i<parentItem->rowCount();i++) {
    QStandardItem* item = parentItem->child(i);
    if(!item->hasChildren()) continue;
    string dirname= item->text().toStdString();
    if(ignores.count(dirname)) continue;

    for(int i=0;i<item->rowCount();i++) {
      QStandardItem* child = item->child(i);
      QVariant var         = child->data();
      QString  coor        = var.toString();
      QString  name        = child->text();

      if(name.isEmpty() || coor.isEmpty()) continue;
      out << dirname << "." << name.toStdString() << "|" << coor.toStdString() << endl;
    }
  }

}



std::string WdbBookmarkTools::createRecordName(float f,char pos, char neg)
{
  float fdeg = f;
  int   deg = int(fdeg);
  fdeg-=deg;
  fdeg*=60;
  int min = int(fdeg);

  ostringstream ost;
  ost << abs(deg) << "° " << abs(min) << "\' " << ( deg >=0 ? pos : neg );
  return ost.str();
}




void WdbBookmarkTools::addRecord(float lon,float lat,std::string name)
{

  ostringstream ost;
  ost << "RECORD.";
  if(name.empty()) {
    ost << createRecordName(lon,'E','W') << " " << createRecordName(lat,'N','S');
  } else
    ost << name;


  ost << "|" << lat << ":" << lon;
  addLine(ost.str(),false,true);
  cutRecord();
}

void WdbBookmarkTools::addSearch(std::string searchPos)
{
    ostringstream ost;
    ost << "SEARCH." << searchPos;

    addLine(ost.str(),false,true);
    cerr << "ADDING:   " << ost.str() << endl;
   cutSearch();
}


void WdbBookmarkTools::cutRecord()
{
  if(!folders.count("RECORD")      ) return;

  QStandardItem* item = model->itemFromIndex (folders ["RECORD"]);

  if(!item                          ) return;
  if(!item->hasChildren()           ) return;
  if( item->rowCount() < maxRecords ) return;

  item->setRowCount(maxRecords);

}

void WdbBookmarkTools::cutSearch()
{
  if(!folders.count("SEARCH")      ) return;

  QStandardItem* item = model->itemFromIndex (folders ["SEARCH"]);

  if(!item                          ) return;
  if(!item->hasChildren()           ) return;
  if( item->rowCount() < maxRecords ) return;

  item->setRowCount(maxRecords);

}




std::vector<std::string> WdbBookmarkTools::getAllBookmarks()
{
  std::vector<std::string> allBookmarks;


  QStandardItem *parentItem = model->invisibleRootItem();
  if(parentItem->hasChildren()) {

    for(int i=0;i<parentItem->rowCount();i++) {
      QStandardItem* item = parentItem->child(i);
      if(!item->hasChildren()) continue;
      string dirname= item->text().toStdString();
      if(dirname=="TRASH") continue;

      for(int i=0;i<item->rowCount();i++) {
        string newbookmark = stringFromItem(item->child(i));

        if(newbookmark.empty()) continue;
        allBookmarks.push_back(newbookmark);
      }
    }
  }
  return allBookmarks;

}



void WdbBookmarkTools::copySelected(QModelIndexList indexlist)
{
  buffer.clear();
  for (int i=0; i<indexlist.size(); i++) {

    QModelIndex index = indexlist.at(i);
    QStandardItem* child = model->itemFromIndex(index);

    string itemstr = stringFromItem(child);

    if(!itemstr.empty())
      buffer.push_back(itemstr);

  }
}

void WdbBookmarkTools::removeSelected(QModelIndexList indexlist)
{
  if(indexlist.isEmpty())
    return;

  QModelIndexList::iterator itr=indexlist.end();

  while(1) {
    itr--;
    QModelIndex index = *itr;
    QStandardItem* child = model->itemFromIndex(index);
    if(child) {
      if(child->isDragEnabled()) {
        QStandardItem* parent = child->parent();
        int row = child->row();
        parent->removeRow(row);
      }
    }
    if(itr==indexlist.begin())
      break;
  }
}


void WdbBookmarkTools::paste(QModelIndex index)
{
  QStandardItem* child = model->itemFromIndex(index);
  QStandardItem* parent;
  int row=0;
  if(child) {
    if(!child->isDragEnabled()) {
      parent=child;
    } else {
      parent = child->parent();
      row = child->row();
    }

    for(size_t i=0; i<buffer.size(); i++) {
      QStandardItem* newItem= itemFromString(buffer[i]);
      parent->insertRow(row+i,newItem);
    }
  }
}

QStandardItem* WdbBookmarkTools::itemFromString(std::string line)
{
  vector<string> words;
  boost::split( words, line, boost::is_any_of("|") );
  if(words.size() < 2 ) return 0;
  string data  = words[1];
  string token = words[0];

  QStandardItem *item  = new QStandardItem(token.c_str());
  item->setData(data.c_str());
  item->setDropEnabled(false);
  return item;
}

std::string WdbBookmarkTools::stringFromItem(QStandardItem* item)
{
  QVariant var         = item->data();
  QString  coor        = var.toString();
  QString  name        = item->text();
  ostringstream ost;
  if(name.isEmpty() || coor.isEmpty()) return "";
  ost << name.toStdString() << "|" << coor.toStdString();
  return ost.str();
}
