PROJECT(CMFEFilter)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS} 
  )
  
IF(VTK_USE_MPI)
	INCLUDE(FindMPI)
  INCLUDE_DIRECTORIES(  
  ${MPI_INCLUDE_PATH}
  )
ENDIF(VTK_USE_MPI)

  
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
 
SET_SOURCE_FILES_PROPERTIES(
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
WRAP_EXCLUDE)


CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/vtkCMFEExport.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/vtkCMFEExport.h")

ADD_PARAVIEW_PLUGIN(CMFEFilter "1.0"
  SERVER_MANAGER_SOURCES ${SERVER_SOURCES}
  SERVER_SOURCES ${EXTRA_SOURCES}
  SERVER_MANAGER_XML CMFEFilter.xml)
