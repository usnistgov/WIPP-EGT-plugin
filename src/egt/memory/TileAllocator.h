//
// Created by gerardin on 4/26/19.
//

#ifndef PYRAMIDBUILDING_TILEALLOCATOR_H
#define PYRAMIDBUILDING_TILEALLOCATOR_H


#include <htgs/api/IMemoryAllocator.hpp>
#include <glog/logging.h>


namespace egt {

    template<class T>
    class TileAllocator : public htgs::IMemoryAllocator<T> {

    public:

        TileAllocator(const size_t &width, const size_t &height) : htgs::IMemoryAllocator<T>(width * height) {}


        T *memAlloc(size_t size) override {
            return new T[size]();
        }

        T *memAlloc() override {
            return new T[this->size()]();
        }

        void memFree(T *&memory) override {
            VLOG(3) << "tile data reclaimed.";
            delete[] memory;
        }

    };

}

#endif //PYRAMIDBUILDING_TILEALLOCATOR_H
