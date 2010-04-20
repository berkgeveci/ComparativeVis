cmake_minimum_required(VERSION 2.6)

project(ComparativeVis)

# -----------------------------------------------------------------------------
# Include CTest for testing
# -----------------------------------------------------------------------------
include(CTest)

option(BUILD_ENSEMBLEANALYZER "Build EnsembleAnalyzer app." OFF)

if(BUILD_ENSEMBLEANALYZER)
 add_subdirectory(EnsembleAnalyzer)
endif()

add_subdirectory(MultiFileReader)
add_subdirectory(CMFEFilter)
add_subdirectory(SQLTablePlugin)

