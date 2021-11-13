cmake_minimum_required(VERSION 3.10)

set(STLVIEW_PROJECT_NAME "STLView")
project(${STLVIEW_PROJECT_NAME} VERSION 0.1 LANGUAGES CXX)

set(STLIMPORT_PATH ${CMAKE_SOURCE_DIR}/submodules/stl-import CACHE STRING "path to stl-import")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-2.4)
pkg_check_modules(GTKGLEXTMM gtkglextmm-1.2)

find_package(Eigen3 REQUIRED)

set(STLVIEW_SRC_DIR ${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE STLVIEW_H ${STLVIEW_SRC_DIR}/*.h)
file(GLOB_RECURSE STLVIEW_CPP ${STLVIEW_SRC_DIR}/*.cpp)

set(BUILD_STATIC ON CACHE BOOL "")
set(BUILD_TESTS OFF CACHE BOOL "")
set(MATHSTUFF_PATH ${STLIMPORT_PATH}/submodules/mathstuff CACHE STRING "")
set(STLUTIL_PATH ${STLIMPORT_PATH}/submodules/stlutil CACHE STRING "")
add_subdirectory(${STLIMPORT_PATH})

add_executable("stlview" ${STLVIEW_CPP})

target_include_directories("stlview" PRIVATE ${STLIMPORT_PATH}/stl_import)
target_include_directories("stlview" PUBLIC ${MATHSTUFF_PATH})
target_include_directories("stlview" PUBLIC ${STLUTIL_PATH})
target_include_directories("stlview" PUBLIC ${EIGEN3_INCLUDE_DIR})
target_include_directories("stlview" PRIVATE ${GTKMM_INCLUDE_DIRS})
target_include_directories("stlview" PRIVATE ${GTKGLEXTMM_INCLUDE_DIRS})

target_compile_options("stlview" PRIVATE -Wno-deprecated-declarations)

target_link_libraries("stlview" PUBLIC stl_import ${GTKMM_LIBRARIES} ${GTKGLEXTMM_LIBRARIES})

add_custom_command(
    TARGET "stlview" POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/STLView.png ${CMAKE_BINARY_DIR}/STLView.png
)

install(
    TARGETS stlview DESTINATION stlview/bin
)

install(
    CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/STLView.png ${CMAKE_INSTALL_PREFIX}/stlview/STLView.png)"
)