#add_executable(thresholdFinderTest thresholdFinderTest.cpp ${SRC_FILES})
#add_executable(featureCollectionTest featureCollectionTest.cpp ${SRC_FILES})
#add_executable(thresholdFinderFromGradientTest thresholdFinderFromGradientTest.cpp ${SRC_FILES})
link_libraries(${CMAKE_THREAD_LIBS_INIT})

add_definitions(-DPROFILE)

set(TEST_SRC
        FastThresholdFinderTest.h
        bitmaskTileLoaderTest.h
        runAllTests.cpp ../../OpenCV.cpp)

add_executable(runAllTests ${TEST_SRC})
target_link_libraries(runAllTests gtest gtest_main)

