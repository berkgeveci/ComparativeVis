IF (QT_USE_FILE)
  INCLUDE(${QT_USE_FILE})
ENDIF (QT_USE_FILE)

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS}
  )

# We need to wrap for Qt stuff such as signals/slots etc. to work correctly.
QT4_WRAP_CPP(MOC_SRCS SQLToolbarActions.h)

# This is a macro for adding QActionGroup subclasses automatically as toolbars.
ADD_PARAVIEW_ACTION_GROUP(IFACES IFACE_SRCS 
  CLASS_NAME SQLToolbarActions
  GROUP_NAME "MenuBar/File")

ADD_PARAVIEW_PLUGIN(SQLToolbar "1.0"
  SERVER_MANAGER_XML
    SQLToolbar.xml
  GUI_INTERFACES ${IFACES}
  SERVER_MANAGER_SOURCES
    vtkSQLiteTableReader.h vtkSQLiteTableReader.cxx
    vtkMySQLTableReader.h vtkMySQLTableReader.cxx
  SOURCES ${MOC_SRCS} ${IFACE_SRCS} 
  SQLToolbarActions.cxx)

ADD_EXECUTABLE(tableReader main.cxx
  vtkSQLTableReader.h vtkSQLTableReader.cxx
  vtkMySQLTableReader.h vtkMySQLTableReader.cxx)
TARGET_LINK_LIBRARIES(tableReader vtkCommon vtkFiltering vtkIO)
