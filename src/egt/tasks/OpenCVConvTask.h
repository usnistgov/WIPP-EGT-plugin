//
// Created by anb22 on 12/15/17.
//

#ifndef HTGS_FAST_IMAGE_OPENCVCONVTASK_H
#define HTGS_FAST_IMAGE_OPENCVCONVTASK_H

#include <utility>

#include <opencv2/opencv.hpp>

#include <htgs/api/MemoryData.hpp>
#include <htgs/api/ITask.hpp>
#include <FastImage/api/FastImage.h>
#include <egt/utils/Utils.h>
#include <egt/api/DataTypes.h>

#include <opencv2/highgui/highgui.hpp>

namespace egt {

    std::string outputPath = "/home/gerardin/Documents/projects/egt++/outputs/debug/";

/// \brief Convolution task with OpenCV
    template<class T>
    class OpenCVConvTask : public htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>> {
    private:
        cv::Mat
                _kernel;        ///< OpenCV kernel

        uint32_t
                _radiusKernel;  ///< Kernel radius

        ImageDepth _imageDepth;

    public:
        /// \brief Create the openCV convolution task
        /// \param numThreads Number of thread for the convolution
        /// \param kernel Convolution kernel
        /// \param fig Fast Image
        /// \param radiusKernel Kernel radius
        OpenCVConvTask(size_t numThreads, uint32_t radiusKernel, ImageDepth imageDepth)
                : htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>>(numThreads),
                  _radiusKernel(radiusKernel), _imageDepth(imageDepth) {}

        /// \brief Do the convolution of the view
        /// \param data View to do the convolution
        void executeTask(std::shared_ptr<htgs::MemoryData<fi::View<T>>> data) override {
            auto
                    view = data->get();
            uint32_t
                    viewWidth = view->getViewWidth(),
                    viewHeight = view->getViewHeight(),
                    tileWidth = view->getTileWidth(),
                    tileHeight = view->getTileHeight();

            cv::Mat
                    matInput(viewHeight, viewWidth, convertToOpencvType(_imageDepth), view->getData()),
                    tileResult;
//            cv::GaussianBlur(matInput,tileResult,cv::Size(_radiusKernel,_radiusKernel),0,0);
            cv::medianBlur(matInput, tileResult, _radiusKernel);

            cv::imwrite(outputPath + "medianBlur" + std::to_string(view->getRow()) + "-" +
                        std::to_string(view->getCol()) + ".png", tileResult);

            std::vector<T>vec(tileResult.begin<T>(), tileResult.end<T>());
            std::copy(vec.begin(), vec.end(), view->getData());
            auto img5 = cv::Mat(viewHeight, viewWidth, convertToOpencvType(_imageDepth), view->getData());
            cv::imwrite(outputPath + "tileout"+ std::to_string(view->getRow()) + "-" +
                                     std::to_string(view->getCol()) + ".png", img5);
            img5.release();

            // Add the tile to be added to the output file
            this->addResult(data);
        }

        /// \brief Get task name
        /// \return Task name
        std::string getName() override { return "ConvTask"; }

        /// \brief Copy the convolution task
        /// \return A new Convolution task
        OpenCVConvTask *copy() override {
            return new OpenCVConvTask(this->getNumThreads(), _radiusKernel, _imageDepth);
        }
    };

#endif //HTGS_FAST_IMAGE_OPENCVCONVTASK_H
}