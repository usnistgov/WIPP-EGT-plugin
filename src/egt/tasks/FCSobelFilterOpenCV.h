//
// Created by Gerardin, Antoine D. (Assoc) on 2019-05-13.
//

#ifndef EGT_FCSOBELFILTEROPENCV_H
#define EGT_FCSOBELFILTEROPENCV_H
//
// Created by gerardin on 5/7/19.
//

#include <FastImage/api/View.h>
#include <htgs/api/ITask.hpp>
#include <egt/data/ConvOutData.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core.hpp>
#include <opencv/cv.hpp>
#include <egt/api/DataTypes.h>
#include <egt/data/ThresholdOrView.h>
#include <egt/utils/Utils.h>
#include <egt/api/DataTypes.h>

namespace egt {

    template <class T>
    class FCSobelFilterOpenCV : public htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>> {


    public:

        FCSobelFilterOpenCV(size_t numThreads, ImageDepth depth) : htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>> (numThreads), _depth(depth) {}

        /// \brief Do the convolution on a view
        /// \param data View
        void executeTask(std::shared_ptr<htgs::MemoryData<fi::View<T>>> data) override {

            VLOG(2) << "SobelFilterOpenCV...";

            counter++;

            std::string outputPath = "/home/gerardin/CLionProjects/newEgt/outputs/";
//            std::string outputPath = "/Users/gerardin/Documents/projects/wipp++/egt/outputs/";

            auto inputDepth = convertToOpencvType(_depth);
            auto outputDepth = convertToOpencvType(_depth);

            int scale = 1;
            int delta = 0;

            auto view = data->get();
            auto viewWidth = view->getViewWidth();
            auto viewHeight = view->getViewHeight();
            auto width = view->getTileWidth();
            auto height = view->getTileHeight();
            auto radius = view->getRadius();


//                    printArray<T>("view",view->getData(),viewWidth,viewHeight);

            auto srcGray = cv::Mat(viewHeight, viewWidth, inputDepth, view->getData());

            VLOG(4) << srcGray.depth();

            /// Generate grad_x and grad_y
            cv::Mat grad_x, grad_y;
            cv::Mat abs_grad_x, abs_grad_y;
            cv::Mat grad;
            /// Gradient X
//                    cv::Scharr( floatSrcGray, grad_x, outputDepth, 1, 0, scale, delta, cv::BORDER_REFLECT );
            Sobel(srcGray , grad_x, outputDepth, 1, 0, 3, scale, delta, cv::BORDER_DEFAULT );
//                        convertScaleAbs( grad_x, abs_grad_x );
            abs(grad_x);

            /// Gradient Y
//                    cv::Scharr( floatSrcGray, grad_y, outputDepth, 0, 1, scale, delta, cv::BORDER_REFLECT );
            Sobel( srcGray, grad_y, outputDepth, 0, 1, 3, scale, delta, cv::BORDER_DEFAULT );
            abs(grad_y);
//                        convertScaleAbs( grad_y, abs_grad_y );

            /// Total Gradient (approximate)
//                        addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad );
            addWeighted( abs(grad_x), 0.5, abs(grad_y), 0.5, 0, grad, outputDepth );

//            cv::Mat out;

//                        printArray<float>("grad",(T*)grad.data,viewWidth,viewHeight);

//            grad.convertTo(out, CV_8U);

//                        printArray<uint8_t>("grad",(uint8_t*)out.data,viewWidth,viewHeight);

//                        cv::imwrite(outputPath + "segComp" + std::to_string(counter) + ".png" , out);

            VLOG(5) << grad_x.depth();
            VLOG(5) << abs_grad_x.depth();
            VLOG(5) << grad.depth();



//                        float *input = (T*)(unsigned char*)(grad.data);
//                        auto img4 = cv::Mat(viewHeight, viewWidth, CV_32F, input);

//                        cv::imwrite(outputPath + "sobelmat" + std::to_string(counter)  + ".png" , img4);


            std::vector<T> array;
            //we copy only the tile back
            if(grad.isContinuous()){
                //  tileOut = (float*)grad.data;
                array.assign((T*)grad.data, (T*)grad.data + grad.total());
            }
            else {
                for (int i = 0; i < grad.rows; ++i) {
                    array.insert(array.end(), grad.ptr<T>(i), grad.ptr<T>(i) + grad.cols);
                }
            }



//                        printArray<T>("grad",(T*)grad.data,viewWidth,viewHeight);
//                        printArray<T>("gradasarray",array.data(),viewWidth,viewHeight);


//                        auto img3 = cv::Mat(viewHeight, viewWidth, CV_32F, array.data());
//                        cv::imwrite(outputPath + "sobel" + std::to_string(counter)  + ".png" , img3);


            //TODO push to the EGTView constructor
            //auto *tileOut = new T[(width + 1) * (height + 1)]();

            T* inputArray = &array[0];
//            for (auto rangeRow = 0; rangeRow < height + 4; ++rangeRow) {
//                std::copy_n( inputArray + rangeRow * (width + 4), (width + 4), (T*)data->get()->getData() + rangeRow * (width + 4));
//            }

                std::copy_n(inputArray, viewHeight * viewWidth, (T*)data->get()->getData());

                auto row = (int)std::ceil(view->getGlobalYOffset() / view->getTileHeight());
                auto col = (int)std::ceil(view->getGlobalXOffset() / view->getTileWidth());

//FOR DEBUGGING
                auto img5 = cv::Mat(viewHeight, viewWidth, outputDepth, (T*)data->get()->getData());
                cv::imwrite(outputPath + "tileout" + std::to_string(row) + "," + std::to_string(col)  + ".tiff" , img5);
                img5.release();


            // Forwarding the modified view
            this->addResult(data);
        }


        htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>> *copy() override {
            return new FCSobelFilterOpenCV(this->getNumThreads(), this->_depth);
        }

        std::string getName() override { return "Sobel Filter OpenCV"; }

    private:

        ImageDepth _depth = ImageDepth::_8U;
        uint32_t counter = 0;


    };
}



#endif //EGT_FCSOBELFILTEROPENCV_H
