// NIST-developed software is provided by NIST as a public service. 
// You may use, copy and distribute copies of the  software in any  medium, 
// provided that you keep intact this entire notice. You may improve, 
// modify and create derivative works of the software or any portion of the 
// software, and you may copy and distribute such modifications or works. 
// Modified works should carry a notice stating that you changed the software 
// and should note the date and nature of any such change. Please explicitly 
// acknowledge the National Institute of Standards and Technology as the 
// source of the software.
// NIST-developed software is expressly provided "AS IS." NIST MAKES NO WARRANTY
// OF ANY KIND, EXPRESS, IMPLIED, IN FACT  OR ARISING BY OPERATION OF LAW, 
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTY OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT AND DATA ACCURACY. NIST 
// NEITHER REPRESENTS NOR WARRANTS THAT THE OPERATION  OF THE SOFTWARE WILL 
// BE UNINTERRUPTED OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE CORRECTED. NIST 
// DOES NOT WARRANT  OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE 
// SOFTWARE OR THE RESULTS THEREOF, INCLUDING BUT NOT LIMITED TO THE 
// CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE SOFTWARE.
// You are solely responsible for determining the appropriateness of using 
// and distributing the software and you assume  all risks associated with 
// its use, including but not limited to the risks and costs of program 
// errors, compliance  with applicable laws, damage to or loss of data, 
// programs or equipment, and the unavailability or interruption of operation. 
// This software is not intended to be used in any situation where a failure 
// could cause risk of injury or damage to property. The software developed 
// by NIST employees is not subject to copyright protection within 
// the United States.

/// @file ListBlobs.h
/// @author Alexandre Bardakoff - Timothy Blattner
/// @date  4/6/18
/// @brief Conveniant object representing a list of blobs

#ifndef FASTIMAGE_LISTBLOBS_H
#define FASTIMAGE_LISTBLOBS_H

#include "Blob.h"
#include <egt/loaders/FeatureBitmaskLoader.h>
#include <egt/FeatureCollection/algorithms/bitmaskAlgorithms.h>
#include <egt/api/EGTOptions.h>

namespace egt {
/// \namespace fc FeatureCollection namespace

/**
 * @struct ListBlobs ListBlobs.h <FastImage/FeatureCollection/Data/ListBlobs.h>
 *
 * @brief Convenient class holding a list of blobs. Derived from IData.
 */
struct ListBlobs : public IData {
  std::list<Blob *>
      _blobs{};

  ~ListBlobs() override {
    while (!_blobs.empty()){
        delete _blobs.front();
        _blobs.pop_front();
    }
  }

    void erode(EGTOptions* options, SegmentationOptions *segmentationOptions) {

        VLOG(1) << "erode feature collection";

        //TODO CHECK HARDCODED VALUES. Should we parameterize?
        uint8_t foregroundValue = 255;
        uint64_t largeFeatureCutoff = 2048 * 2048;
        uint32_t tilesize = 1024;


        for(auto &blob : _blobs) {
            auto feature = blob->getFeature();

            auto width = feature->getBoundingBox().getWidth();
            auto height = feature->getBoundingBox().getHeight();

            uint32_t* bitMask;
            uint64_t maskCount = 0;

            //if we have a large feature, we divide the bitmask into tile and process them separately.
            if(width * height > largeFeatureCutoff) {

                VLOG(3) << "eroding a large feature of size: " << width * height << ". Let start a fast image to process it";

                //prepare bitmask for the full feature
                bitMask = new uint32_t[(uint32_t) ceil((width * height) / 32.)]{0};

                auto loader = new FeatureBitmaskLoader<int>(*feature, tilesize, foregroundValue, options->nbLoaderThreads);
                auto fi = new fi::FastImage<int>(loader,0);
                fi->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
                fi->configureAndRun();
                fi->requestAllTiles(true);
                while(fi->isGraphProcessingTiles()) {

                    auto pview = fi->getAvailableViewBlocking();
                    if (pview != nullptr) {
                        //collect data
                        auto view = pview->get();
                        auto data = view->getData();
                        auto tileHeight = view->getTileHeight();
                        auto tileWidth = view->getTileWidth();

                        //perform erosion
                        auto mat = cv::Mat(tileHeight, tileWidth, CV_8U,  data);
                        auto kernel = cv::getStructuringElement(cv::MORPH_ERODE,cv::Size(3,3), cv::Point(1,1));
                        cv::Mat eroded;
                        cv::erode(mat,eroded,kernel);
                        mat.release();
                        //transform back the matrix to an array
                        std::vector<uchar> array;
                        if (eroded.isContinuous()) {
                            array.assign((uchar*)eroded.datastart, (uchar*)eroded.dataend);
                        } else {
                            for (int i = 0; i < eroded.rows; ++i) {
                                array.insert(array.end(), eroded.ptr<uchar>(i), eroded.ptr<uchar>(i)+eroded.cols);
                            }
                        }
                        eroded.release();

                        //copy back to the bitmask
                        uint32_t
                                rowMin = (uint32_t) view->getGlobalYOffset(),
                                colMin = (uint32_t) view->getGlobalXOffset(),
                                rowMax = (uint32_t) view->getGlobalYOffset() + tileHeight,
                                colMax = (uint32_t) view->getGlobalXOffset() + tileWidth,
                                ulRowL = 0,
                                ulColL = 0,
                                wordPosition = 0,
                                bitPositionInDecimal = 0,
                                absolutePosition = 0;

                        auto erodedData = array.data();
                        auto foregroundPixelCount = BitmaskAlgorithms::copyArrayToBitmask<uint8_t>(erodedData, bitMask, foregroundValue, rowMin, colMin, rowMax, colMax, width);
                        maskCount += foregroundPixelCount;

                        pview->releaseMemory();
                    }
                }
                fi->waitForGraphComplete();
                delete fi;

                if (maskCount < segmentationOptions->MIN_OBJECT_SIZE) {
                    VLOG(3) << "delete eroded feature. Reason : feature size < "
                            << segmentationOptions->MIN_OBJECT_SIZE;
                    delete[] bitMask;
                    continue;
                }
            }
            else {
                //transform each bitMask into a array of uint8 so we can use opencv
                auto data = BitmaskAlgorithms::bitMaskToArray(feature->getBitMask(), width, height, foregroundValue);

                auto mat = cv::Mat(height, width, CV_8U, data);
                auto kernel = cv::getStructuringElement(cv::MORPH_ERODE, cv::Size(3, 3), cv::Point(1, 1));
                cv::Mat eroded;
                cv::erode(mat, eroded, kernel);
                mat.release();
                delete[] data;

                maskCount = (uint64_t) countNonZero(eroded);

                if (maskCount < segmentationOptions->MIN_OBJECT_SIZE) {
                    eroded.release();
                    VLOG(3) << "delete eroded feature. Reason : feature size < "
                            << segmentationOptions->MIN_OBJECT_SIZE;
                    continue;
                }

                //transform back the matrix to an array
                std::vector<uchar> array;
                if (eroded.isContinuous()) {
                    array.assign((uchar *) eroded.datastart, (uchar *) eroded.dataend);
                } else {
                    for (int i = 0; i < eroded.rows; ++i) {
                        array.insert(array.end(), eroded.ptr<uchar>(i), eroded.ptr<uchar>(i) + eroded.cols);
                    }
                }
                eroded.release();

                //transform back the array to a bitmask
                bitMask = BitmaskAlgorithms::arrayToBitMask(array.data(), width, height, foregroundValue);
            }

            blob->setFeature(new Feature(feature->getId(), feature->getBoundingBox(), bitMask));
            blob->setCount(maskCount);
            auto bb = feature->getBoundingBox();
            blob->setColMin(bb.getUpperLeftCol());
            blob->setRowMin(bb.getUpperLeftRow());
            blob->setColMax(bb.getBottomRightCol());
            blob->setRowMax(bb.getBottomRightRow());
            delete[] feature->getBitMask();
            delete feature;
        }
    }


    /// Filter blob objects based on size.
    /// NOTE : unused. For slight optimization we do the filtering earlier in the erode method.
    /// \param options
     void filter(SegmentationOptions* options) {

        VLOG(1) << "erode feature collection";

        auto nbBlobsTooSmall = 0;
        auto i = _blobs.begin();
        while (i != _blobs.end()) {
        //We removed objects that are still too small after the merge occured.
        if((*i)->getCount() < options->MIN_OBJECT_SIZE) {
          nbBlobsTooSmall++;
          i = _blobs.erase(i);
        }
        else {
          i++;
        }
        }
    }


};
}

#endif //FASTIMAGE_LISTBLOBS_H
