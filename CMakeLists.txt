cmake_minimum_required(VERSION 3.5) # 设置CMake所需的最低版本。如果使用的CMake版本低于该版本，则会发出致命错误
project(TuGraph C CXX) # 声明了项目的名称( TuGraph )和支持的编程语言(CXX代表C++)

message("Community version.")

# options
include(Options.cmake)

# env
set(LGRAPH_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(DEPS_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/deps/install/include)
set(DEPS_LIBRARY_DIR ${CMAKE_CURRENT_LIST_DIR}/deps/install/lib)
set(DEPS_LIBRARY64_DIR ${CMAKE_CURRENT_LIST_DIR}/deps/install/lib64)
set(LGRAPH_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)
set(CMAKEFILES_COMPILER_DIR ${PROJECT_BINARY_DIR})
set(DEPS_LOCAL_INCLUDE_DIR /usr/local/include)
set(DEPS_LOCAL_LIBRARY_DIR /usr/local/lib)

include_directories(SYSTEM ${DEPS_INCLUDE_DIR} ${DEPS_LOCAL_INCLUDE_DIR}) # just for IDE
link_directories(${DEPS_LIBRARY_DIR} ${DEPS_LIBRARY64_DIR}) # 指定链接器在链接给定target时应在其中搜索库的路径。每项(each item)可以是绝对路径或相对路径
if (OURSYSTEM STREQUAL "centos")
    link_directories(${DEPS_LIBRARY64_DIR})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    link_directories(${DEPS_LOCAL_LIBRARY_DIR})
endif ()

# output
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/output)

# target
add_subdirectory(src) # 当执行本 CMakeLists.txt 文件时，同时去执行src文件夹下的 CMakeLists.txt
add_subdirectory(test)
add_subdirectory(toolkits)
add_subdirectory(plugins)
add_subdirectory(release)

# fix dependency
set(LGRAPH_TOOLKITS lgraph_import lgraph_backup lgraph_warmup lgraph_peek lgraph_export lgraph_binlog)
add_dependencies(unit_test ${LGRAPH_TOOLKITS} lgraph_server) # 项目在构建时，先去编译 LGRAPH_TOOLKITS 指定的依赖
