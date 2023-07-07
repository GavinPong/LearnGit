if(0)
message(STATUS "#######into " ${CMAKE_CURRENT_SOURCE_DIR} "...")

message(STATUS "c compile is:" ${CMAKE_C_COMPILER})
message(STATUS "cxx compile is:" ${CMAKE_CXX_COMPILER})

set(archiveAlgorithm_Target archiveAlgorithm)

aux_source_directory(./ archiveAlgorithm_Src)
add_library(${archiveAlgorithm_Target} ${archiveAlgorithm_Src})
target_link_libraries(${archiveAlgorithm_Target} ${PTHREAD_LIB})

install(TARGETS ${archiveAlgorithm_Target}
    RUNTIME DESTINATION  bin
    LIBRARY DESTINATION  lib
    ARCHIVE DESTINATION  lib   
    PUBLIC_HEADER DESTINATION  include
)
endif()


if(UTIL_TOOLS_BUILD_TESTS)
    add_subdirectory(test)
else()
    message(STATUS "UTIL_TOOLS_BUILD_TESTS is OFF")
endif()


message(STATUS "#######exit " ${CMAKE_CURRENT_SOURCE_DIR} "...")