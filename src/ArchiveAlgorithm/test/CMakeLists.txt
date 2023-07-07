message(STATUS "########into " ${CMAKE_CURRENT_SOURCE_DIR} "...")

message(STATUS "c compile is:" ${CMAKE_C_COMPILER})
message(STATUS "cxx compile is:" ${CMAKE_CXX_COMPILER})

set(archiveAlgorithmTest_Target archiveAlgorithmTest)

include_directories(${PROJECT_SOURCE_DIR}/src/ArchiveAlgorithm)

aux_source_directory(./ archiveAlgorithmTest_Src)
add_executable(${archiveAlgorithmTest_Target} ${archiveAlgorithmTest_Src})
target_link_libraries(${archiveAlgorithmTest_Target} ${PTHREAD_LIB})

install(TARGETS ${archiveAlgorithmTest_Target}
    RUNTIME DESTINATION  bin
    LIBRARY DESTINATION  lib
    ARCHIVE DESTINATION  lib   
    PUBLIC_HEADER DESTINATION  include
)

message(STATUS "########exit " ${CMAKE_CURRENT_SOURCE_DIR} "...")