cmake_minimum_required(VERSION 3.12.4)
project(igxi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE igxiHpp "include/*.hpp")
file(GLOB_RECURSE igxiCpp "src/*.cpp")

# Link library

add_library(
	igxi STATIC
	${igxiHpp}
	${igxiCpp}
	CMakeLists.txt
)

target_include_directories(igxi PRIVATE include)

source_group("Headers" FILES ${igxiHpp})
source_group("Source" FILES ${igxiCpp})

if(MSVC)
    target_compile_options(igxi PRIVATE /W4 /WX /MD /MP /wd26812)
else()
    target_compile_options(igxi PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

# Optional dependency
# Implement your own file loader if you want that functionality

if(TARGET ocore)
	get_target_property(CORE2_SOURCE_DIR ocore SOURCE_DIR)
	message("-- Enabling core2 support (${CORE2_SOURCE_DIR})")
	target_include_directories(igxi PRIVATE ${CORE2_SOURCE_DIR}/include)
	target_link_libraries(igxi PRIVATE ocore)
	add_definitions(-D__USE_CORE2__)
endif()
