
Tseries - Meteorological Timeseries Viewer
==========================================

Norwegian Meteorological Institute (met.no)  
Box 43 Blindern  
0313 OSLO  
NORWAY  
email: diana@met.no

About Tseries
----------------

Tseries is part of the Diana package from met.no (Norwegian Meteorological Institute) and is distributed under the GPL license. See gpl.txt for details concerning the GPL license.

Tseries is a graphical time series viewer developed for use with meteorological and oceanographic data. It uses OpenGL for the graphics and Trolltech Qt for the user interface.

Tseries supports multiple languages via the Qt linguist system. Additional languages can be added by preparing new `lang/tseries_XX.ts` files (using the Qt linguist program) and adding the new language to the build process (`src/CMakeLists.txt`). 

Depends on:
 * [Qt4](http://qi.io) for the user interface and the drawing code
 * [HDF 4.2](http://www.hdfgroup.org/)
 * [fimex](http://fimex.met.no)
 * MET Norway [metlibs](https://svn.met.no/viewvc/metlibs/)
 * [CMake](https://cmake.org/) (above version 2.8.7)

Installation
------------

For ubuntu users, Tseries is available from the [MET Norway launchpad PPA](https://launchpad.net/~met-norway/+archive/ubuntu/metapps).

To compile Tseries yourself, you first need to compile the required fimex and metlibs libraries and install them. Then you may use something like this to compile Tseries:

    export PACKAGE_CONFIG_PATH=where_you_installed_metlibs/lib/pkgconfig
    BUILD=/path/to/tseries/build
    SOURCE=/path/to/tseries/source
    mkdir -p $BUILD
    cd $BUILD
    cmake $SOURCE
    make
    make install

Running Tseries
---------------

running Tseries with the prepared test datasets and configurations:

1. The test data sets should be installed under `metnolocal/demodata/`
2. Starting Tseries
        cd metnolocal/tseries
        bin/tseries -s tseries.ctl_export

Select the language of preference under "Preferences | languages" in the menu.
User guide available under Help.

Configuration files
--------------------

`tseries.ctl_export`:
  The setup file for Tseries. 
  Change top level directory for installation with `Topdir` variable
  Add new datafiles under section `<streams> `

`etc/tsDiagrams_export.def`:
  Definition of diagram types with supported data
  See comments in file for syntax
  IMPORTANT: if you change the top level directory in `tseries.ctl_export`,
  you must also update the style paths here (see `STYLEFILE=`).

`.tseries.conf`:
  User preferences saved here.

`etc/tseries.filter`:
  User defined position filter saved here (see online doc)

`etc/symbols.def`:
  Weather symbol mapping

`style/style.*`
  Detailed instructions for the plotting package (in ascii). 
  Determines which components to add to a specific diagram.
  Used in `etc/tsDiagrams_export.def`. The structure of these files
  is currently undocumented.

File formats
------------

Tseries supports a number of in-house (met.no) formats. 

1. The most commonly used is the HDF 4.2 based format. In the <streams> section of the setup file you will find references to files of this type with "DataType=HDF". 
The exact specification for this format is omitted here; mainly because we intend to make changes to this format in the near future. If you are still interested: use the 'vshow' program that comes with the HDF4.2 package to view the structure of one the files in the test data set.

2. Tseries also supports a simple ascii format. See the test data file `metnolocal/demodata/hirlam/t2m_lqr.txt` for a complete example. 
Here is a short example:

        # --------------------------------------
        # Example time series data file in ascii
        # --------------------------------------
        
        #- source name, runtime
        Model=HIRLAM20, 2006-05-24 00:00:00
        
        #-----------------
        # New position
        #-----------------
        #- Name of position, longitude, latitude
        Position=GARDERMOEN, 11.1, 60.2
        
        #- Data-header
        #  the Time and Level specification is mandatory
        #  Add your own parameters after these (defined properly in etc/parameters.def)
        Time                  Level     MSLP    TT
        #- Data-table matching header
        2006-05-23 18:00:00   0         1000.1  23.3
        2006-05-24 00:00:00   0         999.9   16.4
        .....
        
        #-----------------
        # Start a new position with Position=
        #-----------------
        #- Name of position, longitude, latitude
        Position=X,....
        ....
        # End of Ascii file format description
# tseries-qt4
