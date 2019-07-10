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

namespace egt {

    template <class T>
    class FCSobelFilterOpenCV : public htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>> {


    public:

        FCSobelFilterOpenCV(size_t numThreads, ImageDepth depth) : htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>> (numThreads), depth(depth) {}

        /// \brief Do the convolution on a view
        /// \param data View
        void executeTask(std::shared_ptr<htgs::MemoryData<fi::View<T>>> data) override {

            VLOG(2) << "SobelFilterOpenCV...";

            counter++;


            std::string outputPath = "/Users/gerardin/Documents/projects/wipp++/egt/outputs/";

            auto inputDepth = CV_32F;
//                    auto inputDepth = convertToOpencvType(depth);
            auto outputDepth = CV_32F;
//                    auto outputDepth = convertToOpencvType(depth);
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
            addWeighted( abs(grad_x), 0.5, abs(grad_y), 0.5, 0, grad, CV_32F );

            cv::Mat out;

//                        printArray<float>("grad",(float*)grad.data,viewWidth,viewHeight);

            grad.convertTo(out, CV_8U);

//                        printArray<uint8_t>("grad",(uint8_t*)out.data,viewWidth,viewHeight);

//                        cv::imwrite(outputPath + "segComp" + std::to_string(counter) + ".png" , out);

            VLOG(4) << grad_x.depth();
            VLOG(4) << abs_grad_x.depth();
            VLOG(4) << grad.depth();



//                        float *input = (float*)(unsigned char*)(grad.data);
//                        auto img4 = cv::Mat(viewHeight, viewWidth, CV_32F, input);

//                        cv::imwrite(outputPath + "sobelmat" + std::to_string(counter)  + ".png" , img4);


            std::vector<float> array;
            //we copy only the tile back
            if(grad.isContinuous()){
                //  tileOut = (float*)grad.data;
                array.assign((float*)grad.data, (float*)grad.data + grad.total());
            }
            else {
                for (int i = 0; i < grad.rows; ++i) {
                    array.insert(array.end(), grad.ptr<float>(i), grad.ptr<float>(i) + grad.cols);
                }
            }

//            assert(grad.rows <= height + 4);
//            assert(grad.cols <= width + 4);
//            assert(viewWidth <= width + 4);
//            assert(viewHeight <= height + 4);
//            assert((radius - 1) == 1);


//                        printArray<T>("grad",(float*)grad.data,viewWidth,viewHeight);
//                        printArray<T>("gradasarray",array.data(),viewWidth,viewHeight);


//                        auto img3 = cv::Mat(viewHeight, viewWidth, CV_32F, array.data());
//                        cv::imwrite(outputPath + "sobel" + std::to_string(counter)  + ".png" , img3);


            //TODO push to the EGTView constructor
            //auto *tileOut = new T[(width + 1) * (height + 1)]();

            float* inputArray = &array[0];
//            for (auto rangeRow = 0; rangeRow < height + 4; ++rangeRow) {
//                std::copy_n( inputArray + rangeRow * (width + 4), (width + 4), (T*)data->get()->getData() + rangeRow * (width + 4));
//            }

                std::copy_n(inputArray, viewHeight * viewWidth, (T*)data->get()->getData());

                auto row = (int)std::ceil(view->getGlobalYOffset() / view->getTileHeight());
                auto col = (int)std::ceil(view->getGlobalXOffset() / view->getTileWidth());

                        auto img5 = cv::Mat(viewHeight, viewWidth, CV_32F, (T*)data->get()->getData());
                        cv::imwrite(outputPath + "tileout" + std::to_string(row) + "," + std::to_string(col)  + ".png" , img5);




            VLOG(1) << "value at border in (5,16) in tile : "<< row << "," << col  << " : "  << view->getPixel(5,16);
            VLOG(1) << "value at border in (5,0) in tile : "<< row << "," << col  << " : "  << view->getPixel(5,0);

            // Forwarding the modified view
            this->addResult(data);
        }


        htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>> *copy() override {
            return new FCSobelFilterOpenCV(this->getNumThreads(), this->depth);
        }

        std::string getName() override { return "Sobel Filter OpenCV"; }

    private:

        ImageDepth depth = ImageDepth::_8U;
        uint32_t counter = 0;


    };
}



#endif //EGT_FCSOBELFILTEROPENCV_H
