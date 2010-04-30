cmake_minimum_required(VERSION 2.8.1)

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})



# Give user option of building shared or static libraries.  Default to
# the choice made for VTK.
option(BUILD_SHARED_LIBS "Build with shared libraries."
       ${VTK_BUILD_SHARED_LIBS})

#add_library(vtkDatabaseConnection
#            vtkDatabaseConnection.h vtkDatabaseConnection.cxx)
#target_link_libraries(vtkDatabaseConnection vtkIO)


set(STATDATA_PATH ${CMAKE_CURRENT_SOURCE_DIR}/data CACHE PATH
    "Full path to the location of the data.")
add_definitions(-DSTATDATA_PATH="${STATDATA_PATH}")

add_executable(TestStats TestStats.cxx)
target_link_libraries(TestStats vtkMetadataBrowser MultiFileReader)

message("PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}")
message("PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}")

include_directories("${PROJECT_SOURCE_DIR}/EnsembleAnalyzer" "${PROJECT_SOURCE_DIR}/MultiFileReader")
