project(newEgt CXX)

cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_VERBOSE_MAKEFILE ON)

#TODO Set this flag only in debug
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -Wno-reorder -Wno-unused-parameter" )

#TODO CHECK release flags
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -funwind-tables -g")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

#TODO REMOVE TRY TO BUILD STATICALLY THIS WAY
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

#TODO DOES NOT USEFUL. REMOVE
# make_unique is not available on current OSX libc++
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
endif(APPLE)

#TODO TRY STH LIKE THAT TO STRIP LOGS
#if(PRODUCTION)
#    set(GOOGLE_STRIP_LOG 1)
#endif(PRODUCTION)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer")


# IMPORTANT to allow CMAKE to find HTGS an                  d FAST IMAGE
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")

# TODO CHECK On OSX we use Homebrew and need to additional paths or we need to symlink package files.
if(APPLE)
    set(TIFF_INCLUDE_DIR /usr/lib)
    set(CMAKE_PREFIX_PATH /usr/local/Cellar/opencv\@3/3.4.5/)
    link_directories(/usr/local/Cellar/opencv\@3/3.4.5/lib/)
    include_directories(/usr/local/Cellar/opencv\@3/3.4.5/include/)
endif(APPLE)
message(Apple : ${APPLE})

# if installed through package manager a FindTIFF.cmake is available on the path
# (ex : /usr/share/cmake-3.10/Modules/FindTIFF.cmake)
# and provide default variables such as ${TIFF_LIBRARIES}
# if installed from sources, libtiff needs to be link manually.
message(libTiff path :  ${TIFF_LIBRARIES} )

#set(OpenCV_SHARED ON)
#
##TODO Set only in debug mode
find_package(NVTX QUIET)
if(NVTX_FOUND)
    message(STATUS "FOUND NVTX!")
    add_definitions(-DUSE_NVTX)
    include_directories(${NVTX_INCLUDE_DIR})
    link_libraries(${NVTX_LIBRARIES})
endif()

# FAST IMAGE pull HTGS transitively
find_package(FastImage REQUIRED)
include_directories(${HTGS_INCLUDE_DIR})
include_directories(${FastImage_INCLUDE_DIR})
link_libraries(${FastImage_LIBRARIES})
#set(OpenCV_INSTALL_PATH "/home/geradin")
find_package(OpenCV REQUIRED)

message(FAST_IMAGE_LINK_LIBRAIRIES_ARG = ${FastImage_LIBRARIES})


#include_directories(/home/gerardin/Downloads/opencv-3.4.6/include)
#include_directories(/home/gerardin/Downloads/opencv-3.4.6-static/build/include)
#link_directories(/home/gerardin/Downloads/opencv-3.4.6/lib)

#TODO REMOVE for PROD. highgui and imgcodecs are useful for debugging (provide cv::imwrite)
link_libraries(opencv_core opencv_imgproc opencv_highgui opencv_imgcodecs)
#link_libraries(opencv_core opencv_imgproc)

#link_directories(/home/gerardin/Downloads/opencv-3.4.6-static/build/lib)
#link_directories(/home/gerardin/Downloads/opencv-3.4.6-static/build/3rdparty/lib)
#link_directories(/home/gerardin/Downloads/opencv-3.4.6-static/build/3rdparty/lib)

        #message(opencv path :  ${OpenCV_LIBRARIES} )
#message(OpenCV_INCLUDE_DIRS :  ${OpenCV_INCLUDE_DIRS} )
#link_libraries(${OpenCV_LIBRARIES})
#message("OpenCV Version: ${OpenCV_VERSION} ${OpenCV_SHARED}")

# Necessary to use experimental filesystem lib
if(APPLE)
    link_directories(/usr/local/Cellar/llvm/7.0.1/lib/)
    link_libraries(c++fs)
else(APPLE)
    link_libraries(stdc++fs)
endif(APPLE)


if(NOT PRODUCTION)
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
endif(NOT PRODUCTION)

link_libraries(glog)

#add_definitions(-DPROFILE)
#add_definitions(-DUSE_PRIORITY_QUEUE)

include_directories(src)

#TODO add only for tests
add_subdirectory(test)


set(SRC_FILES src  src/egt/FeatureCollection/Data/Feature.h src/egt/FeatureCollection/Data/BoundingBox.h src/egt/tasks/ViewToConvOutDataConverter.h src/egt/tasks/FCCustomSobelFilter3by3.h src/egt/api/SegmentationOptions.h src/egt/FeatureCollection/Data/ViewOrViewAnalyse.h  src/egt/FeatureCollection/Tasks/ViewAnalyseFilter.h src/egt/tasks/TiffTileWriter.h src/egt/FeatureCollection/Tasks/ViewFilter.h src/egt/FeatureCollection/Data/Feature.h)

add_executable(commandLineCli CommandLineCli.cpp ${SRC_FILES})
add_executable(main main.cpp ${SRC_FILES})
add_executable(generateSegmentedImage GenerateSegmentedImage.cpp ${SRC_FILES})

#target_link_libraries(commandLineCli opencv_core opencv_imgcodecs zlib)


#    find_package(OpenCV REQUIRED)
#    include_directories(${OpenCV_INCLUDE_DIRS}) # Not needed for CMake >= 2.8.11
#    target_link_libraries(MY_TARGET_NAME ${OpenCV_LIBS})
#
#if(DOCKER)
#    find_library(opencv_core /usr/local/share/)
#    target_link_libraries(commandLineCli glog "-static")
#    target_link_libraries(commandLineCli ${TIFF_LIBRARIES} "-static")
#    target_link_libraries(commandLineCli opencv_core "-static")
#endif(DOCKER)
