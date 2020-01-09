//
// Created by gerardin on 11/4/19.
//

#ifndef NEWEGT_BITMASK_H
#define NEWEGT_BITMASK_H

#include <cstdint>
#include <cmath>

template<class T>
class Bitmask {

public:
/**
 * Create a bitwise bitmask from an binary array
 * @param array
 * @param width
 * @param height
 * @return
 */
static uint32_t *createBitmask(T *array, uint32_t width, uint32_t height) {

    auto wordPosition = 0;
    auto bitPosition = 0;
    //finding bitmask size
    auto bitmaskSize = (uint32_t) ceil(
            (width * height) / 32.);
    auto bitMask = new uint32_t[bitmaskSize]();

    for (uint32_t wordIndex = 0; wordIndex < bitmaskSize; wordIndex++) {
        bitMask[wordIndex] = 0;
    }

    // For every pixel in the bit mask
    for (uint32_t row = 0; row < height; ++row) {
        for (uint32_t col = 0; col < width; ++col) {
            auto absolutePosition = row * width + col;
            if (array[absolutePosition] == 1) {
                // Add it to the bit mask
                wordPosition = absolutePosition >> (uint32_t) 5;
                bitPosition =
                        (uint32_t) abs(32 - ((int32_t) absolutePosition
                                             - ((int32_t) (wordPosition << (uint32_t) 5))));
                bitMask[wordPosition] = bitMask[wordPosition]
                                        | ((uint32_t) 1 << (bitPosition - (uint32_t) 1));
            }
        }
    }

    return bitMask;
}
};

#endif //NEWEGT_BITMASK_H
