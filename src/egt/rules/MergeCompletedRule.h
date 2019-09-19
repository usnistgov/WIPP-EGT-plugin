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

            for(auto parentSon : data->getFinalBlobsParentSons()) {
                VLOG(3) << "level : " << data->getLevel() << " - blobset is finalized. Root : " << parentSon.first << ", Size : " << parentSon.second.size();
                this->addResult(new BlobSet(parentSon.second));
            }


            //generated all  levels. We are done.
            if(data->getLevel() == pyramid.getNumLevel() - 1) {
                VLOG(3) << "done generating last level." << std::endl;

                for(auto parentSon : data->getBlobsParentSons()) {
                    this->addResult(new BlobSet(parentSon.second));
                }
            }
        }

        pb::Pyramid pyramid;
    };




}


#endif //NEWEGT_MERGECOMPLETEDRULE_H
