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

#include <QApplication>

#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>
#include <glob.h>

#include "diPrintOptions.h"
#include "tsSetup.h"
#include "tsConfigure.h"
#include "tsDrawArea.h"

#include "config.h"

#include <fstream>
#include <iostream>

using std::cout;
using std::endl;
using std::cerr;

const std::string version_string=VERSION;

tsDrawArea *drawarea;

bool verbose = false;
bool hardcopy_started;

void startHardcopy(const printOptions priop)
{
  if (verbose)
    cout << "- startHardcopy" << endl;
#if 0
  drawarea->setPrintOptions(priop);
  drawarea->startHardcopy();
#endif
  hardcopy_started = true;
}

void endHardcopy()
{
  // finish off postscript-session
  if (hardcopy_started) {
    if (verbose)
      cout << "- endHardcopy " << endl;
#if 0
    drawarea->endHardcopy();
#endif
  }
  hardcopy_started = false;
}

/*
 key/value pairs from commandline-parameters
 */
struct keyvalue {
  std::string key;
  std::string value;
};

/*
 clean an input-string: remove preceding and trailing blanks,
 remove comments
 */
void cleanstr(std::string& s)
{
  std::string::size_type p;
  if ((p = s.find("#")) != std::string::npos)
    s.erase(p);

  miutil::remove(s, '\n');
  miutil::trim(s);
}

// one list of strings with name
struct stringlist {
  std::string name;
  std::vector<std::string> l;
};

// list of lists..
std::vector<stringlist> lists;

/*
 Recursively unpack one (or several nested) LOOP-section(s)
 1) convert a LOOP-section to multiple copies of original text
 with VARIABLES set from ARGUMENTS
 2) ARGUMENTS may be a previously defined LIST

 Syntax for LOOPS:
 LOOP VAR1 [ | VAR2 ... ] = ARG1 [ | ARG2 ... ] , ARG1 [ | ARG2 ... ] , ..
 <contents, all VAR1,VAR2,.. replaced by ARG1,ARG2,.. for each iteration>
 ENDLOOP or LOOP.END
 */
void unpackloop(std::vector<std::string>& orig, // original strings..
    std::vector<int>& origlines, // ..with corresponding line-numbers
    unsigned int& index, // original string-counter to update
    std::vector<std::string>& part, // final strings from loop-unpacking..
    std::vector<int>& partlines) // ..with corresponding line-numbers
{
  unsigned int start = index;

  std::string loops = orig[index];
  loops = loops.substr(4, loops.length() - 4);

  std::vector<std::string> vs, vs2;

  vs = miutil::split(loops, 0, "=");
  if (vs.size() < 2) {
    cerr << "ERROR missing \'=\' in loop-statement at line:"
        << origlines[start] << endl;
    exit(1);
  }

  std::string keys = vs[0]; // key-part
  std::vector<std::string> vkeys = miutil::split(keys, 0, "|");
  unsigned int nkeys = vkeys.size();

  std::string argu = vs[1]; // argument-part
  int nargu;
  std::vector<std::vector<std::string> > arguments;

  /* Check if argument is name of list
   Lists are recognized with preceding '@' */
  if (argu.length() > 1 && argu.substr(0, 1) == "@") {
    std::string name = argu.substr(1, argu.length() - 1);
    // search for list..
    unsigned int k;
    for (k = 0; k < lists.size(); k++) {
      if (lists[k].name == name)
        break;
    }
    if (k == lists.size()) {
      // list not found
      cerr << "ERROR reference to unknown list at line:" << origlines[start]
          << endl;
      exit(1);
    }
    nargu = lists[k].l.size();
    // split listentries into separate arguments for loop
    for (int j = 0; j < nargu; j++) {
      vs = miutil::split(lists[k].l[j], 0, "|");
      // check if correct number of arguments
      if (vs.size() != nkeys) {
        cerr << "ERROR number of arguments in loop at:'" << lists[k].l[j]
            << "' line:" << origlines[start] << " does not match key:" << keys
            << endl;
        exit(1);
      }
      arguments.push_back(vs);
    }

  } else {
    // ordinary arguments to loop: comma-separated
    vs2 = miutil::split(argu, 0, ",");
    nargu = vs2.size();
    for (int k = 0; k < nargu; k++) {
      vs = miutil::split(vs2[k], 0, "|");
      // check if correct number of arguments
      if (vs.size() != nkeys) {
        cerr << "ERROR number of arguments in loop at:'" << vs2[k] << "' line:"
            << origlines[start] << " does not match key:" << keys << endl;
        exit(1);
      }
      arguments.push_back(vs);
    }
  }

  // temporary storage of loop-contents
  std::vector<std::string> tmppart;
  std::vector<int> tmppartlines;

  // go to next line
  index++;

  // start unpacking loop
  for (; index < orig.size(); index++) {
    if (orig[index] == "ENDLOOP" || orig[index] == "LOOP.END") { // reached end
      // we have the loop-contents
      for (int i = 0; i < nargu; i++) { // loop over arguments
        for (unsigned int j = 0; j < tmppart.size(); j++) { // loop over lines
          std::string l = tmppart[j];
          for (unsigned int k = 0; k < nkeys; k++) { // loop over keywords
            // replace all variables
            miutil::replace(l, vkeys[k], arguments[i][k]);
          }
          part.push_back(l);
          partlines.push_back(tmppartlines[j]);
        }
      }
      break;

    } else if (orig[index].substr(0, 4) == "LOOP") { // start of new loop
      unpackloop(orig, origlines, index, tmppart, tmppartlines);

    } else { // fill loop-contents to temporary vector
      tmppart.push_back(orig[index]);
      tmppartlines.push_back(origlines[index]);
    }
  }
  if (index == orig.size()) {
    cerr << "ERROR missing \'LOOP.END\' for loop at line:" << origlines[start]
        << endl;
    exit(1);
  }
}

/*
 Prepare input-lines
 1. unpack loops
 2. recognize and store lists

 Syntax for list with name: <listname>
 LIST.<listname>
 <entry>
 <entry>
 ...
 LIST.END
 */
void unpackinput(std::vector<std::string>& orig, // original setup
    std::vector<int>& origlines, // original list of linenumbers
    std::vector<std::string>& final, // final setup
    std::vector<int>& finallines) // final list of linenumbers
{
  for (unsigned int i = 0; i < orig.size(); i++) {
    if (orig[i].substr(0, 4) == "LOOP") {
      // found start of loop - unpack it
      unpackloop(orig, origlines, i, final, finallines);
    } else if (orig[i].substr(0, 5) == "LIST.") {
      // save a list
      stringlist li;
      if (orig[i].length() < 6) {
        cerr << "ERROR missing name for LIST at line:" << origlines[i] << endl;
        exit(1);
      }
      li.name = orig[i].substr(5, orig[i].length() - 5);
      int start = i;
      i++;
      for (; i < orig.size() && orig[i] != "LIST.END"; i++)
        li.l.push_back(orig[i]);
      if (i == orig.size() || orig[i] != "LIST.END") {
        cerr << "ERROR missing LIST.END for list starting at line:"
            << origlines[start] << endl;
        exit(1);
      }
      // push it..
      lists.push_back(li);
    } else {
      // plain input-line -- push it on the final list
      final.push_back(orig[i]);
      finallines.push_back(origlines[i]);
    }
  }
}

/*
 parse setupfile
 perform other initialisations based on setup information
 */
bool readSetup(const std::string& setupfile, const std::string& site,
    SessionManager& session, DatafileColl& data)
{
  cout << "Reading setupfile:" << setupfile << " for site " << site << endl;

  tsSetup setup;
  if (!setup.read(setupfile, site)) {
    cerr << "ERROR an error occured while reading setup: " << setupfile << endl;
    return false;
  }

  session.readSessions(setup.files.defs,setup.path.styles, verbose);

  for (unsigned int i = 0; i < setup.streams.size(); i++) {
    data.addDataset(setup.streams[i].collectionName);

    for (unsigned int j = 0; j < setup.streams[i].data.size(); j++) {
      data.addStream(setup.streams[i].data[j].name, // streamname
          setup.streams[i].data[j].descript, // description
          setup.streams[i].data[j].type, // streamtype
          i, j, // dataset and
          // number in dataset
          setup.streams[i].data[j].contents); // models/runs
    }
  }

  data.openStreams();

  return true;
}


/*
  Output Help-message:  call-syntax and optionally an example
  input-file for btseries
*/
void printUsage(bool showexample)
{

  const std::string
      help =
                "***************************************************               \n"
                "* TSERIES batch version:" + version_string +
                "                  *\n"
                "* plot products in batch and dump result to file. *               \n"
                "***************************************************               \n"
                "* Available output-formats:                       *               \n"
                "* - as PostScript (to file and printer)          *                \n"
                "* - as EPS (Encapsulated PostScript)              *               \n"
                "* - as RASTER (format from filename suffix)       *               \n"
                "* - as PNG (raster-format)                        *               \n"
                "***************************************************               \n"
                "                                                                  \n"
                "Usage: btseries "
                "-i <JOB-FILENAME> [-s <SETUP-FILENAME>] [-S <SITE-NAME>] [-v] [-display XHOST:DISPLAY] [-example] "
                "[key=value key=value]                                             \n"
                "                                                                  \n"
                "-i       : job-control file. See example below.                   \n"
                "                                                                  \n"
                "-s       : setupfile for tseries                                  \n"
                "                                                                  \n"
                "-S       : site [VA:VV:VNN:VTK:MA:FOU] (default FOU)              \n"
                "                                                                  \n"
                "-v       : (verbose) for more job-output                          \n"
                "                                                                  \n"
/*
                "-display : btseries needs access to X-server.                     \n"
                "           HINT: for true batch, try Xvfb                         \n"
*/
                "                                                                  \n"
                "-example : list example input-file and exit                       \n"
                "                                                                  \n"
                "                                                                  \n";

  const std::string
      example =
            "#--------------------------------------------------------------   \n"
            "# inputfile for btseries                                          \n"
            "# - '#' marks start of comment.                                   \n"
            "# - you may split long lines by adding '\\' at the end.           \n"
            "#--------------------------------------------------------------   \n"
            "                                                                  \n"
            "#- Mandatory:                                                     \n"
            "buffersize=800x600       # plotbuffer (WIDTHxHEIGHT)              \n"
            "                         # For output=RASTER: size of plot.       \n"
            "                         # For output=POSTSCRIPT: size of buffer  \n"
            "                         #  affects output-quality.      \n"
            "                                                                  \n"
            "#- Optional: values for each option below are default-values      \n"
            "setupfile=tseries.ctl    # use a standard setup-file              \n"
            "output=POSTSCRIPT        # POSTSCRIPT/EPS/RASTER/PNG              \n"
            "colour=COLOUR            # GREYSCALE/COLOUR                       \n"
            "filename=tmp_tseries.ps  # output filename                        \n"
            "                                                                  \n"
            "# the following options for output=POSTSCRIPT or EPS only         \n"
            "toprinter=NO             # send output to printer (postscript) \n"
            "                         # obsolete command! use PRINT_DOCUMENT instead\n"
            "printer=fou3             # name of printer        (postscript)    \n"
            "                         # (see PRINT_DOCUMENT command below)     \n"
            "papersize=297x420,A4     # size of paper in mm,   (postscript)    \n"
            "                         # papertype (A4 etc) or both.            \n"
            "drawbackground=NO        # plot background colour (postscript)    \n"
            "orientation=LANDSCAPE    # PORTRAIT/LANDSCAPE     (postscript)    \n"
            "                         # (default here is really 'automatic'    \n"
            "                         # which sets orientation according to    \n"
            "                         # width/height-ratio of buffersize)      \n"
            "                                                                  \n"
            "#--------------------------------------------------------------   \n"
            "# Diagram Options\n"
            "#--------------------------------------------------------------   \n"
            "# DIAGRAMTYPE=<name>     # Name of diagramtype (defined elsewhere)\n"
            "# POSITION=<name>        # Name of position in datasources        \n"
            "# POSITION_NAME=<name>   # Change name on diagram                 \n"
            "# MODEL=<name>           # Name of model in datasources           \n"
            "# RUN=<run-hour>         # Model runtime (UNDEF is a legal value) \n"
            "# PLOT                   # Make the diagram-plot                  \n"
            "#--------------------------------------------------------------   \n"
            "# Additional:                                                     \n"
            "#--------------------------------------------------------------   \n"
            "#- You can add LOOPS with one or more variables:                  \n"
            "#  LOOP [X]|[Y] = X_value1 | Y_value1 , X_value2 | Y_value2       \n"
            "#   <any other input lines, all \"[X]\" and \"[Y]\" will be       \n"
            "#   replaced by the values after '=' for each iteration>          \n"
            "#  LOOP.END                                                       \n"
            "#  The example shows a loop with two variables ([X] and [Y],      \n"
            "#  separated by '|') and two iterations (separated with ',')      \n"
            "#  Loops kan be nested                                            \n"
            "#--------------------------------------------------------------   \n"
            "#- Make a LIST for use in loops:                                  \n"
            "#  LIST.stations           # A new list with name=stations        \n"
            "#  OSLO                    # May contain any strings..            \n"
            "#  KIRKENES                #                                      \n"
            "#  LIST.END                # Marks End of list                    \n"
            "#                                                                 \n"
            "#  To use in a loop:                                              \n"
            "#  LOOP [VAR]=@stations    # The key here is the \'@\' with the   \n"
            "#                          # list-name.                           \n"
            "#  LOOP.END                # This will loop over all list-entries \n"
            "#                                                                 \n"
            "#  NOTES:                                                         \n"
            "#  - To make a list with multiple variables, convenient for       \n"
            "#    multiple-variable loops, just add \'|\'s in the list-strings.\n"
            "#    Example:                                                     \n"
            "#    LIST.name             # new list                             \n"
            "#    OSLO | blue           # two variables for each entry         \n"
            "#    KIRKENES | red        #                                      \n"
            "#    LIST.END                                                     \n"
            "#                                                                 \n"
            "#    LOOP [POS] | [COL] = @name # Loop using two variables in list\n"
            "#    LOOP.END                                                     \n"
            "#  - Lists must be defined OUTSIDE all loop statements            \n"
            "#--------------------------------------------------------------   \n"
            "#- \"key=value\" pairs given on the commandline controls variables\n"
            "#  in the inputfile: Any \"$key\" found in the text will be       \n"
            "#  substituted by \"value\".                                      \n"
            "#--------------------------------------------------------------   \n"
            "#* PostScript output * \n"
            "#- Send current postscript-file to printer (immediate command):   \n"
            "#  PRINT_DOCUMENT                         \n"
            "#\n"
            "#- MULTIPLE PLOTS PER PAGE                  \n"
            "#  You can put several plots in one postscript page by using the \'multiple.plots\'\n"
            "#  and \'plotcell\' commands. Start with describing the layout you want:\n"
            "# \n"
            "#  MULTIPLE.PLOTS=<rows>,<columns> # set the number of rows and columns\n"
            "# \n"
            "#  In the same command, you can specify the spacing between plots and \n"
            "#  the page-margin (given as percent of page width/height [0-100]): \n"
            "#  MULTIPLE.PLOTS=<rows>,<columns>,<spacing>,<margin> \n"
            "#  \n"
            "#  Then, for each separate plot use the plotcell command to place plot on page:\n"
            "#  PLOTCELL=<row>,<column>         # the row and column, starting with 0\n"
            "#  \n"
            "#  Finally, end the page with: \n"
            "#  MULTIPLE.PLOTS=OFF \n"
            "#  \n"
            "#- To produce multi-page postscript files: Just make several plots \n"
            "#  to the same filename (can be combined with MULTIPLE.PLOTS). \n"
            "#  \n"
            "#- Use of alpha-channel blending is not supported in postscript \n"
            "#--------------------------------------------------------------   \n"
            "#- Loop over all positions in datasource \n"
            "#  POSLOOP <POSVARNAME> [ MULTIPLE.PLOTS=<nx>,<ny>,<spac.>,<marg.>  <YVARNAME> <XVARNAME> ] \n"
            "#  POSITION=<POSVARNAME> \n"
            "#  if MULTIPLE.PLOTS used:\n"
            "#    PLOTCELL=<YVARNAME>,<XVARNAME>   \n"
            "#  PLOT  \n"
            "#  POSLOOP.END \n"
            "#--------------------------------------------------------------   \n"
            "# Product-examples:                                               \n"
            "#--------------------------------------------------------------   \n"
            "# == single diagram == \n"
            "DIAGRAMTYPE=Meteogram # Name of diagramtype  \n"
            "MODEL=HIRLAM.12km     # Name of model in datasources \n"
            "RUN=12                # Model runtime \n"
            "POSITION=OSLO         # Name of position in datasources \n"
            "POSITION_NAME=Oslo    # Change name on plot \n"
            "PLOT                  # make the diagram  \n"
            "\n"
            "# == several diagrams on one page == \n"
            "buffersize=1000x1500    # plotbuffer (WIDTHxHEIGHT)              \n"
            "filename=bt2.ps  \n"
            "\n"
            "LIST.stations \n"
            "BERGEN       | 0 | 0 | HIRLAM.20km | 06 | Meteogram \n"
            "OSLO         | 0 | 1 | HIRLAM.20km | 06 | Meteogram m/snø \n"
            "BODØ         | 0 | 2 | EPS         | 12 | EPS_T2M \n"
            "KRISTIANSAND | 0 | 3 | EPS         | 12 | EPS_RR24 \n"
            "STAVANGER    | 1 | 0 | EPS         | 12 | EPS_Z500 \n"
            "STAD         | 1 | 1 | ECOM3D_20km | 00 | Havdiagram \n"
            "STAD         | 1 | 2 | WAM         | 12 | Bølgediagram \n"
            "LONDON       | 1 | 3 | ECMWF_U     | 12 | EPS_RR24/T2M \n"
            "LIST.END \n"
            "\n"
            "MULTIPLE.PLOTS=4,2,3,2 \n"
            "LOOP [POS] | [COL] | [ROW] | [MODEL] | [RUN] | [DTYPE] = @stations \n"
            "POSITION=[POS] \n"
            "PLOTCELL=[ROW],[COL] \n"
            "MODEL=[MODEL] \n"
            "DIAGRAMTYPE=[DTYPE] \n"
            "RUN=[RUN] \n"
            "PLOT \n"
            "LOOP.END \n"
            "MULTIPLE.PLOTS=OFF \n"
            "#--------------------------------------------------------------   \n";

  if (!showexample)
    cout << help << endl;
  else
    cout << example << endl;

  exit(1);
}


/*
 =================================================================
 BTSERIES - BATCH PRODUCTION OF PETS GRAPHICAL PRODUCTS
 =================================================================
 */
int main(int argc, char** argv)
{
  QApplication a(argc,argv);

  int xsize, ysize; // total pixmap size
  bool multiple_plots = false; // multiple plots per page
  int numcols, numrows; // for multiple plots
  int plotcol, plotrow; // current plotcell for multiple plots
  int deltax, deltay; // width and height of plotcells
  int margin, spacing; // margin and spacing for multiple plots
  bool multiple_newpage = false; // start new page for multiple plots

  float pixwidth;
  float pixheight;

  tsSetup setup;
  tsRequest request;
  SessionManager session;
  DatafileColl data;

  // replaceable values for plot-commands
  std::vector<keyvalue> keys;

  miutil::miTime time, ptime, fixedtime;

  std::string sarg;
  std::string batchinput;
  // tseries setup file
  std::string setupfile = "tseries.ctl";
  std::string site = "FOU";
  bool setupfilegiven = false;

  // check command line arguments
  if (argc < 2) {
    printUsage(false);
  }

  std::vector<std::string> ks;
  int ac = 1;
  while (ac < argc) {
    sarg = argv[ac];

    if (sarg == "-display") {
      ac++;
      if (ac >= argc)
        printUsage(false);

    } else if (sarg == "-input" || sarg == "-i") {
      ac++;
      if (ac >= argc)
        printUsage(false);
      batchinput = argv[ac];

    } else if (sarg == "-setup" || sarg == "-s") {
      ac++;
      if (ac >= argc)
        printUsage(false);
      setupfile = argv[ac];
      setupfilegiven = true;

    } else if (sarg == "-site" || sarg == "-S") {
      ac++;
      if (ac >= argc)
        printUsage(false);
      site = argv[ac];

    } else if (sarg == "-v") {
      verbose = true;

    } else if (sarg == "-example") {
      printUsage(true);

    } else {
      ks = miutil::split(sarg, "=");
      if (ks.size() == 2) {
        keyvalue tmp;
        tmp.key = ks[0];
        tmp.value = ks[1];
        keys.push_back(tmp);

      } else {
        cerr << "WARNING unknown argument on commandline:" << sarg << endl;
      }
    }
    ac++;
  }

  if ((batchinput.empty()))
    printUsage(false);

  cout << argv[0] << " : TSERIES batch version " << version_string << endl;

  data.setVerbose(verbose);

  // open batchinput
  std::ifstream bfile(batchinput.c_str());
  if (!bfile) {
    cerr << "ERROR cannot open inputfile " << batchinput << endl;
    return 1;
  }

  bool toprinter = false;
  bool raster = false; // false means postscript
  enum image_type {
    image_rgb, image_png, image_unknown
  };
  int raster_type = image_unknown; // see enum image_type above

  printerManager printman;
  printOptions priop;
  priop.fname = "tmp_tseries.ps";
  priop.colop = d_print::greyscale;
  priop.drawbackground = false;
  priop.orientation = d_print::ori_automatic;
  priop.pagesize = d_print::A4;
  // 1.4141
  priop.papersize.hsize = 297;
  priop.papersize.vsize = 420;
  priop.doEPS = false;

  std::string diagramtype;
  std::string modelname;
  int modelrun = R_UNDEF;
  std::string posname;
  std::string newposname;

  std::string s;
  std::vector<std::string> vs, vvs, vvvs;
  int n, nv;
  bool setupread = false;
  bool buffermade = false;
  xsize = 1696;
  ysize = 1200;

  pixwidth = 1.0;
  pixheight = 1.0;

  std::vector<std::string> lines, tmplines;
  std::vector<int> linenumbers, tmplinenumbers;
  bool merge = false, newmerge;
  int linenum = 0;
  int nkeys = keys.size();

  hardcopy_started = false;

  /*
   if setupfile specified on the command-line, parse it now
   */
  if (setupfilegiven) {
    setupread = readSetup(setupfile, site, session, data);
    if (!setupread) {
      cerr << "ERROR no setup information ... exiting: " << endl;
      return 99;
    }
  }

  /*
   read input-file
   - skip blank lines
   - strip lines for comments and left/right whitespace
   - merge lines (ending with \)
   */
  while (std::getline(bfile, s)) {
    linenum++;
    cleanstr(s);
    n = s.length();
    if (n > 0) {
      newmerge = false;
      if (s[n - 1] == '\\') {
        newmerge = true;
        s = s.substr(0, s.length() - 1);
      }
      if (merge) {
        tmplines[tmplines.size() - 1] += s;
      } else {
        tmplines.push_back(s);
        tmplinenumbers.push_back(linenum);
      }
      merge = newmerge;
    }
  }
  // unpack loops and lists
  unpackinput(tmplines, tmplinenumbers, lines, linenumbers);

  linenum = lines.size();

  // substitute key-values
  if (nkeys > 0)
    for (int k = 0; k < linenum; k++)
      for (int m = 0; m < nkeys; m++)
        miutil::replace(lines[k], "$" + keys[m].key, keys[m].value);

  // parse input - and perform plots
  for (int k = 0; k < linenum; k++) {// input-line loop
    if (verbose)
      cout << "PARSING LINE:" << lines[k] << endl;
    // start parsing...

    if (lines[k] == "PLOT") {
      // --- START PLOT ---
      if (verbose)
        cout << "Start new " << diagramtype << " plot for pos:" << posname
            << " model:" << modelname << " and run:" << modelrun << endl;

      if (!buffermade) {
        cerr << "ERROR no buffersize set..exiting" << endl;
        return 1;
      }
      if (!setupread) {
        setupread = readSetup(setupfile, site, session, data);
        if (!setupread) {
          cerr << "ERROR, no setupinformation..exiting" << endl;
          return 99;
        }
      }

      if (!drawarea) {
        drawarea = new tsDrawArea(&request, &data, &session);
      }

      if (verbose)
        cout << "- setPlotWindow" << endl;
      if (!multiple_plots) {
        pixwidth = (globalWindow.x2 - globalWindow.x1) / float(xsize);
        pixheight = (globalWindow.y2 - globalWindow.y1) / float(ysize);
#if 0
        drawarea->setViewport(xsize, ysize, pixwidth, pixheight);
#endif
      } else {
        pixwidth = (globalWindow.x2 - globalWindow.x1) / float(deltax);
        pixheight = (globalWindow.y2 - globalWindow.y1) / float(deltay);
#if 0
        drawarea->setViewport(deltax, deltay, pixwidth, pixheight);
#endif
      }

      if (verbose)
        cout << "- preparing.." << endl;
      if ((!modelname.empty()))
        request.setModel(modelname);
      request.setPos(posname, newposname);
      request.setStyle(diagramtype);
      request.setRun(modelrun);
      drawarea->prepare();

      if (!raster && (!multiple_plots || multiple_newpage)) {
        startHardcopy(priop);
        multiple_newpage = false;
      }

      if (multiple_plots) {
#if 0
        glViewport(margin + plotcol * (deltax + spacing), margin + plotrow
            * (deltay + spacing), deltax, deltay);
#endif
      }

      if (verbose)
        cout << "- plot" << endl;
#if 0
      glLoadIdentity();
      glOrtho(globalWindow.x1, globalWindow.x2, globalWindow.y1,
          globalWindow.y2, -1, 1);
      drawarea->plot();
#endif

      if (raster) {
        QImage image; // FIXME = qpbuffer->toImage();

        if (verbose){
          cout << "- Saving image to:" << priop.fname;
          cout.flush();
        }

        bool result = image.save(priop.fname.c_str());

        if (verbose){
          cout << " .." << (result ? "Ok" : " **FAILED!**") << endl;
        } else if (!result){
          cerr << " ERROR, saving image to:" << priop.fname << endl;
        }

      } else { // PostScript only
        if (toprinter) { // automatic print of each page
          // Note that this option works bad for multi-page output:
          // use PRINT_DOCUMENT instead
          if ((priop.printer.empty())) {
            cerr << " ERROR printing document:" << priop.fname
                << "  Printer not defined!" << endl;
            continue;
          }
          // first stop postscript-generation
          endHardcopy();
          multiple_newpage = true;

          std::string command = printman.printCommand();
          priop.numcopies = 1;

          printman.expandCommand(command, priop);

          if (verbose)
            cout << "- Issuing print command:" << command << endl;
         int sys = system(command.c_str());
        }
      }

      continue;

      /* ==============================================
       POSLOOP <POSVARNAME> [ MULTIPLE.PLOTS=<nx>,<ny>,<spac.>,<marg.>  <YVARNAME> <XVARNAME> ]
       */
    } else if (miutil::contains(miutil::to_upper(lines[k]), "POSLOOP ")) {
      int j;
      std::string loopvar, xvar = "[COL]", yvar = "[ROW]";
      std::vector<std::string> newlines, vs, vvs, positions;
      vs = miutil::split(lines[k], " ");
      if (vs.size() > 1) {
        loopvar = vs[1];
      }
      bool make_multi = false;
      int nx = 1, ny = 1, ns = 0, nm = 0;
      int ix, iy;
      std::string mulcom;
      if (vs.size() > 2) {
        if (miutil::contains(miutil::to_upper(vs[2]), "MULTIPLE.PLOTS=")) {
          mulcom = vs[2];
          vvs = miutil::split(vs[2], "=");
          if (vvs.size() > 1) {
            vvs = miutil::split(vvs[1], ",");
            if (vvs.size() > 1) {
              ny = atoi(vvs[0].c_str());
              nx = atoi(vvs[1].c_str());
              make_multi = true;
              if (vvs.size() == 4) {
                ns = atoi(vvs[2].c_str());
                nm = atoi(vvs[3].c_str());
              }
            }
          }
          if (vs.size() > 4) {
            yvar = vs[3];
            xvar = vs[4];
          }
        }
      }
      for (j = k + 1; j < linenum && miutil::to_upper(lines[j]) != "POSLOOP.END"; j++)
        newlines.push_back(lines[j]);
      if (j == linenum) {
        cerr << "  ERROR - POSLOOP without proper ending in line:" << k << endl;
        exit(1);
      }
      int numpos = data.getNumPositions();
      int ii = 0;
      std::string pos;
      int id, prio;
      float lat, lng;
      while (data.getPosition(-1, ii, pos, id, lat, lng, prio)) {
        positions.push_back(pos);
      }
      numpos = positions.size();
      std::vector<std::string> looplines;
      int m = newlines.size();
      ix = -1;
      iy = 0;
      if (make_multi)
        looplines.push_back(mulcom);

      for (int ii = 0; ii < numpos; ii++) {
        if (make_multi) {
          ix++;
          if (ix >= nx) {
            ix = 0;
            iy++;
            if (iy >= ny) {
              ix = 0;
              iy = 0;
              looplines.push_back("MULTIPLE.PLOTS=OFF");
              looplines.push_back(mulcom);
            }
          }
        }
        for (int jj = 0; jj < m; jj++) {
          std::string tmp = newlines[jj];
          miutil::replace(tmp, loopvar, positions[ii]);
          miutil::replace(tmp, xvar, miutil::from_number(ix));
          miutil::replace(tmp, yvar, miutil::from_number(iy));
          looplines.push_back(tmp);
        }
      }
      if (make_multi)
        looplines.push_back("MULTIPLE.PLOTS=OFF");

      lines.erase(lines.begin() + k, lines.begin() + j + 1);
      lines.insert(lines.begin() + k, looplines.begin(), looplines.end());
      linenum = lines.size();
      k--;

      continue;

    } else if (miutil::to_upper(lines[k]) == "PRINT_DOCUMENT") {
      if (raster) {
        cerr << " ERROR printing raster-images" << endl;
        continue;
      }
      if ((priop.printer.empty())) {
        cerr << " ERROR printing document:" << priop.fname
            << "  Printer not defined!" << endl;
        continue;
      }
      // first stop postscript-generation
      endHardcopy();
      multiple_newpage = true;

      std::string command = printman.printCommand();
      priop.numcopies = 1;

      printman.expandCommand(command, priop);

      if (verbose)
        cout << "- Issuing print command:" << command << endl;
      int sys = system(command.c_str());

      continue;
    }

    // all other options on the form KEY=VALUE

    vs = miutil::split(lines[k], "=");
    nv = vs.size();
    if (nv < 2) {
      cerr << "ERROR unknown command:" << lines[k] << " Linenumber:"
          << linenumbers[k] << endl;
      return 1;
    }
    std::string key = miutil::to_lower(vs[0]);
    int ieq = lines[k].find_first_of("=");
    std::string value = lines[k].substr(ieq + 1, lines[k].length() - ieq - 1);
    miutil::trim(key);
    miutil::trim(value);

    if (key == "setupfile") {
      if (setupread) {
        cerr
            << "WARNING setupfile overrided by command line option. Linenumber:"
            << linenumbers[k] << endl;
      } else {
        setupfile = value;
        setupread = readSetup(setupfile, site, session, data);
        if (!setupread) {
          cerr << "ERROR no setup information ... exiting: " << endl;
          return 99;
        }
      }

    } else if (key == "buffersize") {
      vvs = miutil::split(value, "x");
      if (vvs.size() < 2) {
        cerr << "ERROR buffersize should be WxH:" << lines[k] << " Linenumber:"
            << linenumbers[k] << endl;
        return 1;
      }
      xsize = atoi(vvs[0].c_str());
      ysize = atoi(vvs[1].c_str());

      // first stop ongoing postscript sessions
      endHardcopy();

#if 0
      glOrtho(globalWindow.x1, globalWindow.x2, globalWindow.y1,
          globalWindow.y2, -1, 1);
      glViewport(0, 0, xsize, ysize);
#endif

      // for multiple plots
      priop.viewport_x0 = 0;
      priop.viewport_y0 = 0;
      priop.viewport_width = xsize;
      priop.viewport_height = ysize;

      buffermade = true;

    } else if (key == "papersize") {
      vvvs = miutil::split(value, ","); // could contain both pagesize and papersize
      for (unsigned int l = 0; l < vvvs.size(); l++) {
        if (miutil::contains(vvvs[l], "x")) {
          vvs = miutil::split(vvvs[l], "x");
          if (vvs.size() < 2) {
            cerr
                << "ERROR papersize should be WxH or WxH,PAPERTYPE or PAPERTYPE:"
                << lines[k] << " Linenumber:" << linenumbers[k] << endl;
            return 1;
          }
          priop.papersize.hsize = atoi(vvs[0].c_str());
          priop.papersize.vsize = atoi(vvs[1].c_str());
          priop.usecustomsize = true;
        } else {
          priop.pagesize = printman.getPage(vvvs[l]);
        }
      }

    } else if (key == "filename") {
      if ((not !value.empty())) {
        cerr << "ERROR illegal filename in:" << lines[k] << " Linenumber:"
            << linenumbers[k] << endl;
        return 1;
      } else {
        miutil::replace(value, " ", "_");
        priop.fname = value;
      }

    } else if (key == "toprinter") {
      toprinter = (miutil::to_lower(value) == "yes");

    } else if (key == "printer") {
      priop.printer = value;

    } else if (key == "output") {
      value = miutil::to_lower(value);
      if (value == "postscript") {
        raster = false;
        priop.doEPS = false;
      } else if (value == "eps") {
        raster = false;
        priop.doEPS = true;
      } else if (value == "rgb") {
        raster = true;
        raster_type = image_rgb;
      } else if (value == "png") {
        raster = true;
        raster_type = image_png;
      } else if (value == "raster") {
        raster = true;
        raster_type = image_unknown;
      } else {
        cerr << "ERROR unknown output-format:" << lines[k] << " Linenumber:"
            << linenumbers[k] << endl;
        return 1;
      }
      if (raster && multiple_plots) {
        cerr
            << "ERROR. multiple plots and raster-output can not be used together: "
            << lines[k] << " Linenumber:" << linenumbers[k] << endl;
        return 1;
      }
      if (raster) {
        // first stop ongoing postscript sessions
        endHardcopy();
      }

    } else if (key == "colour") {
      if (miutil::to_lower(value) == "greyscale")
        priop.colop = d_print::greyscale;
      else
        priop.colop = d_print::incolour;

    } else if (key == "drawbackground") {
      priop.drawbackground = (miutil::to_lower(value) == "yes");

    } else if (key == "diagramtype") {
      diagramtype = value;

    } else if (key == "model") {
      modelname = value;

    } else if (key == "run") {
      if (miutil::to_upper(value) == "UNDEF")
        modelrun = R_UNDEF;
      else
        modelrun = atoi(value.c_str());

    } else if (key == "position") {
      posname = newposname = value;
      miutil::replace(newposname, "%", "");

    } else if (key == "position_name") {
      newposname = value;

    } else if (key == "orientation") {
      value = miutil::to_lower(value);
      if (value == "landscape")
        priop.orientation = d_print::ori_landscape;
      else if (value == "portrait")
        priop.orientation = d_print::ori_portrait;
      else
        priop.orientation = d_print::ori_automatic;

    } else if (key == "multiple.plots") {
      if (raster) {
        cerr
            << "ERROR. multiple plots and raster-output can not be used together: "
            << lines[k] << " Linenumber:" << linenumbers[k] << endl;
        return 1;
      }
      if (miutil::to_lower(value) == "off") {
        multiple_newpage = false;
        multiple_plots = false;
#if 0
        glViewport(0, 0, xsize, ysize);
#endif

      } else {
        std::vector<std::string> v1 = miutil::split(value, ",");
        if (v1.size() < 2) {
          cerr << "WARNING. Illegal values to multiple.plots:" << lines[k]
              << " Linenumber:" << linenumbers[k] << endl;
          multiple_plots = false;
          return 1;
        }
        numrows = atoi(v1[0].c_str());
        numcols = atoi(v1[1].c_str());
        if (numrows < 1 || numcols < 1) {
          cerr << "WARNING. Illegal values to multiple.plots:" << lines[k]
              << " Linenumber:" << linenumbers[k] << endl;
          multiple_plots = false;
          return 1;
        }
        float fmargin = 0.0;
        float fspacing = 0.0;
        if (v1.size() > 2) {
          fspacing = atof(v1[2].c_str());
          if (fspacing >= 100 || fspacing < 0) {
            cerr << "WARNING. Illegal value for spacing:" << lines[k]
                << " Linenumber:" << linenumbers[k] << endl;
            fspacing = 0;
          }
        }
        if (v1.size() > 3) {
          fmargin = atof(v1[3].c_str());
          if (fmargin >= 100 || fmargin < 0) {
            cerr << "WARNING. Illegal value for margin:" << lines[k]
                << " Linenumber:" << linenumbers[k] << endl;
            fmargin = 0;
          }
        }
        margin = int(xsize * fmargin / 100.0);
        spacing = int(xsize * fspacing / 100.0);
        deltax = (xsize - 2 * margin - (numcols - 1) * spacing) / numcols;
        deltay = (ysize - 2 * margin - (numrows - 1) * spacing) / numrows;
        multiple_plots = true;
        multiple_newpage = true;
        plotcol = plotrow = 0;
        if (verbose)
          cout << "Starting multiple_plot, rows:" << numrows << " , columns: "
              << numcols << endl;
      }

    } else if (key == "plotcell") {
      if (!multiple_plots) {
        cerr << "ERROR. multiple plots not initialised:" << lines[k]
            << " Linenumber:" << linenumbers[k] << endl;
        return 1;
      } else {
        std::vector<std::string> v1 = miutil::split(value, ",");
        if (v1.size() != 2) {
          cerr << "WARNING. Illegal values to plotcell:" << lines[k]
              << " Linenumber:" << linenumbers[k] << endl;
          return 1;
        }
        plotrow = atoi(v1[0].c_str());
        plotcol = atoi(v1[1].c_str());
        if (plotrow < 0 || plotrow >= numrows || plotcol < 0 || plotcol
            >= numcols) {
          cerr << "WARNING. Illegal values to plotcell:" << lines[k]
              << " Linenumber:" << linenumbers[k] << endl;
          return 1;
        }
        // row 0 should be on top of page
        plotrow = (numrows - 1 - plotrow);
      }

    } else {
      cerr << "WARNING. Unknown command:" << lines[k] << " Linenumber:"
          << linenumbers[k] << endl;
    }
  }

  // finish off postscript-sessions
  endHardcopy();

  return 0;
}
