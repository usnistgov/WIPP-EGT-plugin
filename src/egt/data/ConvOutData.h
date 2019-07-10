//
// Created by gerardin on 5/6/19.
//

#ifndef EGT_CONVOUTDATA_H
#define EGT_CONVOUTDATA_H

#include <htgs/api/IData.hpp>
#include <ostream>
#include "FastImage/api/View.h"

/// \brief Output convolution data: output float tile
template <class V>
class ConvOutData : public htgs::IData {
private:

    V *
            _outputdata;  ///< Output data array

    uint32_t
            _globalRow,   ///< Global tile row
            _globalCol,   ///< Global tile column
            _tileWidth,
            _tileHeight;

public:
    /// \brief Convolution output data constructor
    /// \param outputdata Output data
    /// \param globalRow Global row index
    /// \param globalCol Global column index
    ConvOutData(V *outputdata, uint32_t globalRow, uint32_t globalCol) : _outputdata(outputdata),
                                                                             _globalRow(globalRow),
                                                                             _globalCol(globalCol) {}


    ConvOutData(V *_outputdata, uint32_t _globalRow, uint32_t _globalCol, uint32_t _tileWidth, uint32_t _tileHeight)
            : _outputdata(_outputdata), _globalRow(_globalRow), _globalCol(_globalCol), _tileWidth(_tileWidth),
              _tileHeight(_tileHeight) {}

    /// \brief Get output data array
    /// \return Output data array
    V *getOutputdata() const { return _outputdata; }

    /// \brief Get global tile row index
    /// \return Global tile row index
    uint32_t getGlobalRow() const { return _globalRow; }

    /// \brief Get global tile column index
    /// \return Global tile column index
    uint32_t getGlobalCol() const { return _globalCol; }

    uint32_t getTileWidth() const {
        return _tileWidth;
    }

    uint32_t getTileHeight() const {
        return _tileHeight;
    }

};

#endif //EGT_CONVOUTDATA_H
