message(STATUS "#########into " ${CMAKE_CURRENT_SOURCE_DIR} "...")

message(NOTICE "c compile is:" ${CMAKE_C_COMPILER})
message(NOTICE "cxx compile is:" ${CMAKE_CXX_COMPILER})

set(utilToolsTest_Target utilToolsTest)

include_directories(${PROJECT_SOURCE_DIR}/src)
link_directories(${CMAKE_BINARY_DIR/src})
aux_source_directory(./ utilToolsTest_Src)
add_executable(${utilToolsTest_Target} ${utilToolsTest_Src})
target_link_libraries(${utilToolsTest_Target} ${PTHREAD_LIB} ${utilTools_Target})

message(STATUS "PTHREAD_LIB:" ${PTHREAD_LIB})
message(STATUS "utilTools_Target" ${utilTools_Target})

install(TARGETS ${utilToolsTest_Target}
    RUNTIME DESTINATION  bin
    LIBRARY DESTINATION  lib
    ARCHIVE DESTINATION  lib   
    PUBLIC_HEADER DESTINATION  include
)
message(STATUS "########exit " ${CMAKE_CURRENT_SOURCE_DIR} "...")