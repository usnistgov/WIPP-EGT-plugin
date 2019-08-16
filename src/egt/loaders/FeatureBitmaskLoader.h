//
// Created by gerardin on 8/16/19.
//

#ifndef NEWEGT_FEATUREBITMASKLOADER_H
#define NEWEGT_FEATUREBITMASKLOADER_H


#include <FastImage/api/ATileLoader.h>
#include <egt/FeatureCollection/Data/Feature.h>
#include <egt/utils/Utils.h>


namespace egt {

    template<typename UserType>
    class FeatureBitmaskLoader : public fi::ATileLoader<UserType> {
    public:

        explicit FeatureBitmaskLoader(const egt::Feature &feature, uint32_t tileSize, size_t numThreads = 1) :
        fi::ATileLoader<UserType>("gfdgfd", numThreads) {

            _feature = feature;

            _imageHeight = feature.getBoundingBox().getHeight();
            _imageWidth = feature.getBoundingBox().getWidth();
            _tileWidth = tileSize;
            _tileHeight = tileSize;

            //we only generate 8bits uint
            _bitsPerSample = 8;
        }

        std::string getName() override {
            return "Feature Bitmask Loader";
        }

        fi::ATileLoader<UserType> *copyTileLoader() override {
            return new FeatureBitmaskLoader(_feature, _tileWidth);
        }

        uint32_t getImageHeight(uint32_t level) const override {
            return _imageHeight;
        }

        uint32_t getImageWidth(uint32_t level) const override {
            return _imageWidth;
        }

        uint32_t getTileWidth(uint32_t level) const override {
            return _tileWidth;
        }

        uint32_t getTileHeight(uint32_t level) const override {
            return _tileHeight;
        }

        short getBitsPerSample() const override {
            return _bitsPerSample;
        }

        uint32_t getNbPyramidLevels() const override {
            return 1;
        }


        double loadTileFromFile(UserType *tile, uint32_t indexRowGlobalTile, uint32_t indexColGlobalTile) override {
            auto tileRow = indexRowGlobalTile * _tileHeight;
            auto tileCol = indexColGlobalTile * _tileWidth;

            for (uint32_t row = 0; row < _tileHeight; row++) {
                for (uint32_t col = 0; col < _tileWidth; col++) {
                    if (_feature.isBitSet(tileRow + row, tileCol + col)) {
                        tile[row * _tileWidth + col] = 1;
                    }
                    else {
                        tile[row * _tileWidth + col] = 0; //just a random piece of memory, we need to initialize it to 0
                    }
                }
            }

            return 0;
        }


        uint32_t _imageHeight,
                _imageWidth,
                _tileHeight,
                _tileWidth;

        short _bitsPerSample;

        egt::Feature _feature;


    };
}


#endif //NEWEGT_FEATUREBITMASKLOADER_H
