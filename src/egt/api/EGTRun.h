//
// Created by gerardin on 8/23/19.
//

#ifndef NEWEGT_EGTRUN_H
#define NEWEGT_EGTRUN_H

#include <string>
#include <egt/loaders/PyramidTiledTiffLoader.h>
#include <FastImage/api/FastImage.h>
#include <egt/FeatureCollection/Tasks/BlobMerger.h>
#include <egt/FeatureCollection/Tasks/FeatureCollection.h>
#include <egt/FeatureCollection/Tasks/ViewAnalyseFilter.h>
#include "DataTypes.h"
#include <egt/tasks/SobelFilterOpenCV.h>
#include <egt/tasks/FCSobelFilterOpenCV.h>
#include <htgs/log/TaskGraphSignalHandler.hpp>
#include <egt/tasks/CustomSobelFilter3by3.h>
#include <egt/tasks/FCCustomSobelFilter3by3.h>
#include <egt/memory/TileAllocator.h>
#include <egt/FeatureCollection/Tasks/ViewFilter.h>
#include <egt/tasks/TiffTileWriter.h>
#include <egt/api/EGTOptions.h>
#include <random>
#include <egt/tasks/EGTSobelFilter.h>
#include <egt/FeatureCollection/Tasks/EGTGradientViewAnalyzer.h>
#include "DerivedSegmentationParams.h"
#include "EGT.h"
#include <experimental/filesystem>


namespace egt {


    template <class T>
    class EGTRun {

    public:

        ~EGTRun() {
            delete _runtime;
        }

        void run(EGTInput* input){
            configureAndRun();
            _taskGraph->produceData(input);
        }

        void done(){
            _taskGraph->finishedProducingData();
            _runtime->waitForRuntime();
        }


    private :


        void configureAndRun() {
            configure();
            _runtime = new htgs::TaskGraphRuntime(_taskGraph);
            _runtime->executeRuntime();
        }

        void configure() {
            _taskGraph = new htgs::TaskGraphConf<EGTInput, EGTOutput>();
            auto egt = new EGT<T>();
            _taskGraph->setGraphConsumerTask(egt);
        }

        htgs::TaskGraphConf <EGTInput, EGTOutput>* _taskGraph;
        htgs::TaskGraphRuntime * _runtime;

    };
}

#endif //NEWEGT_EGTRUN_H
