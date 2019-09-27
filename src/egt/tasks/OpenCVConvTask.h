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

#include <opencv2/highgui/highgui.hpp>


/// \brief Convolution task with OpenCV
template<class T>
class OpenCVConvTask : public htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>> {
 private:
  cv::Mat
      _kernel;        ///< OpenCV kernel

  uint32_t
      _radiusKernel;  ///< Kernel radius

 public:
  /// \brief Create the openCV convolution task
  /// \param numThreads Number of thread for the convolution
  /// \param kernel Convolution kernel
  /// \param fig Fast Image
  /// \param radiusKernel Kernel radius
  OpenCVConvTask(size_t numThreads, cv::Mat kernel, uint32_t radiusKernel)
      : htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>>(numThreads), _kernel(std::move(kernel)), _radiusKernel(radiusKernel) {}

  /// \brief Do the convolution of the view
  /// \param data View to do the convolution
  void executeTask(std::shared_ptr<htgs::MemoryData<fi::View<T>>> data) override {
    auto
        view = data->get();
    uint32_t
        viewWidth = view->getViewWidth(),
        viewHeight = view->getViewHeight(),
        tileWidth = view->getTileWidth(),
        tileHeight = view->getViewHeight();

    uint32_t radius = view->getRadius();

    auto startRow = 2;
    auto startCol = 2;

//    egt::printArray<T>("view",view->getData(),viewWidth,viewHeight);

    auto *tileOut = new T[viewHeight * viewWidth]();

    cv::Mat
        matInput(viewHeight, viewWidth, CV_16U, view->getData()),
        tileResult;

    cv::GaussianBlur(matInput,tileResult,cv::Size(35,35), 20);
    //filter2D(matInput, tileResult, CV_32F, _kernel, cvPoint(-1, -1));

    for(auto row = 0; row < viewHeight; row++){
      for(auto col = 0; col < tileHeight; col++){
        auto res = tileResult.at<T>(
                (int) row,
                (int) col
        );
//        VLOG(2) << "res : " << res;
        auto index = row * viewWidth + col;
        tileOut[index] = res;
      }
    }

//    egt::printArray<T>("gaussian",tileOut,viewWidth,viewHeight);
//
//
////    #pragma omp parallel for
//    for (auto row = startRow; row < tileHeight + 2 * radius - startRow; ++row) {
//      for (auto col = startCol; col < tileWidth + 2 * radius - startCol; ++col) {
//        auto res = tileResult.at<float>(
//            (int) row,
//            (int) col
//        );
//        auto index = row * viewWidth + col;
//        tileOut[index] = static_cast<T>(res);
//      }
//    }

    std::copy_n(tileOut, viewHeight * viewWidth, view->getData());
    delete[] tileOut;

    // Add the tile to be added to the output file
    this->addResult(data);
  }

  /// \brief Get task name
  /// \return Task name
  std::string getName() override { return "ConvTask"; }

  /// \brief Copy the convolution task
  /// \return A new Convolution task
  OpenCVConvTask *copy() override { return new OpenCVConvTask(this->getNumThreads(), _kernel, _radiusKernel); }
};

#endif //HTGS_FAST_IMAGE_OPENCVCONVTASK_H
