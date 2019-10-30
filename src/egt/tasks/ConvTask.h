//
// Created by anb22 on 7/5/17.
//

#ifndef GABOR_CONVTASK_H
#define GABOR_CONVTASK_H

#include <htgs/api/ITask.hpp>
#include <FastImage/api/FastImage.h>
#include <FastImage/api/View.h>

/// \brief HTGS convolution task
template <class T>
class ConvTask : public htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>> {

 private:
  std::vector<double>
      _kernel;        ///< kernel vector
  int
      _radiusKernel;  ///< Radius kernel

  int _startRow{}, _startCol{};

 public:
  /// \brief Convolution task constructor
  /// \param numThreads Number of threads for the convolution task
  /// \param kernel Kernel
  /// \param fig Fast Image
  /// \param radiusKernel Radius Kernel
  ConvTask(size_t numThreads, std::vector<double> &kernel, int radiusKernel, int startRow, int startCol)
      : ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>>(numThreads), _kernel(kernel), _radiusKernel(radiusKernel), _startRow(startRow), _startCol(startCol) {}

  /// \brief Do the convolution on a view
  /// \param data View
  void executeTask(std::shared_ptr<htgs::MemoryData<fi::View<T>>> data) override {

    auto view = data->get();
    auto viewWidth = view->getViewWidth();
    auto viewHeight = view->getViewHeight();
    auto viewData = view->getData();

    int radius = view->getRadius();
    assert(radius >= _startRow && radius >= _startCol);
    auto tileWidth = view->getTileWidth();
    auto tileHeight = view->getTileHeight();

    float pixelOut;

    T* tileOut = new T[viewWidth * viewHeight]();

    // Do the convolution on the view
    for (auto row = 0; row < view->getTileHeight(); ++row) {
      for (auto col = 0; col < view->getTileWidth(); ++col) {

        pixelOut = 0;

        for (auto rR = -radius; rR < radius; ++rR) {
          for (auto cR = -radius; cR < radius; ++cR) {
            auto pixelInRow = row +rR, pixelInCol = col + cR;
            T pixelIn =  view->getPixel(pixelInRow, pixelInCol);
            pixelOut += pixelIn *
                        _kernel[(rR + radius) * (2 * radius + 1) + (cR + radius)];
          }
        }
        pixelOut /= (2 * radius + 1) * (2 * radius + 1);
        // Set the pixel into the output tile
        tileOut[row * viewWidth + col] = static_cast<T>(pixelOut);
      }
    }


//    T* tileOut = new T[viewWidth * viewHeight]();
//
//    float pixelOut = 0;
//    T pixelIn = 0;
//
//    VLOG(3) << "Gaussian Blur : (" << view->getRow() << "," << view->getCol() << ")";
//
//    auto endRow = tileHeight + 2 * radius - _startRow;
//    auto endCol = tileWidth + 2 * radius - _startCol;
//
//    for (auto row = _startRow; row < endRow; ++row) {
//      for (auto col = _startCol; col < endCol; ++col) {
//        pixelOut = 0;
//        for (auto rR = -radius; rR < radius; ++rR) {
//          for (auto cR = -radius; cR < radius; ++cR) {
//            auto pixelInRow = row;
//            auto pixelInCol = col;
//            pixelIn = view->getPixel(pixelInRow, pixelInCol);
//            VLOG(3) << "value at (" << pixelInRow << "," << pixelInCol << ") = " << pixelIn;
//
//            VLOG(3) << std::to_string(pixelIn);
//            pixelOut +=  pixelIn *
//                        _kernel[(rR + radius) * (2 * radius + 1) + (cR + radius)];
//          }
//        }
//        pixelOut /= (2 * radius + 1) * (2 * radius + 1);
//        // Set the pixel into the output tile
//        tileOut[row * viewWidth + col] = static_cast<T>(pixelOut);
//      }
//    }
//

    std::copy_n(tileOut, viewHeight * viewWidth, view->getData());
    delete[] tileOut;

    // Write the output tile
    this->addResult(data);
  }

  /// \brief Get the task name
  /// \return The task name
  std::string getName() override { return "ConvTask"; }

  /// \brief Get a copy of the convolution task
  /// \return A new convolution task
  ConvTask *copy() override {
    return new ConvTask(this->getNumThreads(), _kernel, _radiusKernel, _startRow, _startCol);
  }

};

#endif //GABOR_CONVTASK_H
