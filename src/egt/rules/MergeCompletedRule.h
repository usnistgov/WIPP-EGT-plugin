//
// Created by gerardin on 9/16/19.
//

#ifndef NEWEGT_MERGECOMPLETEDRULE_H
#define NEWEGT_MERGECOMPLETEDRULE_H

#include <egt/FeatureCollection/Data/ViewAnalyse.h>
#include <egt/utils/pyramid/Pyramid.h>


namespace egt {

    class MergeCompletedRule : public htgs::IRule<ViewAnalyse, ViewAnalyse> {

    public:

        MergeCompletedRule(pb::Pyramid &pyramid) : pyramid(pyramid) {
        }

        void applyRule(std::shared_ptr<ViewAnalyse> data, size_t pipelineId) override {

            //generated all  levels. We are done.
            if(data->getLevel() == pyramid.getNumLevel() - 1) {
                VLOG(3) << "done generating last level." << std::endl;
                this->addResult(data);
            }




        }

        pb::Pyramid pyramid;
    };




}


#endif //NEWEGT_MERGECOMPLETEDRULE_H
