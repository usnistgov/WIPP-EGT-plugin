//
// Created by gerardin on 8/23/19.
//

#include <htgs/types/TaskGraphDotGenFlags.hpp>
#include <htgs/api/TaskGraphConf.hpp>
#include <egt/api/EGT.h>
#include <egt/api/EGTRun.h>


void main(){

    auto segmentationOptions = new egt::SegmentationOptions();
    auto options = new egt::EGTOptions();
    std::map<std::string,uint32_t> expertOptions = {};

    auto input = new egt::EGTInput(options, segmentationOptions, expertOptions);

    auto egt = new egt::EGTRun<uint16_t>();
    egt->run(input);

}
