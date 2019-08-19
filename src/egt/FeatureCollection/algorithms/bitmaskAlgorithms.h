//
// Created by gerardin on 8/19/19.
//

#ifndef NEWEGT_BITMASKALGORITHMS_H
#define NEWEGT_BITMASKALGORITHMS_H

#include <cstdint>
#include <cmath>

namespace egt {

    class BitmaskAlgorithms {

    public :

        /// Convert an array into a bitmask
        /// \tparam T the source array resolution
        /// \param src the source array
        /// \param width the width of the source array
        /// \param height the height of the source array
        /// \param foreground the value of the foreground pixel
        /// \return
        template<class T>
        static uint32_t *arrayToBitMask(const T *src, uint32_t width, uint32_t height, T foreground) {

            auto bitMask = new uint32_t[(uint32_t) ceil((width * height) / 32.)]{0};

            // For every pixel in the source array
            for (uint64_t pos = 0; pos < width * height; pos++) {
                if (src[pos] >= foreground) {
                    addPixelToBitMask(bitMask, pos);
                }
            }

            return bitMask;
        }


        /// Test if a pixel 1D index is foreground.
        /// NOTE Boundary checks are not performed.
        /// It is the user responsability to check `pos` is valid.
        /// \param bitMask
        /// \param pos the 1D index of the pixel's position
        /// \return true if this pixel is foreground.
        static bool isBitSet(const uint32_t *bitMask, uint32_t pos) {
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

        /// Convert a bitmask into an array.
        /// \tparam T the destination array resolution
        /// \param bitMask the source bitmask
        /// \param width the size of the destination array
        /// \param height the height of the destination array
        /// \param foregroundValue the value that will represent foreground in the destination array.
        /// \return
        template<class T>
        static T *bitMaskToArray(uint32_t *bitMask, uint32_t width, uint32_t height, T foregroundValue) {

            auto array = new T[width * height]{0};

            for (uint32_t pos = 0; pos < width * height; pos++) {
                if (isBitSet(bitMask, pos)) {
                    array[pos] = foregroundValue;
                }
            }

            return array;
        }

        template<class T>
        static uint64_t copyArrayToBitmask(T *src, uint32_t *bitmask, T foregroundValue,
                                    uint32_t rowMin, uint32_t colMin, uint32_t rowMax, uint32_t colMax,
                                    uint32_t bitmaskWidth) {

            uint64_t foregroundCount = 0;

            uint32_t
                    tileWidth = colMax - colMin,
                    tileHeight = rowMax - rowMin,
                    ulRowL = 0,
                    ulColL = 0,
                    positionInBitmask = 0;

            // Copy back each foreground pixel in the view back to the bitmask.
            for (auto row = 0; row < tileHeight; ++row) {
                for (auto col = 0; col < tileWidth; ++col) {
                    // Test if the pixel is in the current feature (using global coordinates)
                    if (src[row * tileWidth + col] == foregroundValue) {
                        foregroundCount++;
                        ulRowL = row + rowMin;
                        ulColL = col + colMin;
                        positionInBitmask = ulRowL * bitmaskWidth + ulColL; //to 1D array coordinates
                        addPixelToBitMask(bitmask, positionInBitmask);
                    }
                }
            }

            return foregroundCount;
        }


    private:
        static void addPixelToBitMask(uint32_t *bitmask, uint64_t pos) {
            // Add it to the bit mask
            //optimization : right-shifting binary representation by 5 is equivalent to dividing by 32
            auto wordPosition = pos >> (uint32_t) 5;
            //left-shifting back previous result gives the 1D array coordinates of the word beginning
            auto beginningOfWord = ((int32_t) (wordPosition << (uint32_t) 5));
            //subtracting original value in decimal representation gives the remainder of the division by 32.
            auto remainder = ((int32_t) pos - beginningOfWord);
            //at which position in a binary representation the bit needs to be set?
            auto bitPositionInDecimal = (uint32_t) abs(32 - remainder);
            //create a 32bit word with this bit set to 1 by left shifting 1 as necessary
            auto bitPositionInBinary = ((uint32_t) 1 << (bitPositionInDecimal - (uint32_t) 1));
            //adding this bit to the word with and OR
            bitmask[wordPosition] = bitmask[wordPosition] | bitPositionInBinary;
        }
    };
}

#endif //NEWEGT_BITMASKALGORITHMS_H
