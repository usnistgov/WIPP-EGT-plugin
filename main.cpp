//
// Created by gerardin on 8/23/19.
//

#include <htgs/types/TaskGraphDotGenFlags.hpp>
#include <htgs/api/TaskGraphConf.hpp>
#include <egt/api/EGT.h>
#include <egt/api/EGTRun.h>


int main(int argc, const char **argv) {
//-i "/home/gerardin/Documents/images/egt-test-images/egt_test/inputs/phase_image_002_tiled256_pyramid.tif"
// -o "/home/gerardin/CLionProjects/newEgt/outputs/"
// -d "16U" --level "0" --minhole "100" --maxhole "inf" --minobject "100" -x "0"
// --op "and" --minintensity "0" --maxintensity "100" -e "loader=2;tile=10;threshold=108"
    auto segmentationOptions = new egt::SegmentationOptions();
    auto options = new egt::EGTOptions();
    std::map<std::string,uint32_t> expertOptions = {};
    options->inputPath = "/home/gerardin/Documents/images/egt-test-images/egt_test/inputs/phase_image_002_tiled256_pyramid.tif";
    options->outputPath = "/home/gerardin/CLionProjects/newEgt/outputs/";
    segmentationOptions->MIN_OBJECT_SIZE = 100;
    segmentationOptions->MAX_HOLE_SIZE = std::numeric_limits<uint16_t >::max();
    segmentationOptions->MIN_HOLE_SIZE = 100;
    segmentationOptions->disableIntensityFilter = true;

    auto input = new egt::EGTInput(options, segmentationOptions, expertOptions);


    auto egt = new egt::EGTRun<uint16_t>();
    egt->run(input);
    egt->done();
    delete egt;
    delete segmentationOptions;
    delete options;
    VLOG(3) << "done";

}
