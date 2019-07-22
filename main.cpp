#include <iostream>
#include <egt/api/EGT.h>
#include <egt/api/DataTypes.h>
#include <egt/api/CommandLineCli.h>

int main() {


//    std::string path = "/Users/gerardin/Documents/images/egt_test/inputs/phase_image_002_tiled256_pyramid.tif";
//    std::string path = "/home/gerardin/Documents/images/egt-test-images/egt_test/inputs/phase_image_002_tiled256_pyramid.tif";
  std::string path = "/home/gerardin/Documents/images/egt-test-images/egt_test/inputs/phase_image_002_tiled256.tif";
//     std::string path = "/home/gerardin/Documents/images/egt-test-images/datasetSegmentationTest2/test2_160px_tiled64_8bit.tif";
//    std::string path = "/home/gerardin/Documents/images/egt-test-images/dataset01/images/test01-tiled.tif";
//    std::string path = "/home/gerardin/Documents/images/egt-test-images/egt_test/inputs/phase_image_002_tiled256_pyramid.tif";


//TEST DATASET 2
//       std::string path = "/Users/gerardin/Documents/images/egt_test/stitchedImage/inputs/stitched_c01t020p1_tiled1024_pyramid_lzw.ome.tif";
//    std::string path = "/home/gerardin/Documents/images/dataset2/images/tiled_stitched_c01t020p1_pyramid_1024.ome.tif";

    std::string outputPath = "/home/gerardin/CLionProjects/newEgt/outputs/";

    ImageDepth depth = egt::parseImageDepth("16U");

    auto egt = new egt::EGT();

    auto options = new egt::EGT::EGTOptions();
    options->inputPath = path;
    options->outputPath = outputPath;
    options->imageDepth = depth;
    options->nbLoaderThreads = 2;
    options->concurrentTiles = 10;


    //        uint32_t MAX_HOLE_SIZE = 10000;
//        uint32_t MIN_OBJECT_SIZE = 3000;
//        uint32_t MIN_HOLE_SIZE = 1000;
    uint32_t MAX_HOLE_SIZE = 3000;
    uint32_t MIN_OBJECT_SIZE = 20;
    uint32_t MIN_HOLE_SIZE = 10;
//        uint32_t MIN_OBJECT_SIZE = 2;
//        uint32_t MIN_HOLE_SIZE = 1;
    bool MASK_ONLY=false;

    auto segmentationOptions = new SegmentationOptions();
    segmentationOptions->MIN_HOLE_SIZE = MIN_HOLE_SIZE;
    segmentationOptions->MIN_OBJECT_SIZE = MIN_OBJECT_SIZE;
    segmentationOptions->MAX_HOLE_SIZE = MAX_HOLE_SIZE;
    segmentationOptions->MASK_ONLY = MASK_ONLY;




    //TODO should we choose the userType? We should just get it from the tif image and stick to it.
    switch (depth) {
        case ImageDepth::_32F:
            egt->run<float>(options, segmentationOptions);
            break;
        case ImageDepth::_16U:
            egt->run<uint16_t>(options, segmentationOptions);
            break;
        case ImageDepth::_8U:
            egt->run<uint8_t >(options, segmentationOptions);
            break;
    }

    delete egt;

    return 0;
}



