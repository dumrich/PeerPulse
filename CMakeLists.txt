cmake_minimum_required(VERSION 3.12)
project(PeerPulse LANGUAGES CXX)

find_library(CURSES_NCURSES_LIBRARY ncurses)
find_library(CURSES_PANEL_LIBRARY panel)


# Find Boost packages (modify components as needed)
find_package(Boost 1.71 REQUIRED 
    COMPONENTS 
    system
    serialization
    filesystem
)

# Recursively find all source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.tpp"
)

# Recursively find all headers
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    "include/*.hpp"
    "include/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${CURSES_INCLUDE_DIRS}
)

# Link Boost libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    ${CURSES_NCURSES_LIBRARY}
    ${CURSES_PANEL_LIBRARY}
    ${Boost_LIBRARIES}
)

# Modern compiler flags
#target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -O2)
target_compile_options(${PROJECT_NAME} PRIVATE -O2)

# C++ standard settings
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
