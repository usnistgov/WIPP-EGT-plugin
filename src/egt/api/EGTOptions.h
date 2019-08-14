//
// Created by gerardin on 8/7/19.
//

#ifndef NEWEGT_EGTOPTIONS_H
#define NEWEGT_EGTOPTIONS_H

#include "DataTypes.h"

namespace egt {

    class EGTOptions {

    public:
        std::string inputPath{};
        std::string outputPath{};

        ImageDepth imageDepth = ImageDepth::_8U;
        uint32_t pyramidLevel = 0;
        uint32_t pixelIntensityBoundsLevelUp = 0;

        size_t nbLoaderThreads{};
        uint32_t concurrentTiles{};

        int32_t nbTilePerSample = -1;
        int32_t nbExperiments = -1;

        int32_t threshold{};

        bool label{};

        int rank{};

    };
}

#endif //NEWEGT_EGTOPTIONS_H
