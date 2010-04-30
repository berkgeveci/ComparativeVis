cmake_minimum_required(VERSION 2.6)

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})

set(BUILD_SHARED_LIBS "ON")

set(MULTIFILE_READER_SRCS
    vtkFileCollectionReader.cxx
    vtkMyDelimitedTextReader.cxx
    vtkSMIntRangeHelper.cxx)

add_paraview_plugin(MultiFileReader "1.0"
  SERVER_MANAGER_XML MultiFileReader.xml
  SERVER_MANAGER_SOURCES ${MULTIFILE_READER_SRCS}
)
set_target_properties(MultiFileReader PROPERTIES PREFIX "")

wrap_plugin_for_python(MultiFileReader ${MULTIFILE_READER_SRCS}) 

if (PARAVIEW_BUILD_QT_GUI)

  qt4_wrap_ui(UI_BUILT_SOURCES
    pqCVColumnChooser.ui
  )

  set (MULTIFILE_READER_GUI_SOURCES
    ${UI_BUILT_SOURCES}
    pqCVPluginManager.cxx
    pqCVActionsGroup.cxx
    )

  qt4_wrap_cpp(MOC_SRCS
    pqCVPluginManager.h
    pqCVActionsGroup.h
  )

  add_paraview_action_group(
    PARAVIEW_PLUGIN_IFACES
    PARAVIEW_PLUGIN_SRCS
    CLASS_NAME pqCVActionsGroup
    GROUP_NAME "MenuBar/Comparative Vis")

  add_paraview_auto_start(
    PARAVIEW_PLUGIN_IFACES2
    PARAVIEW_PLUGIN_SRCS2
    CLASS_NAME pqCVPluginManager)

  add_paraview_plugin(MultiFileReaderGUI "1.0"
    GUI_SOURCES
      ${MULTIFILE_READER_GUI_SOURCES}
      ${MOC_SRCS}
      ${PARAVIEW_PLUGIN_SRCS}
      ${PARAVIEW_PLUGIN_SRCS2}
    GUI_INTERFACES
      ${PARAVIEW_PLUGIN_IFACES}
      ${PARAVIEW_PLUGIN_IFACES2}
    )

  set_target_properties(MultiFileReaderGUI PROPERTIES PREFIX "")
endif (PARAVIEW_BUILD_QT_GUI)

#add_executable(TestStats TestStats.cxx)
#target_link_libraries(TestStats MultiFileReader vtkIO)

# -----------------------------------------------------------------------------
# Testing
# -----------------------------------------------------------------------------
if(BUILD_TESTING)
 add_subdirectory(Testing)
endif()
