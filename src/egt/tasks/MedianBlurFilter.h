//
// Created by gerardin on 10/22/19.
//

#ifndef NEWEGT_MEDIANBLURFILTER_H
#define NEWEGT_MEDIANBLURFILTER_H

namespace egt {

    std::string outputPath = "/home/gerardin/Documents/projects/egt++/outputs/debug/";

/// \brief Convolution task with OpenCV
    template<class T>
    class MedianBlurFilter : public htgs::ITask<GradientView<T>, GradientView<T>> {
    private:

        uint32_t
                _radiusKernel;  ///< Kernel radius

        ImageDepth _imageDepth;

    public:
        /// \brief Create the openCV convolution task
        /// \param numThreads Number of thread for the convolution
        /// \param kernel Convolution kernel
        /// \param fig Fast Image
        /// \param radiusKernel Kernel radius
        MedianBlurFilter(size_t numThreads, uint32_t radiusKernel, ImageDepth imageDepth)
                : htgs::ITask<GradientView<T>, GradientView<T>>(numThreads),
                  _radiusKernel(radiusKernel), _imageDepth(imageDepth = ImageDepth::_16U) {}

        /// \brief Do the convolution of the view
        /// \param data View to do the convolution
        void executeTask(std::shared_ptr<GradientView<T>> data) override {

            fi::View<T>* view = data->getGradientView()->get();
            uint32_t
                    viewWidth = view->getViewWidth(),
                    viewHeight = view->getViewHeight(),
                    tileWidth = view->getTileWidth(),
                    tileHeight = view->getTileHeight();


            cv::Mat
                    matInput(viewHeight, viewWidth, convertToOpencvType(_imageDepth), view->getData()),
                    tileResult;
////            cv::GaussianBlur(matInput,tileResult,cv::Size(_radiusKernel,_radiusKernel),0,0);


//            auto kernel = cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5));
////            cv::Mat eroded;
//            cv::dilate(matInput,tileResult2,kernel);
//            cv::erode(tileResult2,tileResult3,kernel);
            cv::medianBlur(matInput, tileResult, _radiusKernel);

            cv::imwrite(outputPath + "medianBlur" + std::to_string(view->getRow()) + "-" +
                        std::to_string(view->getCol()) + ".png", tileResult);

            std::vector<T> vec(tileResult.begin<T>(), tileResult.end<T>());
            std::copy(vec.begin(), vec.end(), view->getData());
//            auto img5 = cv::Mat(viewHeight, viewWidth, convertToOpencvType(_imageDepth), view->getData());
//            cv::imwrite(outputPath + "tileout"+ std::to_string(view->getRow()) + "-" +
//                        std::to_string(view->getCol()) + ".png", img5);
//            img5.release();
            matInput.release();
            tileResult.release();

            // Add the tile to be added to the output file
            this->addResult(data);
        }

        /// \brief Get task name
        /// \return Task name
        std::string getName() override { return "Median Blur Filter Task"; }

        /// \brief Copy the convolution task
        /// \return A new Convolution task
        MedianBlurFilter *copy() override {
            return new MedianBlurFilter(this->getNumThreads(), _radiusKernel, _imageDepth);
        }
    };
}

#endif //NEWEGT_MEDIANBLURFILTER_H
