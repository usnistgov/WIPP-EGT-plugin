//
// Created by gerardin on 7/19/19.
//

#ifndef NEWEGT_TILETIFFWRITER_H
#define NEWEGT_TILETIFFWRITER_H

#include <FastImage/api/View.h>
#include <htgs/api/MemoryData.hpp>
#include <htgs/api/ITask.hpp>
#include <htgs/api/VoidData.hpp>
#include <tiffio.h>

namespace egt {

    template <class UserType>
    class TiffTileWriter : public htgs::ITask<htgs::MemoryData<fi::View<UserType>>, htgs::VoidData> {

    public:
        ///
        /// \param numThreads
        /// \param imageHeight
        /// \param imageWidth
        /// \param tileSize
        /// \param outputDepth The depth of the Output Image.
        /// \param outputPath
        TiffTileWriter(size_t numThreads,uint32_t imageHeight, uint32_t imageWidth, uint32_t tileSize, ImageDepth outputDepth, std::string outputPath) :
        _imageHeight(imageHeight), _imageWidth(imageWidth), _tileSize(tileSize), outputDepth(outputDepth), _outputPath(outputPath) {
            // Create the tiff file
            tif = TIFFOpen(outputPath.c_str(), "w");

            if (tif != nullptr) {
                TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, imageWidth);
                TIFFSetField(tif, TIFFTAG_IMAGELENGTH, imageHeight);
                TIFFSetField(tif, TIFFTAG_TILELENGTH, tileSize);
                TIFFSetField(tif, TIFFTAG_TILEWIDTH, tileSize);
                TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8 * calculateBitsPerSample(outputDepth) );
                TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
                TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
                TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
                TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
                TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
                TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
            }

        }

        void executeTask(std::shared_ptr<htgs::MemoryData<fi::View<UserType>>> data) override {

            fi::View<UserType> *view = data->get();

            auto *tile = new int8_t[_tileSize * _tileSize]();

            for( auto row = 0 ; row < _tileSize ; row++){
                std::copy_n(view->getPointerTile() + row * view->getViewWidth(), _tileSize, tile + row * _tileSize);
            }

            TIFFWriteTile(tif,
                          (tdata_t)tile,
                          view->getCol() * _tileSize,
                          view->getRow() * _tileSize,
                          0,
                          0);

            delete[] tile;
            data->releaseMemory();
        }

        /// \brief Close the tiff file
        void shutdown() override {
            TIFFClose(tif);
        }


        htgs::ITask <htgs::MemoryData<fi::View<UserType>>, htgs::VoidData> *copy() override {
            return new TiffTileWriter(this->getNumThreads(), _imageHeight, _imageWidth, _tileSize, outputDepth ,_outputPath);
        }

        TIFF *tif;
        uint32_t _imageHeight;
        uint32_t _imageWidth;
        uint32_t _tileSize;
        std::string _outputPath;
        ImageDepth outputDepth;

    };


}
#endif //NEWEGT_TILETIFFWRITER_H
