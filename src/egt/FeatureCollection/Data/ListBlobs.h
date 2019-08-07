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

    void erode(SegmentationOptions *segmentationOptions) {

        VLOG(1) << "erode feature collection";

        auto erodedFeatures = std::vector<Feature>();
        auto foregroundValue = 255;

        for(auto &blob : _blobs) {

            auto feature = blob->getFeature();

            auto width = feature->getBoundingBox().getWidth();
            auto height = feature->getBoundingBox().getHeight();

            //transform each bitMask into a array of uint8 so we can use opencv
            auto data = bitMaskToArray(feature->getBitMask(), width, height, foregroundValue);

//      //TODO remove
//      std::string outputPath = "/home/gerardin/CLionProjects/newEgt/outputs/";

            //erosion from opencv
            auto mat = cv::Mat(height, width, CV_8U,  data);
//      cv::imwrite(outputPath + "feature-" + std::to_string(feature.getId())  + ".tif" , mat);
            auto kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE,cv::Size(3,3));
            cv::Mat eroded;
            cv::erode(mat,eroded,kernel);
            mat.release();
            delete[] data;

            uint64_t maskCount = (uint64_t)countNonZero(eroded);
//      VLOG(3) << eroded;

            if(maskCount < segmentationOptions->MIN_OBJECT_SIZE){
                eroded.release();
                VLOG(3) << "delete eroded feature. Reason : feature size < " << segmentationOptions->MIN_OBJECT_SIZE;
                continue;
            }

//      cv::imwrite(outputPath + "erodedFeature-" + std::to_string(feature.getId())  + ".tif" , eroded);


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
            //TODO remove
            //printBoolArray<uint8_t>("eroded", array.data(),width,height);


            //transform back the array to a bitmask
            auto bitmask = arrayToBitMask(array.data(), width, height, foregroundValue);

            blob->setFeature(new Feature(feature->getId(), feature->getBoundingBox(), bitmask));
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


    uint32_t* arrayToBitMask(const uint8_t* src, uint32_t width, uint32_t height, uint8_t foreground) {

        auto bitMask = new uint32_t[(uint32_t) ceil((width * height) / 32.)]{0};

        // For every pixel in the bit mask
        for (auto pos = 0; pos < width * height; pos++) {
            // Test if the pixel is in the current feature (using global coordinates)
            if (src[pos] == foreground) {
                // Add it to the bit mask
                //optimization : right-shifting binary representation by 5 is equivalent to dividing by 32
                auto wordPosition = pos >> (uint32_t) 5;
                //left-shifting back previous result gives the 1D array coordinates of the word beginning
                auto beginningOfWord = ((int32_t) (wordPosition << (uint32_t) 5));
                //subtracting original value gives the remainder of the division by 32.
                auto remainder = ((int32_t) pos - beginningOfWord);
                //at which position in a binary representation the bit needs to be set?
                auto bitPositionInDecimal = (uint32_t) abs(32 - remainder);
                //create a 32bit word with this bit set to 1.
                auto bitPositionInBinary = ((uint32_t) 1 << (bitPositionInDecimal - (uint32_t) 1));
                //adding the bitPosition to the word
                bitMask[wordPosition] = bitMask[wordPosition] | bitPositionInBinary;
            }
        }

        return bitMask;
    }

    uint8_t* bitMaskToArray(uint32_t* bitMask, uint32_t width, uint32_t height, uint8_t foregroundValue) {

        auto array = new uint8_t[width * height]{0};

        for (uint32_t pos = 0; pos < width * height; pos++) {
            if (testBitMaskValue(bitMask, pos)) {
                array[pos] = foregroundValue;
            }
        }

        return array;
    }

    bool testBitMaskValue(const uint32_t* bitMask, uint32_t pos) {
        // Find the good "word" (uint32_t)
        auto wordPosition = pos >> (uint32_t) 5;
        // Find the good bit in the word
        auto bitPosition = (uint32_t) abs((int32_t) 32 - ((int32_t) pos
                                                          - (int32_t) (wordPosition << (uint32_t) 5)));
        // Test if the bit is one
        auto answer = (((((uint32_t) 1 << (bitPosition - (uint32_t) 1))
                         & bitMask[wordPosition])
                >> (bitPosition - (uint32_t) 1)) & (uint32_t) 1) == (uint32_t) 1;
        return answer;
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
