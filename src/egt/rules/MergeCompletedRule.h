//
// Created by gerardin on 9/16/19.
//

#ifndef NEWEGT_MERGECOMPLETEDRULE_H
#define NEWEGT_MERGECOMPLETEDRULE_H

#include <egt/FeatureCollection/Data/ViewAnalyse.h>
#include <egt/utils/pyramid/Pyramid.h>
#include <egt/FeatureCollection/Data/BlobSet.h>


namespace egt {

    class MergeCompletedRule : public htgs::IRule<ViewAnalyse, BlobSet> {

    public:

        explicit MergeCompletedRule(pb::Pyramid &pyramid) : pyramid(pyramid) {
        }



        void applyRule(std::shared_ptr<ViewAnalyse> data, size_t pipelineId) override {

            VLOG(4) << "level : " << data->getLevel() << "(" << data->getRow() << "," << data->getCol() << ")" << " - merge completed " << ", producing : " << data->getFinalBlobsParentSons().size() << " features...";

            trackNbOfFeatures(data);

            for(auto parentSon : data->getFinalBlobsParentSons()) {
                VLOG(5) << "level : " << data->getLevel() << "(" << data->getRow() << "," << data->getCol() << ")" << " - blobset is finalized. Root : blob_" << parentSon.first->getId() << ", Size : " << parentSon.second.size();
                this->addResult(new BlobSet(parentSon.second));
            }

            //generated all  levels. We are done.
            if(data->getLevel() == pyramid.getNumLevel() - 1) {
                VLOG(4) << "done generating last level." << std::endl;
                VLOG(4) << "blobs that have not been finalized : " << data->getBlobsParentSons().size();

                printTrackingInfo();

//                for(auto parentSon : data->getBlobsParentSons()) {
//                    this->addResult(new BlobSet(parentSon.second));
//                }
            }
        }

        void trackNbOfFeatures(std::shared_ptr<ViewAnalyse> data) {
            auto oldCount = featuresPerLevel[data->getLevel()];
            auto newCount = oldCount + data->getFinalBlobsParentSons().size();
            featuresPerLevel[data->getLevel()] = newCount;
            auto oldBlobCount = blobsPerLevel[data->getLevel()];
            auto nbBlobs = 0;
            for(auto blobEntry : data->getFinalBlobsParentSons()){
                nbBlobs += blobEntry.second.size();
            }
            blobsPerLevel[data->getLevel()] = oldBlobCount + nbBlobs;
        }


        void printTrackingInfo() {
            VLOG(2) << "Number of features per level: ";
            for(auto fCount : featuresPerLevel){
                VLOG(2) << "level " << fCount.first  << " : " <<  fCount.second;
            }
            VLOG(2) << "Total number of blobs per level: ";
            for(auto bCount : blobsPerLevel){
                VLOG(2) << "level " << bCount.first  << " : " <<  bCount.second;
            }
        }

        pb::Pyramid pyramid;
        std::map<uint32_t,size_t> featuresPerLevel{};
        std::map<uint32_t,size_t> blobsPerLevel{};
    };






};


#endif //NEWEGT_MERGECOMPLETEDRULE_H
