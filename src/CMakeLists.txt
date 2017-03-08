
CONFIGURE_FILE(config.h.in config.h)

SET(CMAKE_AUTOMOC TRUE)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

INCLUDE_DIRECTORIES(${PC_INCLUDE_DIRS})
ADD_DEFINITIONS(-W -Wall ${PC_CFLAGS_OTHER} ${QT_DEFINITIONS})
LINK_DIRECTORIES(${PC_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS})

IF(ENABLE_FIMEX)
ADD_DEFINITIONS(-DFIMEX)
ENDIF() # ENABLE_FIMEX

SET(lib_SOURCES
  tsDrawArea.cc
  PrepareDataThread.cc
)

SET(tseries_SOURCES
  tseries.cc
  tsSession.cc
  tsSessionOptions.cc
  tsDatafileColl.cc
  tsSetup.cc
  tsRequest.cc
  tsConfigure.cc

  qtsMain.cc
  qtsWork.cc
  qtsSidebar.cc
  qtsShow.cc
  qtsFilterManager.cc
  qtsTimeControl.cc
  StationTab.cc
  CoordinateTab.cc
  FimexTab.cc
  WdbCacheThread.cc
  CoordinateManager.cc
  WdbBookmarkTools.cc
  ParameterFilterDialog.cc
  PopupCalendar.cc
  ClearLineEdit.cc
  FetchStations.cc
)

SET(tseries_IMG
  copy.xpm
  delete.xpm
  filter.xpm
  tb_close.xpm
  tb_left_arrow.xpm
  tb_no.xpm
  tb_refresh.xpm
  tb_right_arrow.xpm
  tb_save.xpm
  tb_yes.xpm
  tseries.xpm
  ts_filter.xpm
  ts_find.xpm
  bookmark.xpm
  view-refresh.xpm
  list-add.xpm
  directory.xpm
  media-record.xpm
  metno.xpm
  trashcan.xpm
  synop.xpm
  expand.xpm
  collapse.xpm
  clear.xpm
  search_small.xpm
  search_folder.xpm
)

QT4_ADD_TRANSLATION(tseries_QM
  ../lang/tseries_de.ts
  ../lang/tseries_fr.ts
  ../lang/tseries_nn.ts
  ../lang/tseries_no.ts
  ../lang/tseries_se.ts
)

ADD_EXECUTABLE(tseries
  ${lib_SOURCES}
  ${tseries_SOURCES}
  ${tseries_IMG}
  ${tseries_QM}
)

TARGET_LINK_LIBRARIES(tseries
  ${PC_LIBRARIES}
  ${Boost_LIBRARIES}
  ${QT_LIBRARIES}
)

SET_TARGET_PROPERTIES(tseries PROPERTIES
  OUTPUT_NAME "tseries.bin-${PVERSION}"
)

########################################################################

INSTALL(TARGETS tseries DESTINATION bin)
INSTALL(FILES ${tseries_QM}
  DESTINATION "share/${PROJECT_NAME}/${PVERSION}/lang")

# building btseries -----------------

## bin_PROGRAMS += btseries
##
## btseries_SOURCES=         \
## 	btseries.cc       \
## 	diImageIO.cc      \
## 	$(header_sources) \
## 	$(common_sources) \
## 	$(cc_sources)
##
## nodist_btseries_SOURCES = \
## 	$(common_moc_sources)
##
## btseries_CXXFLAGS= $(common_cppflags)
## btseries_LDFLAGS=  $(common_ldflags)
## btseries_LDADD=    $(common_ldadd)
