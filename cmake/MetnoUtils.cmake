
SET(CMAKE_BUILD_TYPE RelWithDebInfo)
SET(CMAKE_VERBOSE_MAKEFILE OFF)

INCLUDE(GNUInstallDirs)
INCLUDE(FindPkgConfig)

if(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

IF("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
  SET(SYSCONFDIR "/etc")
ELSE()
  SET(SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc")
ENDIF()

#########################################################################

MACRO(METNO_FIND_QT4 components)
  FOREACH(c ${ARGV})
    LIST(APPEND qc "Qt${c}")
    LIST(APPEND ql "Qt4::Qt${c}")
  ENDFOREACH()
  FIND_PACKAGE(Qt4 REQUIRED COMPONENTS ${qc})
  SET(METNO_PC_DEPS_QT ${qc})
  SET(QT_LIBRARIES ${ql})
ENDMACRO()
