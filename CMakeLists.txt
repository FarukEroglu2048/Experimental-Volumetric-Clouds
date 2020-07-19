cmake_minimum_required(VERSION 3.8)

project("Volumetric Clouds")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(XPLM_SDK_INCLUDE_DIRECTORY "SDK/CHeaders/XPLM" CACHE PATH "X-Plane SDK include directory")
set(XPLM_SDK_LIBRARY_FILE "" CACHE FILEPATH "X-Plane SDK library file (leave empty on Linux)")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

option(VOLUMETRIC_PRE_1050 "Builds Volumetric Clouds for pre-10.50 X-Plane versions" Off)

add_library(volumetric_clouds SHARED "include/dataref_helpers.hpp" "include/opengl_helpers.hpp" "src/dataref_helpers.cpp" "src/opengl_helpers.cpp" "src/volumetric_clouds.cpp")

target_include_directories(volumetric_clouds PRIVATE "include" ${XPLM_SDK_INCLUDE_DIRECTORY} GLEW::glew ${ZLIB_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
target_link_libraries(volumetric_clouds PRIVATE ${XPLM_SDK_LIBRARY_FILE} OpenGL::GL GLEW::glew ${ZLIB_LIBRARIES} ${PNG_LIBRARIES})

target_compile_definitions(volumetric_clouds PRIVATE "XPLM200=1" "XPLM210=1" "XPLM300=1" "XPLM301=1" "XPLM302=1" "XPLM_DEPRECATED=1")
if(NOT VOLUMETRIC_PRE_1050)
    message("-- Volumetric Clouds: creating X-Plane 10.50+ (Modern3D) plugin")
    target_compile_definitions(volumetric_clouds PRIVATE "XPLM303=1")
else()
    message("-- Volumetric Clouds: Creating X-Plane pre-10.50 plugin")
endif()

set_target_properties(volumetric_clouds PROPERTIES PREFIX "")
set_target_properties(volumetric_clouds PROPERTIES SUFFIX ".xpl")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(volumetric_clouds PRIVATE "NOMINMAX=1" "IBM=1" "GLEW_BUILD=1")

	set_target_properties(volumetric_clouds PROPERTIES OUTPUT_NAME "win")

	install(DIRECTORY "Volumetric Clouds" DESTINATION ${CMAKE_INSTALL_PREFIX})
	install(TARGETS volumetric_clouds RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/Volumetric Clouds")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_compile_definitions(volumetric_clouds PRIVATE "LIN=1")

	set_target_properties(volumetric_clouds PROPERTIES OUTPUT_NAME "lin")

	install(DIRECTORY "Volumetric Clouds" DESTINATION ${CMAKE_INSTALL_PREFIX})
	install(TARGETS volumetric_clouds LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Volumetric Clouds")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_compile_definitions(volumetric_clouds PRIVATE "APL=1")

	set_target_properties(volumetric_clouds PROPERTIES OUTPUT_NAME "mac")

	install(DIRECTORY "Volumetric Clouds" DESTINATION ${CMAKE_INSTALL_PREFIX})
	install(TARGETS volumetric_clouds LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/Volumetric Clouds")
endif()
