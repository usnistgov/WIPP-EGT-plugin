//
// Created by gerardin on 7/17/19.
//

#ifndef NEWEGT_COMMANDLINECLI_H
#define NEWEGT_COMMANDLINECLI_H


#include "egt/api/DataTypes.h"
#include <tclap/CmdLine.h>
#include <glog/logging.h>
#include <egt/api/SegmentationOptions.h>
#include <egt/api/EGTRun.h>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;


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

void run(egt::ImageDepth imageDepth, egt::EGTOptions* &options, egt::SegmentationOptions* &segmentationOptions, std::map<std::string,uint32_t> &expertModeOptions){

    VLOG(1) << "Processing image : " << options->inputPath;

    switch (imageDepth) {
        case egt::ImageDepth::_32F: {
            auto input = new egt::EGTInput(options, segmentationOptions, expertModeOptions);
            auto egt = new egt::EGTRun<float>();
            egt->run(input);
            egt->done();
            delete egt;
            break;
        }
        case egt::ImageDepth::_16U: {
            auto input = new egt::EGTInput(options, segmentationOptions, expertModeOptions);
            auto egt = new egt::EGTRun<uint16_t>();
            egt->run(input);
            egt->done();
            delete egt;
            break;
        }
        case egt::ImageDepth::_8U: {
            auto input = new egt::EGTInput(options, segmentationOptions, expertModeOptions);
            auto egt = new egt::EGTRun<uint8_t>();
            egt->run(input);
            egt->done();
            delete egt;
            break;
        }
    }
}

int main(int argc, const char **argv) {

    try {
        TCLAP::CmdLine cmd("EGT", ' ', "1.0");

        TCLAP::ValueArg<std::string> inputPathArg("i", "images", "input images directory", true, "",
                                                       "filePath");
        cmd.add(inputPathArg);

        TCLAP::ValueArg<std::string> outputFileArg("o", "output", "output directory", true, "", "filePath");
        cmd.add(outputFileArg);

        TCLAP::ValueArg<std::string> imageDepthArg("d", "depth", "Image Depth", true, "16U", "string");
        cmd.add(imageDepthArg);

        TCLAP::ValueArg<std::uint32_t> pyramidLevelArg("l", "level", "Pyramid Level", false, 0, "uint32_t");
        cmd.add(pyramidLevelArg);

        TCLAP::ValueArg<std::uint32_t> MinHoleSizeArg("m", "minhole", "Minimum Hole Size", true, 1000, "uint32_t");
        cmd.add(MinHoleSizeArg);

        TCLAP::ValueArg<std::string> MaxHoleSizeArg("M", "maxhole", "Maximum Hole Size", false, "inf", "uint32_t or inf");
        cmd.add(MaxHoleSizeArg);

        TCLAP::ValueArg<std::uint32_t> MinObjectSizeArg("s", "minobject", "Minimum Object Size", true, 3000, "uint32_t");
        cmd.add(MinObjectSizeArg);

        TCLAP::ValueArg<bool> MaskOnlyArg("x", "maskonly", "Mask only", false, false, "bool");
        cmd.add(MaskOnlyArg);

        TCLAP::ValueArg<std::string> expertModeArg("e", "expertmode", "Expert mode", false, "", "string");
        cmd.add(expertModeArg);

        TCLAP::ValueArg<bool> labelFlag("","label","Generate a labeled mask", false, false, "bool");
        cmd.add(labelFlag);

        TCLAP::ValueArg<bool> disableIntensityFilterArg("", "disableIntensityFilter", "disable intensity filter", false, true, "bool");
        cmd.add(disableIntensityFilterArg);

        std::vector<std::string> joinOperatorsAllowed;
        joinOperatorsAllowed.emplace_back("and");
        joinOperatorsAllowed.emplace_back("or");
        TCLAP::ValuesConstraint<std::string> allowedJoinOperatorVals( joinOperatorsAllowed );
        TCLAP::ValueArg<std::string> joinOperatorArg("","op","Keep Holes Intensity Join Operator",false,"and",&allowedJoinOperatorVals);
        cmd.add( joinOperatorArg );

        TCLAP::ValueArg<std::uint32_t> minPixelIntensityPercentileArg("", "minintensity", "Min Pixel Intensity Percentile", false, 0, "uint32_t");
        cmd.add(minPixelIntensityPercentileArg);

        TCLAP::ValueArg<std::uint32_t> maxPixelIntensityPercentileArg("", "maxintensity", "Max Pixel Intensity Percentile", false, 100, "uint32_t");
        cmd.add(maxPixelIntensityPercentileArg);



        cmd.parse(argc, argv);

        std::string inputPath = inputPathArg.getValue();
        std::string outputDir = outputFileArg.getValue();
        uint32_t pyramidLevel = pyramidLevelArg.getValue();
        std::string depth = imageDepthArg.getValue();
        uint32_t minHoleSize = MinHoleSizeArg.getValue();
        std::string maxHoleSizeString = MaxHoleSizeArg.getValue();
        uint32_t minObjectSize = MinObjectSizeArg.getValue();
        bool maskOnly = MaskOnlyArg.getValue();
        std::string expertMode = expertModeArg.getValue();
        std::string joinOperatorString = joinOperatorArg.getValue();
        uint32_t minPixelIntensityPercentile = minPixelIntensityPercentileArg.getValue();
        uint32_t maxPixelIntensityPercentile = maxPixelIntensityPercentileArg.getValue();
        bool label = labelFlag.getValue();
        bool disableIntensityFilter = disableIntensityFilterArg.getValue();

        if (!hasEnding(outputDir, "/")) {
            outputDir += "/";
        }

        VLOG(1) << inputPathArg.getDescription() << ": " << inputPath << std::endl;
        VLOG(1) << outputFileArg.getDescription() << ": " << outputDir << std::endl;
        VLOG(1) << pyramidLevelArg.getDescription() << ": " << pyramidLevel << std::endl;
        VLOG(1) << MinHoleSizeArg.getDescription() << ": " << minHoleSize << std::endl;
        VLOG(1) << MaxHoleSizeArg.getDescription() << ": " << maxHoleSizeString << std::endl;
        VLOG(1) << MinObjectSizeArg.getDescription() << ": " << minObjectSize << std::endl;
        VLOG(1) << joinOperatorArg.getDescription() << ": " << joinOperatorString << std::endl;
        VLOG(1) << minPixelIntensityPercentileArg.getDescription() << ": " << minPixelIntensityPercentile << std::endl;
        VLOG(1) << maxPixelIntensityPercentileArg.getDescription() << ": " << maxPixelIntensityPercentile << std::endl;
        VLOG(1) << MaskOnlyArg.getDescription() << ": " << std::noboolalpha  << maskOnly << ":" << std::boolalpha << maskOnly << std::endl;
        VLOG(1) << labelFlag.getDescription() << ": " << std::boolalpha << label << std::endl;
        VLOG(1) << disableIntensityFilterArg.getDescription() << ": " << std::boolalpha << disableIntensityFilter << std::endl;
        VLOG(1) << expertModeArg.getDescription() << ": " << expertMode << std::endl;


        egt::ImageDepth imageDepth = egt::parseImageDepth(depth);
        uint32_t maxHoleSize = parseUint32String(maxHoleSizeString);
        egt::JoinOperator op = egt::parseJoinOperator(joinOperatorString);


        auto *options = new egt::EGTOptions();
        options->imageDepth = imageDepth;
        options->outputPath = outputDir;
        options->pyramidLevel = pyramidLevel;
        options->label = label;

        auto segmentationOptions = new egt::SegmentationOptions();
        segmentationOptions->MIN_HOLE_SIZE = minHoleSize;
        segmentationOptions->MIN_OBJECT_SIZE = minObjectSize;
        segmentationOptions->MAX_HOLE_SIZE = maxHoleSize;
        segmentationOptions->MASK_ONLY = maskOnly;
        segmentationOptions->KEEP_HOLES_WITH_JOIN_OPERATOR = op;
        segmentationOptions->MIN_PIXEL_INTENSITY_PERCENTILE = minPixelIntensityPercentile;
        segmentationOptions->MAX_PIXEL_INTENSITY_PERCENTILE = maxPixelIntensityPercentile;
        segmentationOptions->disableIntensityFilter = disableIntensityFilter;

        auto expertModeOptions = parseExpertMode(expertMode);

        if (fs::is_directory(inputPath)) {
            VLOG(1) << "Processing folder : " <<  inputPath;
            for (const auto &entry : fs::directory_iterator(inputPath)) {
                if (!hasEnding(entry.path(), ".tiff") || !hasEnding(entry.path(), ".tif")) {
                    options->inputPath = entry.path();
                    run(imageDepth, options, segmentationOptions, expertModeOptions);
                }
            }
        }
        else {
            options->inputPath = inputPath;
            run(imageDepth, options, segmentationOptions, expertModeOptions);

            delete segmentationOptions;
            delete options;
        }
    } catch (TCLAP::ArgException &e)  // catch any exceptions
    {
        DLOG(FATAL) << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }

    exit(0);

}

#endif //NEWEGT_COMMANDLINECLI_H
