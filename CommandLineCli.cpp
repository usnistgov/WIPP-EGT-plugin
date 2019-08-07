//
// Created by gerardin on 7/17/19.
//

#ifndef NEWEGT_COMMANDLINECLI_H
#define NEWEGT_COMMANDLINECLI_H


#include "egt/api/DataTypes.h"
#include <tclap/CmdLine.h>
#include <glog/logging.h>
#include <egt/api/SegmentationOptions.h>
#include <egt/api/EGT.h>




bool hasEnding(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

uint32_t parseUint32String( std::string &val) {
    if(val == "inf") {
        return std::numeric_limits<uint32_t >::max();
    }
    else {
        return (uint32_t)std::strtoul(val.c_str(), nullptr, 10);
    }
}

ImageDepth parseImageDepth(const std::string &depth) {
    if (depth == "16U") {
        return ImageDepth::_16U;
    } else if (depth == "8U") {
        return ImageDepth::_8U;
    } else if (depth == "32F") {
            return ImageDepth::_32F;
    } else {
        throw std::invalid_argument("image depth not recognized. Should  be one of : 8U, 16U, 32F");
    }
}

std::map<std::string,uint32_t> parseExpertMode(std::string &expertMode) {

    std::map<std::string,uint32_t> flags = {};

    std::string flagDelimiter = ";";
    std::string valueDelimiter = "=";

    size_t pos = 0;
    std::string flag;
    do {
        pos = expertMode.find(flagDelimiter);
        flag = expertMode.substr(0, pos);
        size_t pos2 = flag.find(valueDelimiter);
        if(pos2 != std::string::npos){
            auto key = flag.substr(0,pos2);
            auto value = flag.substr(pos2 + valueDelimiter.size(),std::string::npos);
            flags[key] = static_cast<uint32_t>(std::stoul(value,nullptr,10));
        }
        expertMode.erase(0, pos + flagDelimiter.length());
    }
    while(pos != std::string::npos);
    return flags;
}

int main(int argc, const char **argv) {

    try {
        TCLAP::CmdLine cmd("EGT", ' ', "1.0");

        TCLAP::ValueArg<std::string> inputFileArg("i", "images", "input images directory", true, "",
                                                       "filePath");
        cmd.add(inputFileArg);

        TCLAP::ValueArg<std::string> outputFileArg("o", "output", "output directory", true, "", "filePath");
        cmd.add(outputFileArg);

        TCLAP::ValueArg<std::string> imageDepthArg("d", "depth", "Image Depth", false, "16U", "string");
        cmd.add(imageDepthArg);

        TCLAP::ValueArg<std::uint32_t> pyramidLevelArg("l", "level", "Pyramid Level", false, 0, "uint32_t");
        cmd.add(pyramidLevelArg);

        TCLAP::ValueArg<std::uint32_t> MinHoleSizeArg("m", "minhole", "Minimum Hole Size", false, 1000, "uint32_t");
        cmd.add(MinHoleSizeArg);

        TCLAP::ValueArg<std::string> MaxHoleSizeArg("M", "maxhole", "Maximum Hole Size", false, "inf", "uint32_t or inf");
        cmd.add(MaxHoleSizeArg);

        TCLAP::ValueArg<std::uint32_t> MinObjectSizeArg("s", "minobject", "Minimum Object Size", false, 3000, "uint32_t");
        cmd.add(MinObjectSizeArg);

        TCLAP::ValueArg<bool> MaskOnlyArg("x", "maskonly", "Mask only", false, false, "bool");
        cmd.add(MaskOnlyArg);

        TCLAP::ValueArg<std::string> expertModeArg("e", "expertmode", "Expert mode", false, "", "string");
        cmd.add(expertModeArg);

        cmd.parse(argc, argv);

        std::string inputFile = inputFileArg.getValue();
        std::string outputDir = outputFileArg.getValue();
        uint32_t pyramidLevel = pyramidLevelArg.getValue();
        std::string depth = imageDepthArg.getValue();
        uint32_t minHoleSize = MinHoleSizeArg.getValue();
        std::string maxHoleSizeString = MaxHoleSizeArg.getValue();
        uint32_t minObjectSize = MinObjectSizeArg.getValue();
        bool maskOnly = MaskOnlyArg.getValue();
        std::string expertMode = expertModeArg.getValue();

        if (!hasEnding(outputDir, "/")) {
            outputDir += "/";
        }

        VLOG(1) << inputFileArg.getDescription() << ": " << inputFile << std::endl;
        VLOG(1) << outputFileArg.getDescription() << ": " << outputDir << std::endl;
        VLOG(1) << pyramidLevelArg.getDescription() << ": " << pyramidLevel << std::endl;
        VLOG(1) << MinHoleSizeArg.getDescription() << ": " << minHoleSize << std::endl;
        VLOG(1) << MaxHoleSizeArg.getDescription() << ": " << maxHoleSizeString << std::endl;
        VLOG(1) << MinObjectSizeArg.getDescription() << ": " << minObjectSize << std::endl;
        VLOG(1) << MaskOnlyArg.getDescription() << ": " << std::noboolalpha  << maskOnly << ":" << std::boolalpha << maskOnly << std::endl;
        VLOG(1) << expertModeArg.getDescription() << ": " << expertMode << std::endl;

        ImageDepth imageDepth = parseImageDepth(depth);
        uint32_t maxHoleSize = parseUint32String(maxHoleSizeString);


        auto *options = new egt::EGT::EGTOptions();
        options->imageDepth = imageDepth;
        options->inputPath = inputFile;
        options->outputPath = outputDir;
        options->pyramidLevel = pyramidLevel;

        auto segmentationOptions = new SegmentationOptions();
        segmentationOptions->MIN_HOLE_SIZE = minHoleSize;
        segmentationOptions->MIN_OBJECT_SIZE = minObjectSize;
        segmentationOptions->MAX_HOLE_SIZE = maxHoleSize;
        segmentationOptions->MASK_ONLY = maskOnly;


        auto expertModeOptions = parseExpertMode(expertMode);

        auto egt = new egt::EGT();

        switch (imageDepth) {
            case ImageDepth::_32F:
                egt->run<float>(options, segmentationOptions, expertModeOptions);
                break;
            case ImageDepth::_16U:
                egt->run<uint16_t>(options, segmentationOptions, expertModeOptions);
                break;
            case ImageDepth::_8U:
                egt->run<uint8_t >(options, segmentationOptions, expertModeOptions);
                break;
        }

        delete egt;

    } catch (TCLAP::ArgException &e)  // catch any exceptions
    {
        DLOG(FATAL) << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }

    exit(0);

}

#endif //NEWEGT_COMMANDLINECLI_H
