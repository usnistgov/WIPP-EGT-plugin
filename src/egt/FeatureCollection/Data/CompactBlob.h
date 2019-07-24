//
// Created by gerardin on 7/24/19.
//

#ifndef NEWEGT_COMPACTBLOB_H
#define NEWEGT_COMPACTBLOB_H


#include <egt/FeatureCollection/Data/Feature.h>
#include <cmath>
#include "Blob.h"
#include <egt/FeatureCollection/Data/BoundingBox.h>
namespace egt {

    class CompactBlob {

    public:
        explicit CompactBlob(Blob *blob) {

            uint32_t
                    idFeature = 0,
                    rowMin = (uint32_t) blob->getRowMin(),
                    colMin = (uint32_t)blob->getColMin(),
                    rowMax = (uint32_t)blob->getRowMax(),
                    colMax = (uint32_t)blob->getColMax(),
                    ulRowL = 0,
                    ulColL = 0,
                    wordPosition = 0,
                    bitPositionInDecimal = 0,
                    absolutePosition = 0,
                    bitPositionInBinary = 0;

            //TODO change coords to uint32_t?
            BoundingBox boundingBox(
                    rowMin,
                    colMin,
                    rowMax,
                    colMax);

            uint32_t* bitMask = new uint32_t[(uint32_t) ceil((boundingBox.getHeight() * boundingBox.getWidth()) / 32.)]();

            // For every pixel in the bit mask
            for (auto row = (uint32_t) rowMin; row < rowMax; ++row) {
                for (auto col = (uint32_t) colMin; col < colMax; ++col) {
                    // Test if the pixel is in the current feature (using global coordinates)
                    if (blob->isPixelinFeature(row, col)) {
                        // Add it to the bit mask
                        ulRowL = row - boundingBox.getUpperLeftRow(); //convert to local coordinates
                        ulColL = col - boundingBox.getUpperLeftCol();
                        absolutePosition = ulRowL * boundingBox.getWidth() + ulColL; //to 1D array coordinates
                        //optimization : right-shifting binary representation by 5 is equivalent to dividing by 32
                        wordPosition = absolutePosition >> (uint32_t) 5;
                        //left-shifting back previous result gives the 1D array coordinates of the word beginning
                        auto beginningOfWord = ((int32_t) (wordPosition << (uint32_t) 5));
                        //subtracting original value gives the remainder of the division by 32.
                        auto remainder = ((int32_t) absolutePosition - beginningOfWord);
                        //at which position in a binary representation the bit needs to be set?
                        bitPositionInDecimal = (uint32_t) abs(32 - remainder);
                        //create a 32bit word with this bit set to 1.
                        bitPositionInBinary = ((uint32_t) 1 << (bitPositionInDecimal - (uint32_t) 1));
                        //adding the bitPosition to the word
                        bitMask[wordPosition] = bitMask[wordPosition] | bitPositionInBinary;
                    }
                }
            }

            feature = new Feature(blob->getTag(), boundingBox, (uint32_t)blob->getCount(), bitMask);

        }

        Feature *feature = nullptr;
    };

}



#endif //NEWEGT_COMPACTBLOB_H
