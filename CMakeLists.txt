cmake_minimum_required(VERSION 3.0)

project(vmecs)

# set basic compile flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g -Wall -pedantic -Wno-address -Wno-unused-function")

# add common headers
include_directories(${PROJECT_SOURCE_DIR})

message(STATUS "Compile using flags: '${CMAKE_C_FLAGS}'")

macro(add_exe_batch exe_name)
	file(GLOB c_list ${ARGN})
	add_executable(${exe_name} ${c_list})
	install(TARGETS ${exe_name} DESTINATION bin)
endmacro()

macro(add_lib_batch lib_name linkage)
	file(GLOB c_list ${ARGN})
	add_library(${lib_name} ${linkage} ${c_list})
	install(TARGETS ${lib_name} DESTINATION lib)
endmacro()

add_subdirectory(app)
add_subdirectory(proto)
add_subdirectory(crypto)
add_subdirectory(pub)
