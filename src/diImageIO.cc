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

#include "diImageIO.h"

#include <puTools/miStringFunctions.h>

#include <png.h>

#include <iostream>
#include <fstream>
#include <map>

using namespace std;

bool imageIO::read_image(Image_data& img)
{
  if (miutil::contains(img.filename, ".png"))
    return read_png(img);
  else if (miutil::contains(img.filename, ".xpm"))
    return read_xpm(img);

  return false;
}




/*
  PNG routines
*/


bool imageIO::read_png(Image_data& img){
  cerr << "--------- read_png: " << img.filename << endl;

  FILE *fp = fopen(img.filename.c_str(), "rb");
  if (!fp){
    cerr << "read_png ERROR can't open file:" << img.filename << endl;
    return false;
  }

  png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING,
     (png_voidp)0,//user_error_ptr,
     0,//user_error_fn,
     0);//user_warning_fn);
  if (!png_ptr){
    cerr << "read_png ERROR creating png_struct" << endl;
    fclose(fp);
    return false;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr){
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    cerr << "read_png ERROR creating info_struct" << endl;
    fclose(fp);
    return false;
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info){
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    cerr << "read_png ERROR creating end_info_struct" << endl;
    fclose(fp);
    return false;
  }
#ifdef linux
  if (setjmp(png_jmpbuf(png_ptr))) {
#else
  if (setjmp(png_ptr->jmpbuf)) {
#endif
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    cerr << "read_png ERROR longjmp out of process" << endl;
    fclose(fp);
    return false;
  }

  png_init_io(png_ptr, fp);


  // do the read
  const int png_transforms = 0;
  png_read_png(png_ptr, info_ptr, png_transforms, NULL);

  png_uint_32 uwidth, uheight;
  int color_type;
  int bit_depth;
  int interlace_type;//=   PNG_INTERLACE_NONE;
  int compression_type;//= PNG_COMPRESSION_TYPE_DEFAULT;
  int filter_type;//=      PNG_FILTER_TYPE_DEFAULT;

  png_get_IHDR(png_ptr, info_ptr, &uwidth, &uheight,
      &bit_depth, &color_type, &interlace_type,
      &compression_type, &filter_type);
  img.width= uwidth;
  img.height= uheight;

  //   cerr << "image width:" << img.width << endl;
  //   cerr << "image height:" << img.height << endl;
  //   cerr << "image bit_depth:" << bit_depth << endl;
  //   cerr << "image color_type:" << color_type << endl;

  img.nchannels=4;
  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    img.nchannels= 4;
  else if (color_type == PNG_COLOR_TYPE_RGB)
    img.nchannels= 3;
  else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    img.nchannels= 4;
  else if (color_type == PNG_COLOR_TYPE_GRAY)
    img.nchannels= 3;
  else if (color_type == PNG_COLOR_TYPE_PALETTE){
    cerr << "PNG_COLOR_TYPE_PALETTE"
         << " ..exiting" << endl;
    return false;
  } else {
    cerr << "Unknown color_type:" << color_type
         << " ..exiting" << endl;
    return false;
  }

  //   cerr << "image nchannels:" << img.nchannels << endl;


  png_bytep *row_pointers;
  row_pointers = png_get_rows(png_ptr, info_ptr);

  //png_read_image(png_ptr, row_pointers);
  //png_read_end(png_ptr, end_info);

  // unpack image from row-based structure
  img.data= new unsigned char [img.width*img.height*img.nchannels];
  int bp=0;
  for (int i=img.height-1; i>=0; i--){
    for (int j=0; j<img.width*img.nchannels; j++,bp++){
      img.data[bp] = row_pointers[i][j];
    }
  }

  // clean up
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  return true;
}



bool imageIO::write_png(const Image_data& img){
  cerr << "--------- write_png: " << img.filename << endl;

  // open file for write
  FILE *fp = fopen(img.filename.c_str(), "wb");
  if (!fp) {
    cerr << "write_png ERROR can't open file:" << img.filename << endl;
    return false;
  }

  // create png struct (private)
  png_structp png_ptr = png_create_write_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)0, 0, 0);

  if (!png_ptr){
    cerr << "write_png ERROR creating png_struct" << endl;
    fclose(fp);
    return false;
  }

  // create info struct
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    cerr << "write_png ERROR creating info_struct" << endl;
    fclose(fp);
    return false;
  }
#ifdef linux
  if (setjmp(png_jmpbuf(png_ptr))) {
#else
  if (setjmp(png_ptr->jmpbuf)) {
#endif
    png_destroy_write_struct(&png_ptr, &info_ptr);
    cerr << "write_png ERROR longjmp out of process" << endl;
    fclose(fp);
    return false;
  }

  png_init_io(png_ptr, fp);

  const int color_type=       (img.nchannels==4 ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB);
  const int bit_depth=        8;
#ifdef linux
  const int interlace_type=   PNG_INTERLACE_NONE;
  const int compression_type= PNG_COMPRESSION_TYPE_DEFAULT;
  const int filter_type=      PNG_FILTER_TYPE_DEFAULT;
#else
  const int interlace_type=   0;
  const int compression_type= 0;
  const int filter_type=      0;
#endif

  // set all info
  png_set_IHDR(png_ptr, info_ptr, img.width, img.height,
      bit_depth, color_type, interlace_type,
      compression_type, filter_type);

  // write info to file
  png_write_info(png_ptr, info_ptr);

  // pack image into row-based structure
  png_byte **row_pointers;
  row_pointers= new png_byte*[img.height];
  int bp=0;
  for (int i=img.height-1; i>=0; i--){
    row_pointers[i]= new png_byte[img.width*img.nchannels];
    for (int j=0; j<img.width*img.nchannels; j++,bp++){
      row_pointers[i][j]= img.data[bp];
    }
  }

  // write image to file
  png_write_image(png_ptr, row_pointers);

  // write any trailing info-data
  png_write_end(png_ptr, info_ptr);

  // clean up
  png_destroy_write_struct(&png_ptr, &info_ptr);

  for (int i=0; i<img.height; i++)
    delete[] row_pointers[i];
  delete[] row_pointers;

  fclose(fp);
  return true;
}




/*
  XPM - routines
*/


// convert a HEX-character (0-9,A-F) to int
int chartoint__(const char c)
{
  const int zv= int('0');
  const int nv= int('9');
  const int av= int('A');
  const int fv= int('F');

  int v= int(c);

  if (v >= zv && v<= nv)
    return v-zv;
  else if (v >= av && v <= fv)
    return v-av+10;

  // illegal character
  return 0;
}

int hexToInt__(const std::string& p){
  int l= p.length(), res=0, fact=1;
  for (int i=l-1; i>=0; i--,fact*=15)
    res += chartoint__(p[i])*fact;

  return res;
}


bool imageIO::imageFromXpmdata(const char** xd, Image_data& img){
  int xsize=-1,ysize,ncols,nchar;
  vector<std::string> vs;
  std::string buf= xd[0];

  vs= miutil::split(buf, " ");
  if (vs.size() < 4){
    cerr << "imageFromXpmdata ERROR too few elements:" << buf << endl;
    return false;
  }
  xsize= atoi(vs[0].c_str());
  ysize= atoi(vs[1].c_str());
  ncols= atoi(vs[2].c_str());
  nchar= atoi(vs[3].c_str());

  if (xsize < 1 || ysize < 1 || ncols < 1 || nchar < 1){
    cerr << "imageFromXpmdata ERROR Illegal numbers "
         << " xsize:" << xsize << " ysize:" << ysize
         << " ncols:" << ncols << " nchar:" << nchar
         << endl;
    return false;
  }

  map<std::string,int> redmap;
  map<std::string,int> greenmap;
  map<std::string,int> bluemap;
  map<std::string,int> alphamap;

  for (int i=0; i<ncols; i++){
    buf = xd[1+i];
    int j= buf.find_last_of("c");
    if (j < 0){
      cerr << "imageFromXpmdata ERROR Illegal colourdefinition:"
           << buf << endl;
      return false;
    }
    std::string key=    buf.substr(0,nchar);
    std::string colour= buf.substr(j+1,buf.length()-j-1);
    miutil::trim(colour);
    if (colour == "None"){
      redmap[key]  = 255;
      greenmap[key]= 255;
      bluemap[key] = 255;
      alphamap[key]= 0;
    } else {
      if (colour.size() < 2){
        cerr << "imageFromXpmdata ERROR Illegal colourdefinition:"
             << buf << endl;
        return false;
      }
      colour= colour.substr(1,colour.length()-1);
      int numcomp = colour.length()/3;
      redmap[key]  = hexToInt__(colour.substr(0,numcomp));
      greenmap[key]= hexToInt__(colour.substr(numcomp,numcomp));
      bluemap[key] = hexToInt__(colour.substr(numcomp*2,numcomp));
      alphamap[key]= 255;
    }
  }

  // data
  img.width = xsize;
  img.height= ysize;
  img.nchannels= 4;
  img.data= new unsigned char [img.width*img.height*img.nchannels];
  int pp= 0;
  for (int y=ysize-1; y>=0; y--){
    std::string line= xd[y+ncols+1];
    for (int x=0; x<xsize*nchar; x+=nchar){
      std::string pixel= line.substr(x,nchar);
      img.data[pp+0]= redmap[pixel];
      img.data[pp+1]= greenmap[pixel];
      img.data[pp+2]= bluemap[pixel];
      img.data[pp+3]= alphamap[pixel];
      pp+= 4;
    }
  }

  return true;
}




bool imageIO::read_xpm(Image_data& img){
  cerr << "--------- read_xpm: " << img.filename << endl;

  ifstream file(img.filename.c_str());

  if (!file){
    cerr << "readXpmFile ERROR: Unable to open file:"
         << img.filename << endl;
    return false;
  }

  std::string buf;
  vector<std::string> vs, vs2;

  while(getline(file,buf)){
    miutil::trim(buf);
    if (buf.length() == 0)
      continue;
    if (buf[0]!='\"')
      continue;
    int i= buf.find_last_of("\"");
    buf= buf.substr(1,i-1);
    vs.push_back(buf);
  }
  if (vs.size() == 0)
    return false;

  //   cerr << "RESULTING DATA:" << endl;
  char **data = new char*[vs.size()];
  for (unsigned int i=0; i<vs.size(); i++){
    data[i]= strdup(vs[i].c_str());
    //     cerr << data[i] << endl;
  }

  bool res=  imageFromXpmdata(const_cast<const char**>(data),img);

  // OBS: free !!!!!!!!!!!!!!!!!!!!!!!!

  delete[] data;

  return res;
}
