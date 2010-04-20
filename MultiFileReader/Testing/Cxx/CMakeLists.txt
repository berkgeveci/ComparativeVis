set(myTests
    TestXMLGenericReader.cxx)

create_test_sourcelist(Tests 
  MultiReaderCxxTests.cxx 
  ${myTests}
  EXTRA_INCLUDE vtkTestDriver.h
  )
add_executable(MultiReaderCxxTests ${Tests})
target_link_libraries(MultiReaderCxxTests MultiFileReader vtkIO)
set(TestsToRun ${Tests})
remove(TestsToRun MultiReaderCxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  add_test(${TName} ${CXX_TEST_PATH}/MultiReaderCxxTests ${TName}
    -D ${PROJECT_SOURCE_DIR}/Data
    -T ${PROJECT_BINARY_DIR}/Testing/Temporary
    -V ${PROJECT_SOURCE_DIR}/Data/Baseline/${KIT}/${TName}.png
    )
endforeach()
