//
// Created by gerardin on 9/18/19.
//

#ifndef NEWEGT_FEATUREBUILDER_H
#define NEWEGT_FEATUREBUILDER_H

#include <htgs/api/ITask.hpp>
#include <egt/FeatureCollection/Data/BlobSet.h>

namespace egt {

    class FeatureBuilder : public htgs::ITask<BlobSet, Feature> {

    public:
        FeatureBuilder(size_t numThreads, SegmentationOptions *options) : ITask(numThreads), options(options) {}

    public:
        void executeTask(std::shared_ptr<BlobSet> data) override {

            auto blobs = data->_blobs;

            VLOG(5) << "Building feature from blob group of size : " << blobs.size();
            auto count = 0;

            for (auto blob : blobs) {
                count += blob->getCount();
            }

//            VLOG(5) << "Feature of size : " << count;
//            if(count < options->MIN_OBJECT_SIZE){
//                VLOG(5) << "Feature is too small. We delete it";
//                return;
//            }


            uint32_t id = (*blobs.begin())->getTag();

            //To merge several blobs, we calculate the resulting bounding box and fill a bitmask of the same dimensions.
            auto bb = calculateBoundingBox(blobs);
            double size = ceil((bb.getHeight() * bb.getWidth()) / 32.);
            auto *bitMask = new uint32_t[(uint32_t) size]();


            //Merge connected blobs
            //One blob is considered the parent of all the others.
            for (auto blob : blobs) {
                blob->addToBitMask(bitMask, bb);

                VLOG(5) << "deleting blob blob_" << blob->getTag();
                delete blob;
            }

            auto *feature = new Feature(id, bb, bitMask);

            this->addResult(feature);
        }

        /**
 * Calculate the tightest bounding box containing all the blobs individual bounding box.
 * @param sons
 * @return
 */
        BoundingBox calculateBoundingBox(std::set<Blob *> &sons) {

            uint32_t upperLeftRow = std::numeric_limits<int32_t>::max(),
                    upperLeftCol = std::numeric_limits<int32_t>::max(),
                    bottomRightRow = 0,
                    bottomRightCol = 0;

            for (auto son : sons) {
                auto bb = son->getFeature()->getBoundingBox();
                upperLeftRow = (bb.getUpperLeftRow() < upperLeftRow) ? bb.getUpperLeftRow() : upperLeftRow;
                upperLeftCol = (bb.getUpperLeftCol() < upperLeftCol) ? bb.getUpperLeftCol() : upperLeftCol;
                bottomRightRow = (bb.getBottomRightRow() > bottomRightRow) ? bb.getBottomRightRow() : bottomRightRow;
                bottomRightCol = bb.getBottomRightCol() > bottomRightCol ? bb.getBottomRightCol() : bottomRightCol;
            }

            return BoundingBox(upperLeftRow, upperLeftCol, bottomRightRow, bottomRightCol);
        }

        ITask<BlobSet, Feature> *copy() override {
            return new FeatureBuilder(this->getNumThreads(), this->options);
        }



        SegmentationOptions *options;
    };


}

#endif //NEWEGT_FEATUREBUILDER_H
