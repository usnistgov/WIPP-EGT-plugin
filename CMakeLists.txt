project(newEgt CXX)

cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_VERBOSE_MAKEFILE ON)

#TODO Set this flag only in debug
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -Wno-reorder -Wno-unused-parameter" )

#TODO CHECK release flags
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -funwind-tables -g")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")


#TODO DOES NOT USEFUL. REMOVE
# make_unique is not available on current OSX libc++
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
endif(APPLE)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer")


# IMPORTANT to allow CMAKE to find HTGS an                  d FAST IMAGE
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")

# TODO CHECK On OSX we use Homebrew and need to additional paths or we need to symlink package files.
if(APPLE)
    set(TIFF_INCLUDE_DIR /usr/lib)
    set(CMAKE_PREFIX_PATH /usr/local/opt/opencv@3/)
endif(APPLE)

# if installed through package manager a FindTIFF.cmake is available on the path
# (ex : /usr/share/cmake-3.10/Modules/FindTIFF.cmake)
# and provide default variables such as ${TIFF_LIBRARIES}
# if installed from sources, libtiff needs to be link manually.
message(libTiff path :  ${TIFF_LIBRARIES} )

#TODO Set only in debug mode
find_package(NVTX QUIET)
#if(NVTX_FOUND)
#    message(STATUS "FOUND NVTX!")
#    add_definitions(-DUSE_NVTX)
#    include_directories(${NVTX_INCLUDE_DIR})
#    link_libraries(${NVTX_LIBRARIES})
#endif()

# FAST IMAGE pull HTGS transitively
find_package(FastImage REQUIRED)
include_directories(${HTGS_INCLUDE_DIR})
include_directories(${FastImage_INCLUDE_DIR})
link_libraries(${FastImage_LIBRARIES})

find_package(OpenCV 3 REQUIRED)
link_libraries(${OpenCV_LIBRARIES} )
message("OpenCV Version: ${OpenCV_VERSION}")

# Necessary to use experimental filesystem lib
if(APPLE)
    link_directories(/usr/local/Cellar/llvm/7.0.1/lib/)
    link_libraries(c++fs)
else(APPLE)
    link_libraries(stdc++fs)
endif(APPLE)

link_libraries(glog)

add_definitions(-DPROFILE)
#add_definitions(-DUSE_PRIORITY_QUEUE)

include_directories(src)

#TODO add only for tests
add_subdirectory(test)


set(SRC_FILES src src/egt/tasks/ViewToConvOutDataConverter.h src/egt/FeatureCollection/Tasks/ConvOutDataAnalyzer.h src/egt/tasks/FCCustomSobelFilter3by3.h src/egt/FeatureCollection/Tasks/EGTViewAnalyzer.h src/egt/api/SegmentationOptions.h src/egt/FeatureCollection/Data/ViewOrViewAnalyse.h  src/egt/FeatureCollection/Tasks/ViewAnalyseFilter.h src/egt/tasks/TiffTileWriter.h src/egt/FeatureCollection/Tasks/ViewFilter.h src/egt/FeatureCollection/Data/Feature.h)



add_executable(main main.cpp ${SRC_FILES})
add_executable(commandLineCli CommandLineCli.cpp ${SRC_FILES} src/egt/FeatureCollection/Data/Feature.h src/egt/FeatureCollection/Data/BoundingBox.h)