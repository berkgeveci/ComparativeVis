cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
  cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

PROJECT(EnsembleAnalyzer)

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

# Give user option of building shared or static libraries.  Default to
# the choice made for VTK.
OPTION(BUILD_SHARED_LIBS "Build with shared libraries."
       ${VTK_BUILD_SHARED_LIBS})


ADD_LIBRARY( vtkDatabaseConnection
             vtkDatabaseConnection.h vtkDatabaseConnection.cxx )
TARGET_LINK_LIBRARIES( vtkDatabaseConnection vtkIO )

ADD_LIBRARY( vtkMetadataBrowser 
             vtkMetadataBrowser.h vtkMetadataBrowser.cxx )
TARGET_LINK_LIBRARIES( vtkMetadataBrowser vtkDatabaseConnection )

FIND_PACKAGE(Qt4)
IF(QT4_FOUND)
    INCLUDE( ${QT_USE_FILE} )
ELSE(QT4_FOUND)
    MESSAGE(FATAL_ERROR "Cannot build without Qt4")
ENDIF(QT4_FOUND)
SET(SCRIPT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/python CACHE PATH
    "Full path to the location of ParaView/Python scripts")
ADD_DEFINITIONS(-DSCRIPT_PATH="${SCRIPT_PATH}")

SET(DATA_PATH ${CMAKE_CURRENT_SOURCE_DIR}/data CACHE PATH
    "Full path to the location of the data.")
ADD_DEFINITIONS(-DDATA_PATH="${DATA_PATH}")

SET(RESULTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/results CACHE PATH
    "Full path to the directory where results should be stored.")
ADD_DEFINITIONS(-DRESULTS_PATH="${RESULTS_PATH}")

ADD_LIBRARY( vtkBatchAnalyzer 
             vtkBatchAnalyzer.h vtkBatchAnalyzer.cxx )
TARGET_LINK_LIBRARIES( vtkBatchAnalyzer vtkCommon ${QT_LIBRARIES} )

ADD_EXECUTABLE( TestMySQL TestMySQL.cxx )
TARGET_LINK_LIBRARIES( TestMySQL vtkMetadataBrowser )

ADD_EXECUTABLE( TestPostgreSQL TestPostgreSQL.cxx )
TARGET_LINK_LIBRARIES( TestPostgreSQL vtkMetadataBrowser )

ADD_EXECUTABLE( TestSQLite TestSQLite.cxx )
TARGET_LINK_LIBRARIES( TestSQLite vtkMetadataBrowser )

ADD_EXECUTABLE( TestBatchAnalyzer TestBatchAnalyzer.cxx )
TARGET_LINK_LIBRARIES( TestBatchAnalyzer vtkBatchAnalyzer )

INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")
QT4_WRAP_UI( GUI_MOC_HDRS vtkEnsembleAnalyzer.ui )
QT4_WRAP_CPP( GUI_MOC_SRCS vtkEnsembleAnalyzer.h)
ADD_EXECUTABLE( EnsembleAnalyzer
  vtkEnsembleAnalyzer.h vtkEnsembleAnalyzer.cxx
  vtkEnsembleAnalyzerMain.cxx ${GUI_MOC_HDRS} ${GUI_MOC_SRCS} )
TARGET_LINK_LIBRARIES( EnsembleAnalyzer
  vtkMetadataBrowser vtkBatchAnalyzer 
  ${QT_LIBRARIES})

#python wrapping
FIND_PACKAGE(PythonInterp QUIET)
MARK_AS_ADVANCED(PYTHON_EXECUTABLE)
FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping  NO_CMAKE_FIND_ROOT_PATH)
MARK_AS_ADVANCED(VTK_WRAP_HINTS)
SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkWrapPython.cmake)
INCLUDE(FindPythonLibs)

#weird that I needed to do this to get python wrapping working...
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}")

VTK_WRAP_PYTHON3(vtkDatabaseConnectionPython Database_Python_Sources 
                 vtkDatabaseConnection.h vtkDatabaseConnection.cxx )
ADD_LIBRARY(vtkDatabaseConnectionPythonD ${Database_Python_Sources})
TARGET_LINK_LIBRARIES(vtkDatabaseConnectionPythonD ${PYTHON_LIBRARY} vtkDatabaseConnection)
PYTHON_ADD_MODULE(vtkDatabaseConnectionPython vtkDatabaseConnectionPythonInit.cxx)
IF(PYTHON_ENABLE_MODULE_vtkDatabaseConnectionPython)
  TARGET_LINK_LIBRARIES(vtkDatabaseConnectionPython vtkDatabaseConnectionPythonD vtkCommonPythonD)

  # Python extension modules on Windows must have the extension ".pyd"
  # instead of ".dll" as of Python 2.5.  Older python versions do support
  # this suffix.
  IF(WIN32 AND NOT CYGWIN)
    SET_TARGET_PROPERTIES(vtkDatabaseConnectionPython PROPERTIES SUFFIX ".pyd")
  ENDIF(WIN32 AND NOT CYGWIN)

  # The python modules are installed by a setup.py script which does
  # not know how to adjust the RPATH field of the binary.  Therefore
  # we must simply build the modules with no RPATH at all.  The
  # vtkpython executable in the build tree should have the needed
  # RPATH anyway.
  SET_TARGET_PROPERTIES(vtkDatabaseConnectionPython PROPERTIES SKIP_BUILD_RPATH 1)

  IF(WIN32 OR APPLE)
    TARGET_LINK_LIBRARIES (vtkDatabaseConnectionPython ${VTK_PYTHON_LIBRARIES})
  ENDIF(WIN32 OR APPLE)

  # Generally the pyhon extension module created is installed using setup.py.
  # However projects that include VTK (such as ParaView) can override this
  # behaviour by not using setup.py, instead directly installing the extension
  # module at the same location as other libraries.
  IF (VTK_INSTALL_PYTHON_USING_CMAKE AND NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL(TARGETS vtkDatabaseConnectionPython
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ENDIF (VTK_INSTALL_PYTHON_USING_CMAKE AND NOT VTK_INSTALL_NO_LIBRARIES)
ENDIF(PYTHON_ENABLE_MODULE_vtkDatabaseConnectionPython)

VTK_WRAP_PYTHON3(vtkMetadataBrowserPython API_Python_Sources 
                 vtkMetadataBrowser.h vtkMetadataBrowser.cxx )
ADD_LIBRARY(vtkMetadataBrowserPythonD ${API_Python_Sources})
TARGET_LINK_LIBRARIES(vtkMetadataBrowserPythonD ${PYTHON_LIBRARY} vtkMetadataBrowser)
PYTHON_ADD_MODULE(vtkMetadataBrowserPython vtkMetadataBrowserPythonInit.cxx)
IF(PYTHON_ENABLE_MODULE_vtkMetadataBrowserPython)
  TARGET_LINK_LIBRARIES(vtkMetadataBrowserPython vtkMetadataBrowserPythonD vtkCommonPythonD)

  # Python extension modules on Windows must have the extension ".pyd"
  # instead of ".dll" as of Python 2.5.  Older python versions do support
  # this suffix.
  IF(WIN32 AND NOT CYGWIN)
    SET_TARGET_PROPERTIES(vtkMetadataBrowserPython PROPERTIES SUFFIX ".pyd")
  ENDIF(WIN32 AND NOT CYGWIN)

  # The python modules are installed by a setup.py script which does
  # not know how to adjust the RPATH field of the binary.  Therefore
  # we must simply build the modules with no RPATH at all.  The
  # vtkpython executable in the build tree should have the needed
  # RPATH anyway.
  SET_TARGET_PROPERTIES(vtkMetadataBrowserPython PROPERTIES SKIP_BUILD_RPATH 1)

  IF(WIN32 OR APPLE)
    TARGET_LINK_LIBRARIES (vtkMetadataBrowserPython ${VTK_PYTHON_LIBRARIES})
  ENDIF(WIN32 OR APPLE)

  # Generally the pyhon extension module created is installed using setup.py.
  # However projects that include VTK (such as ParaView) can override this
  # behaviour by not using setup.py, instead directly installing the extension
  # module at the same location as other libraries.
  IF (VTK_INSTALL_PYTHON_USING_CMAKE AND NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL(TARGETS vtkMetadataBrowserPython
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ENDIF (VTK_INSTALL_PYTHON_USING_CMAKE AND NOT VTK_INSTALL_NO_LIBRARIES)
ENDIF(PYTHON_ENABLE_MODULE_vtkMetadataBrowserPython)

