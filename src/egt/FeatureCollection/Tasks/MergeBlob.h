//
// Created by gerardin on 9/16/19.
//

#ifndef NEWEGT_MERGEBLOB_H
#define NEWEGT_MERGEBLOB_H

#include <tiffio.h>
#include <htgs/api/ITask.hpp>
#include <cstdint>
#include <utility>
#include <FastImage/FeatureCollection/tools/UnionFind.h>
#include <egt/FeatureCollection/Data/ListBlobs.h>
#include <egt/utils/FeatureExtraction.h>
#include <egt/api/DerivedSegmentationParams.h>
#include <egt/FeatureCollection/Data/ViewAnalyse.h>
#include "egt/FeatureCollection/Data/ViewAnalyzeBlock.h"

namespace egt {

class MergeBlob : public htgs::ITask<ViewAnalyseBlock, ViewAnalyse> {

public:

    explicit MergeBlob(size_t numThreads) : ITask(numThreads) {}

    void executeTask(std::shared_ptr<ViewAnalyseBlock> data) override {
        auto result = new ViewAnalyse(data->getRow(), data->getCol(), data->getLevel() + 1);

        auto views = data->getViewAnalyses();
        std::sort(views.begin(), views.end(), compareCoordinates);

        switch

        VLOG(3) << "Merge produce ViewAnalyse for level " << result->getLevel() << ": (" << result->getRow() << ", " << result->getCol() <<  "). ";
        this->addResult(result);
    }

    ITask<ViewAnalyseBlock, ViewAnalyse> *copy() override {
        return new MergeBlob(this->getNumThreads());
    }


private:

    static bool compareCoordinates (std::shared_ptr<ViewAnalyse> v1, std::shared_ptr<ViewAnalyse> v2) {
        return (v1->getRow() < v2->getRow() || v1->getCol() < v2->getCol());
    }



};

}

#endif //NEWEGT_MERGEBLOB_H
