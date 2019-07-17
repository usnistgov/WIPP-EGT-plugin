//
// Created by Gerardin, Antoine D. (Assoc) on 2019-05-13.
//

#ifndef EGT_PYRAMIDTILEDTIFFLOADER_H
#define EGT_PYRAMIDTILEDTIFFLOADER_H

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

/// @file PyramidTiledTiffLoader.h
/// @author Alexandre Bardakoff - Timothy Blattner - Derek Juba
/// @date  2019-04-12
/// @brief Implements the color tiff tile loader class

////Handle type incompatibility between libtiff and openCV in MACOS
#ifdef __APPLE__
#define uint64 uint64_hack_
#define int64 int64_hack_
#include <tiffio.h>
#undef uint64
#undef int64
#else
#include <tiffio.h>
#include <glog/logging.h>

#endif

#include "FastImage/api/ATileLoader.h"
#include "FastImage/data/DataType.h"
#include "FastImage/object/FigCache.h"

namespace egt {
/// \namespace fi FastImage namespace

/**
   * @class PyramidTiledTiffLoader PyramidTiledTiffLoader.h
   *
   * @brief Tile loader specialized in grayscale tiff file. Use libtiff library.
   *
   * @details Tile Loader specialized in grayscale tiff images.
   * It opens the file, test if it is grayscale and tiled, and extract tiles from it.
   * Each tile' pixels are converted to the UserType format.
   * It implements the following functions from the ATileLoader:
   * @code
   *  std::string getName() override = 0;
   *  virtual ATileLoader *copyTileLoader() = 0;
   *  virtual uint32_t getImageHeight(uint32_t level = 0) const = 0;
   *  virtual uint32_t getImageWidth(uint32_t level = 0) const = 0;
   *  virtual uint32_t getTileWidth(uint32_t level = 0) const = 0;
   *  virtual uint32_t getTileHeight(uint32_t level = 0) const = 0;
   *  virtual short getBitsPerSample() const = 0;
   *  virtual uint32_t getNbPyramidLevels() const = 0;
   *  virtual double loadTileFromFile(UserType *tile, uint32_t indexRowGlobalTile, uint32_t indexColGlobalTile) = 0;
   * @endcode
   *
   * @tparam UserType Pixel Type asked by the end user
   */
    template<typename UserType>
class PyramidTiledTiffLoader : public fi::ATileLoader<UserType> {

    public:
        /// \brief TiffTileLoader constructor
        /// \details Specialized Tile loader constructor
        /// It opens, and extract metadata from the file thanks to the libtiff library.
        /// These metadata are used to check if the file is tiles and in grayscale.
        /// \param fileName File path
        /// \param numThreads Number of threads used by the tiff tile loader
        explicit PyramidTiledTiffLoader(const std::string &fileName,
                                     size_t numThreads = 1)
                : fi::ATileLoader<UserType>(fileName,
                                        numThreads) {

            // Open the file
            _tiff = TIFFOpen(fileName.c_str(), "r");

            if (_tiff != nullptr) {
                if (TIFFIsTiled(_tiff) == 0) {
                    std::stringstream message;
                    message << "Tile Loader ERROR: The image is not tiled.";
                    throw (fi::FastImageException(message.str()));
                }

                // Count pyramid levels
                do {
                    _numPyramidLevels++;
                }
                while (TIFFReadDirectory(_tiff));

                _imageWidths = new uint32_t[_numPyramidLevels];
                _imageHeights = new uint32_t[_numPyramidLevels];
                _tileWidths = new uint32_t[_numPyramidLevels];
                _tileHeights = new uint32_t[_numPyramidLevels];
                _numTilesWidths = new uint32_t[_numPyramidLevels];
                _numTilesHeights = new uint32_t[_numPyramidLevels];

                _samplesPerPixels = new uint16_t[_numPyramidLevels];
                _bitsPerSamples = new uint16_t[_numPyramidLevels];
                _sampleFormats = new uint16_t[_numPyramidLevels];

                // Load/parse header for each directory (pyramid level)
                for (uint32_t pyramidLevel = 0; pyramidLevel < _numPyramidLevels; pyramidLevel++) {
                    if (!TIFFSetDirectory(_tiff, pyramidLevel)) {
                        std::stringstream message;
                        message
                                << "Tile Loader ERROR: TIFFSetDirectory error in PyramidTiledTiffLoader with "
                                << "pyramidLevel = " << pyramidLevel;
                        throw (fi::FastImageException(message.str()));
                    }

                    if (!TIFFGetField(_tiff, TIFFTAG_IMAGEWIDTH, &(_imageWidths[pyramidLevel]))) {
                        throw (fi::FastImageException("Tile Loader ERROR: TIFFTAG_IMAGEWIDTH not defined"));
                    }

                    if (!TIFFGetField(_tiff, TIFFTAG_IMAGELENGTH, &(_imageHeights[pyramidLevel]))) {
                        throw (fi::FastImageException("Tile Loader ERROR: TIFFTAG_IMAGELENGTH not defined"));
                    }

                    if (!TIFFGetField(_tiff, TIFFTAG_TILEWIDTH, &(_tileWidths[pyramidLevel]))) {
                        throw (fi::FastImageException("Tile Loader ERROR: TIFFTAG_TILEWIDTH not defined"));
                    }

                    if (!TIFFGetField(_tiff, TIFFTAG_TILELENGTH, &(_tileHeights[pyramidLevel]))) {
                        throw (fi::FastImageException("Tile Loader ERROR: TIFFTAG_TILELENGTH not defined"));
                    }

                    if (!TIFFGetField(_tiff, TIFFTAG_SAMPLESPERPIXEL, &(_samplesPerPixels[pyramidLevel]))) {
                        throw (fi::FastImageException("Tile Loader ERROR: TIFFTAG_SAMPLESPERPIXEL not defined"));
                    }

                    if (!TIFFGetField(_tiff, TIFFTAG_BITSPERSAMPLE, &(_bitsPerSamples[pyramidLevel]))) {
                        throw (fi::FastImageException("Tile Loader ERROR: TIFFTAG_BITSPERSAMPLE not defined"));
                    }

                    if (!TIFFGetField(_tiff, TIFFTAG_SAMPLEFORMAT, &(_sampleFormats[pyramidLevel]))) {
                        // Interpret missing data format as unsigned integer data
                        _sampleFormats[pyramidLevel] = SAMPLEFORMAT_UINT;
                    }

                    // Integer division rounded up to compute height/width in tiles

                    _numTilesHeights[pyramidLevel] = _imageHeights[pyramidLevel] / _tileHeights[pyramidLevel];
                    if (_imageHeights[pyramidLevel] % _tileHeights[pyramidLevel] != 0) _numTilesHeights[pyramidLevel] += 1;

                    _numTilesWidths[pyramidLevel] = _imageWidths[pyramidLevel] / _tileWidths[pyramidLevel];
                    if (_imageWidths[pyramidLevel] % _tileWidths[pyramidLevel] != 0) _numTilesWidths[pyramidLevel] += 1;

                    // Interpret undefined data format as unsigned integer data
                    if (_sampleFormats[pyramidLevel] == SAMPLEFORMAT_VOID) {
                        _sampleFormats[pyramidLevel] = SAMPLEFORMAT_UINT;
                    }
                }
            }
            else {
                std::stringstream message;
                message << "Tile Loader ERROR: The image can not be opened.";
                throw (fi::FastImageException(message.str()));
            }
        }

        /// \brief TiffTileLoader default destructor
        /// \details Destroy a PyramidTiledTiffLoader and close the underlined file
        ~PyramidTiledTiffLoader() {
            if (_tiff != nullptr) {
                TIFFClose(_tiff);
            }

            delete[] _imageWidths;
            delete[] _imageHeights;
            delete[] _tileWidths;
            delete[] _tileHeights;
            delete[] _numTilesWidths;
            delete[] _numTilesHeights;

            delete[] _samplesPerPixels;
            delete[] _bitsPerSamples;
            delete[] _sampleFormats;
        }

        /// \brief Get Image height
        /// \return Image height in px
        uint32_t getImageHeight(uint32_t level = 0) const override {
            return _imageHeights[level];
        }

        /// \brief Get Image width
        /// \return Image width in px
        uint32_t getImageWidth(uint32_t level = 0) const override {
            return _imageWidths[level];
        }

        /// \brief Get tile width
        /// \return Tile width in px
        uint32_t getTileWidth(uint32_t level = 0) const override {
            return _tileWidths[level];
        }

        /// \brief Get tile height
        /// \return Tile height in px
        uint32_t getTileHeight(uint32_t level = 0) const override {
            return _tileHeights[level];
        }

        /// \brief Get image height in tiles
        /// \return Image height in tiles
        uint32_t getNumTilesHeight(uint32 level = 0) const {
            return _numTilesHeights[level];
        }

        /// \brief Get image width in tiles
        /// \return Image width in tiles
        uint32_t getNumTilesWidth(uint32 level = 0) const {
            return _numTilesWidths[level];
        }

        /// \brief Get the bits per sample from the file
        /// \details Return the number of bits per sample as defined in the libtiff library:
        /// https://www.awaresystems.be/imaging/tiff/tifftags/bitspersample.html
        /// \return the number of bits per sample
        short getBitsPerSample() const override {
            return _bitsPerSamples[0];
        }

        short getSamplesPerPixel(uint32 level = 0) const {
            return _samplesPerPixels[level];
        }

        /// \brief Getter to the number of pyramids levels
        /// \details Get the number of pyramids levels, in this case 1, because the
        /// assumption is the image is planar
        /// \return
        uint32_t getNbPyramidLevels() const override {
            return _numPyramidLevels;
        }

        /// \brief Loader to the tile with a specific cast
        /// \brief Load a specific tile from the file into a tile buffer,
        /// casting each pixels from  FileType to UserType
        /// \tparam FileType Type deduced from the file
        /// \param src Source data buffer
        /// \param dest Tile buffer
        template<typename FileType>
        void loadTile(tdata_t src, UserType *dest, uint32_t numSamples) {
            for (uint32_t sampleNum = 0; sampleNum < numSamples; ++sampleNum) {
                dest[sampleNum] =
                        (UserType) ((FileType *) (src))[sampleNum];
            }
        }

        /// \brief Get down scale Factor for pyramid images. The tiff image for this
        /// loader are not pyramids, so return 1
        /// \param level level to ask--> Not used
        /// \return 1
        float getDownScaleFactor(uint32_t level = 0) override {
            return 2;
        }

        /// \brief Load a tile from the disk
        /// \details Allocate a buffer, load a tile from the file into the buffer,
        /// cast each pixel to the right format into parameter tile, and close
        /// the buffer.
        /// \param tile Pointer to a tile already allocated to fill
        /// \param indexRowGlobalTile Row index tile asked
        /// \param indexColGlobalTile Column Index tile asked
        /// \return Duration in mS to load a tile from the disk, use for statistics
        /// purpose
        double loadTileFromFile(UserType *tile,
                                uint32_t indexRowGlobalTile,
                                uint32_t indexColGlobalTile) override {

            uint32_t pyramidLevel = (uint32_t)this->getPipelineId();

            return loadTileFromFile(tile,
                                    indexRowGlobalTile,
                                    indexColGlobalTile,
                                    pyramidLevel);
        }

        double loadTileFromFile(UserType *tile,
                                uint32_t indexRowGlobalTile,
                                uint32_t indexColGlobalTile,
                                uint32_t pyramidLevel) {

            int setDirectoryStatus = TIFFSetDirectory(_tiff, pyramidLevel);

            if (setDirectoryStatus != 1) {
                std::stringstream message;
                message
                        << "Tile Loader ERROR: TIFFSetDirectory error in loadTileFromFile with "
                        << "pyramidLevel = " << pyramidLevel;
                throw (fi::FastImageException(message.str()));
            }

            tdata_t tiffTile = _TIFFmalloc(TIFFTileSize(_tiff));

            auto begin = std::chrono::high_resolution_clock::now();

            tsize_t numBytesRead = TIFFReadTile(_tiff,
                                                tiffTile,
                                                indexColGlobalTile * _tileWidths[pyramidLevel],
                                                indexRowGlobalTile * _tileHeights[pyramidLevel],
                                                0,
                                                0);

            if (numBytesRead == -1) {
                std::stringstream message;
                message
                        << "Tile Loader ERROR: TIFFReadTile error in loadTileFromFile with "
                        << "row = " << indexRowGlobalTile
                        << ", col = " << indexColGlobalTile
                        << ", level = " << pyramidLevel;
                throw (fi::FastImageException(message.str()));
            }

            auto end = std::chrono::high_resolution_clock::now();

            double diskDuration = (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(
                    end - begin).count());

            uint32_t numTilePixels = _tileWidths[pyramidLevel] * _tileHeights[pyramidLevel];
            uint32_t numTileSamples = numTilePixels * _samplesPerPixels[pyramidLevel];

            switch (_sampleFormats[pyramidLevel]) {
                case SAMPLEFORMAT_UINT:
                    switch (_bitsPerSamples[pyramidLevel]) {
                        case 8:loadTile<uint8_t>(tiffTile, tile, numTileSamples);
                            break;
                        case 16:loadTile<uint16_t>(tiffTile, tile, numTileSamples);
                            break;
                        case 32:loadTile<uint32_t>(tiffTile, tile, numTileSamples);
                            break;
                        case 64:loadTile<uint64_t>(tiffTile, tile, numTileSamples);
                            break;
                        default:
                            std::stringstream message;
                            message
                                    << "Tile Loader ERROR: The data format is not supported for "
                                       "unsigned integer, number bits per pixel = "
                                    << _bitsPerSamples[pyramidLevel];
                            throw (fi::FastImageException(message.str()));
                    }
                    break;
                case SAMPLEFORMAT_INT:
                    switch (_bitsPerSamples[pyramidLevel]) {
                        case 8:loadTile<int8_t>(tiffTile, tile, numTileSamples);
                            break;
                        case 16:loadTile<int16_t>(tiffTile, tile, numTileSamples);
                            break;
                        case 32:loadTile<int32_t>(tiffTile, tile, numTileSamples);
                            break;
                        case 64:loadTile<int64_t>(tiffTile, tile, numTileSamples);
                            break;
                        default:
                            std::stringstream message;
                            message
                                    << "Tile Loader ERROR: The data format is not supported for "
                                       "signed integer, number bits per pixel = "
                                    << _bitsPerSamples[pyramidLevel];
                            throw (fi::FastImageException(message.str()));
                    }
                    break;
                case SAMPLEFORMAT_IEEEFP:
                    switch (_bitsPerSamples[pyramidLevel]) {
                        case 8:loadTile<float>(tiffTile, tile, numTileSamples);
                            break;
                        case 16:loadTile<float>(tiffTile, tile, numTileSamples);
                            break;
                        case 32:loadTile<float>(tiffTile, tile, numTileSamples);
                            break;
                        case 64:loadTile<double>(tiffTile, tile, numTileSamples);
                            break;
                        default:
                            std::stringstream message;
                            message
                                    << "Tile Loader ERROR: The data format is not supported for "
                                       "float, number bits per pixel = " << _bitsPerSamples[pyramidLevel];
                            throw (fi::FastImageException(message.str()));
                    }
                    break;
                default:
                    std::stringstream message;
                    message
                            << "Tile Loader ERROR: The data format is not supported, sample "
                               "format = " << _sampleFormats[pyramidLevel];
                    throw (fi::FastImageException(message.str()));
            }

            _TIFFfree(tiffTile);
            return diskDuration;
        }

        void loadTiffTilesIntoRegion(UserType *region,
                                     uint32_t minPixelRow,
                                     uint32_t minPixelCol,
                                     uint32_t regionHeight,
                                     uint32_t regionWidth,
                                     uint32_t pyramidLevel) {

            // Protect against uint32_t underflow
            if (regionHeight == 0 || regionWidth == 0) {
                return;
            }

            uint32_t maxPixelRow = minPixelRow + regionHeight - 1;
            uint32_t maxPixelCol = minPixelCol + regionWidth - 1;

            uint32_t tileHeight = _tileHeights[pyramidLevel];
            uint32_t tileWidth = _tileWidths[pyramidLevel];

            uint32_t minTileRow = minPixelRow / tileHeight;
            uint32_t minTileCol = minPixelCol / tileWidth;

            uint32_t maxTileRow = maxPixelRow / tileHeight;
            uint32_t maxTileCol = maxPixelCol / tileWidth;

            uint16_t samplesPerPixel = _samplesPerPixels[pyramidLevel];

            UserType * tiffTile = new UserType[tileWidth * tileHeight * samplesPerPixel];

            // Loop over all tiles which intersect the region
            for (uint32_t tileRow = minTileRow; tileRow <= maxTileRow; ++tileRow) {
                for (uint32_t tileCol = minTileCol; tileCol <= maxTileCol; ++tileCol) {

                    loadTileFromFile(tiffTile, tileRow, tileCol, pyramidLevel);

                    uint32_t minPixelRowInTile{};
                    uint32_t minPixelColInTile{};

                    uint32_t maxPixelRowInTile{};
                    uint32_t maxPixelColInTile{};

                    uint32_t minPixelRowInRegion{};
                    uint32_t minPixelColInRegion{};

                    //uint32_t maxPixelRowInRegion{};
                    //uint32_t maxPixelColInRegion{};

                    // Top row of region passes through current tile
                    if (minPixelRow >= tileRow * tileHeight) {
                        minPixelRowInTile = minPixelRow - (tileRow * tileHeight);
                        minPixelRowInRegion = 0;
                    }
                        // Top row of region is outside current tile
                    else {
                        minPixelRowInTile = 0;
                        minPixelRowInRegion = (tileRow * tileHeight) - minPixelRow;
                    }

                    // Left column of region passes through current tile
                    if (minPixelCol >= tileCol * tileWidth) {
                        minPixelColInTile = minPixelCol - (tileCol * tileWidth);
                        minPixelColInRegion = 0;
                    }
                        // Left column of region is outside current tile
                    else {
                        minPixelColInTile = 0;
                        minPixelColInRegion = (tileCol * tileWidth) - minPixelCol;
                    }

                    // Bottom row of region passes through current tile
                    if (maxPixelRow < (tileRow + 1) * tileHeight) {
                        maxPixelRowInTile = maxPixelRow - (tileRow * tileHeight);
                        //maxPixelRowInRegion = regionHeight - 1;
                    }
                        // Bottom row of region is outside current tile
                    else {
                        maxPixelRowInTile = tileHeight - 1;
                        //maxPixelRowInRegion = ((tileRow + 1) * tileHeight) - minPixelRow - 1;
                    }

                    // Right column of region passes through current tile
                    if (maxPixelCol < (tileCol + 1) * tileWidth) {
                        maxPixelColInTile = maxPixelCol - (tileCol * tileWidth);
                        //maxPixelColInRegion = regionWidth - 1;
                    }
                        // Right column of region is outside current tile
                    else {
                        maxPixelColInTile = tileWidth - 1;
                        //maxPixelColInRegion = ((tileCol + 1) * tileWidth) - minPixelCol - 1;
                    }

                    // Copy pixels from current tile to region
                    for (uint32_t pixelRowInTile = minPixelRowInTile, pixelRowInRegion = minPixelRowInRegion;
                         pixelRowInTile <= maxPixelRowInTile;
                         ++pixelRowInTile, ++pixelRowInRegion) {

                        for (uint32_t pixelColInTile = minPixelColInTile, pixelColInRegion = minPixelColInRegion;
                             pixelColInTile <= maxPixelColInTile;
                             ++pixelColInTile, ++pixelColInRegion) {

                            for (uint16_t sampleNum = 0;
                                 sampleNum < samplesPerPixel;
                                 ++sampleNum) {

                                uint32_t tilePixelIndex = pixelRowInTile * tileWidth + pixelColInTile;
                                uint32_t regionPixelIndex = pixelRowInRegion * regionWidth + pixelColInRegion;

                                uint32_t tileSampleIndex = tilePixelIndex * samplesPerPixel + sampleNum;
                                uint32_t regionSampleIndex = regionPixelIndex * samplesPerPixel + sampleNum;

                                region[regionSampleIndex] = tiffTile[tileSampleIndex];
                            }
                        }
                    }
                }
            }

            delete[] tiffTile;
        }

        /// \brief Copy function used by HTGS to use multiple Tile Loader
        /// \return  A new ATileLoader copied
        fi::ATileLoader<UserType> *copyTileLoader() override {
            return new PyramidTiledTiffLoader<UserType>(this->getNumThreads(),
                                                     this->getFilePath(),
                                                     *this);
        }

        /// \brief Get the name of the tile loader
        /// \return Name of the tile loader
        std::string getName() override {
            return "TIFF Tile Loader";
        }

    private:
        /// \brief TiffTileLoader constructor used by the copy operator
        /// \param numThreads Number of thread used by the tiff tile loader
        /// \param filePath File path
        /// \param from Origin TiffTileLoader
        PyramidTiledTiffLoader(size_t numThreads,
                            const std::string &filePath,
                            const PyramidTiledTiffLoader &from)
                : fi::ATileLoader<UserType>(filePath, numThreads) {
            this->_tiff = TIFFOpen(filePath.c_str(), "r");

            this->_numPyramidLevels = from._numPyramidLevels;

            _imageWidths = new uint32_t[_numPyramidLevels];
            _imageHeights = new uint32_t[_numPyramidLevels];
            _tileWidths = new uint32_t[_numPyramidLevels];
            _tileHeights = new uint32_t[_numPyramidLevels];
            _numTilesWidths = new uint32_t[_numPyramidLevels];
            _numTilesHeights = new uint32_t[_numPyramidLevels];

            _samplesPerPixels = new uint16_t[_numPyramidLevels];
            _bitsPerSamples = new uint16_t[_numPyramidLevels];
            _sampleFormats = new uint16_t[_numPyramidLevels];

            for (uint32_t level = 0; level < this->_numPyramidLevels; level++) {
                this->_imageWidths[level] = from._imageWidths[level];
                this->_imageHeights[level] = from._imageHeights[level];
                this->_tileWidths[level] = from._tileWidths[level];
                this->_tileHeights[level] = from._tileHeights[level];
                this->_numTilesWidths[level] = from._numTilesWidths[level];
                this->_numTilesHeights[level] = from._numTilesHeights[level];
                this->_sampleFormats[level] = from._sampleFormats[level];
                this->_bitsPerSamples[level] = from._bitsPerSamples[level];
                this->_samplesPerPixels[level] = from._samplesPerPixels[level];
            }
        }

        TIFF * _tiff = nullptr;             ///< Tiff file pointer

        uint32_t * _imageHeights = nullptr;           ///< Image height in pixel
        uint32_t * _imageWidths = nullptr;            ///< Image width in pixel
        uint32_t * _tileHeights = nullptr;            ///< Tile height
        uint32_t * _tileWidths = nullptr;             ///< Tile width
        uint32_t * _numTilesHeights = nullptr;        ///< Image height in tiles
        uint32_t * _numTilesWidths = nullptr;         ///< Image width in tiles
        uint32_t _numPyramidLevels = 0;      ///< Number of pyramid levels

        uint16_t * _sampleFormats = nullptr;          ///< Sample format as defined by libtiff
        uint16_t * _bitsPerSamples = nullptr;         ///< Bit Per Sample as defined by libtiff
        uint16_t * _samplesPerPixels = nullptr;       ///< Samples Per Pixel as defined by libtiff

    };
}


#endif //EGT_PYRAMIDTILEDTIFFLOADER_H
