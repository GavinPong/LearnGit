message(STATUS "#####into " ${CMAKE_CURRENT_SOURCE_DIR} "...")

message(STATUS "c compile is:" ${CMAKE_C_COMPILER})
message(STATUS "cxx compile is:" ${CMAKE_CXX_COMPILER})

## 生成vs工程时，会在对应项目下面生成一个include文件夹 ##
file(GLOB CURRENT_HEADERS  *.h *.hpp)
source_group("Include" FILES ${CURRENT_HEADERS})

set(utilTools_Target utilTools CACHE STRING INTERNAL)

aux_source_directory(./ utilTools_Src)
add_library(${utilTools_Target} ${utilTools_Src} ${CURRENT_HEADERS})
target_link_libraries(${utilTools_Target} ${PTHREAD_LIB})

#install(TARGETS ${utilTools_Target}  DESTINATION  lib)
install(TARGETS ${utilTools_Target}
    RUNTIME DESTINATION  bin
    LIBRARY DESTINATION  lib
    ARCHIVE DESTINATION  lib   
    PUBLIC_HEADER DESTINATION  include
)

add_subdirectory(ArchiveAlgorithm)

message(STATUS "#####exit " ${CMAKE_CURRENT_SOURCE_DIR} "...")