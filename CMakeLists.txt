cmake_minimum_required(VERSION 3.12.4)
project(igxi)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(TARGET ignis)
	get_target_property(IGNIS_SOURCE_DIR ignis SOURCE_DIR)
	message("-- igxi: ignis found at (${IGNIS_SOURCE_DIR})")
else()
	add_subdirectory(ignis)
	set(IGNIS_SOURCE_DIR ignis)
endif()

file(GLOB_RECURSE igxiHpp "include/*.hpp")
file(GLOB_RECURSE igxiCpp "src/*.cpp")

# Link library

add_library(
	igxi STATIC
	${igxiHpp}
	${igxiCpp}
	CMakeLists.txt
)

get_target_property(CORE2_SOURCE_DIR ocore SOURCE_DIR)
target_include_directories(igxi PUBLIC ${IGNIS_SOURCE_DIR}/include)
target_include_directories(igxi PUBLIC ${CORE2_SOURCE_DIR}/include)
target_include_directories(igxi PUBLIC include)
target_link_directories(igxi PUBLIC ignis ocore)

source_group("Headers" FILES ${igxiHpp})
source_group("Source" FILES ${igxiCpp})

if(MSVC)
    target_compile_options(igxi PRIVATE /W4 /WX /MD /MP /wd26812 /wd4201 /EHsc /GR)
else()
    target_compile_options(igxi PRIVATE -Wall -Wpedantic -Wextra -Werror)
endif()
