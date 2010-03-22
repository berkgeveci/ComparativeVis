PROJECT(CMFEFilter)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})
  
IF(NOT VTK_USE_MPI)
	MESSAGE(SEND_ERROR "CMFE requires MPI" )
ENDIF(NOT VTK_USE_MPI)

INCLUDE(FindMPI)
INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS}  
  ${MPI_INCLUDE_PATH}
  )
  
SET(SERVER_SOURCES
vtkCMFEFilter.h
vtkCMFEFilter.cxx
)

SET(EXTRA_SOURCES
vtkCMFEAlgorithm.h
vtkCMFEAlgorithm.cxx
vtkCMFEDesiredPoints.cxx
vtkCMFEDesiredPoints.h
vtkCMFEFastLookupGrouping.cxx
vtkCMFEFastLookupGrouping.h
vtkCMFESpatialPartition.cxx
vtkCMFESpatialPartition.h
vtkUnstructuredGridRelevantPointsFilter.cxx
vtkUnstructuredGridRelevantPointsFilter.h
vtkCMFEIntervalTree.h
vtkCMFEIntervalTree.cxx
vtkCMFEUtility.h
vtkCMFEUtility.cxx
)

ADD_PARAVIEW_PLUGIN(CMFEFilter "1.0"
  SERVER_MANAGER_SOURCES ${SERVER_SOURCES}
  SERVER_SOURCES ${EXTRA_SOURCES}
  SERVER_MANAGER_XML CMFEFilter.xml)
