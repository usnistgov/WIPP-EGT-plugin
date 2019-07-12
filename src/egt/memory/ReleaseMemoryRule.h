//
// Created by gerardin on 3/28/19.
//

#ifndef PYRAMIDBUILDING_RELEASETILEMEMORYRULE_H
#define PYRAMIDBUILDING_RELEASETILEMEMORYRULE_H

#include <htgs/api/IMemoryReleaseRule.hpp>
#include <glob.h>

namespace egt {

    class ReleaseMemoryRule : public htgs::IMemoryReleaseRule {

    public:

        ReleaseMemoryRule(size_t releaseCount) : releaseCount(releaseCount) {}

        void memoryUsed() override {
            releaseCount--;
        }

        bool canReleaseMemory() override {
            return releaseCount == 0;
        }




    private:
        size_t releaseCount;

    };


}


#endif //PYRAMIDBUILDING_RELEASETILEMEMORYRULE_H
