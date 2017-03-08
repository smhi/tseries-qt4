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
#ifndef _tsConfigure_h
#define _tsConfigure_h

#include <string>

#include <vector>
#include <map>


class tsConfigure {

public:

  bool read(const std::string&,std::string="");

  bool get(const std::string& key, std::string& cont);
  bool get(const std::string& key, float& cont);
  bool get(const std::string& key, bool& cont);
  bool get(const std::string& key, int& cont);

  void set(std::string key, const std::string token);
  void set(std::string key, const float token);
  void set(std::string key, const bool token);
  void set(std::string key, const int token);

  std::vector<std::string> getCustoms();
  std::vector<std::string> getList(std::string);

  bool save(std::string);

private:

  struct tsCustoms {
    std::string name;
    std::vector<std::string> list;
  };

  static std::map<std::string,std::string> contents;
  static std::vector<tsCustoms> custom;


  enum { PUBLIC,STYLE,LIST} sec;

  int its,itl;

  void fetchSection(std::string);
  void setSimpleToken(std::string);
  void stripComments(std::string&);

  bool splitToken(const std::string&,std::string&, std::string&);

  void setToken(std::string);
  void setStyle(std::string&, std::string&);
  void setList(std::string);
  void setDefaults();

};

#endif









