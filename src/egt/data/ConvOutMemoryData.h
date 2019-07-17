//
// Created by gerardin on 5/6/19.
//

#ifndef EGT_CONVOUTMEMORYDATA_H
#define EGT_CONVOUTMEMORYDATA_H

#include <htgs/api/IData.hpp>
#include <ostream>
#include "FastImage/api/View.h"
#include <htgs/api/MemoryData.hpp>
#include <htgs/types/Types.hpp>

/// \brief Output convolution data: output float tile
template <class V>
class ConvOutMemoryData : public htgs::IData {
private:

    htgs::m_data_t<V>_outputdata = nullptr; ///< Output data array

    uint32_t
            _globalRow,   ///< Global tile row
            _globalCol;   ///< Global tile column
    int32_t
            _tileWidth,
            _tileHeight;

public:


    ConvOutMemoryData(htgs::m_data_t<V> _outputdata, uint32_t _globalRow, uint32_t _globalCol, int32_t _tileWidth, int32_t _tileHeight)
            : _outputdata(_outputdata), _globalRow(_globalRow), _globalCol(_globalCol), _tileWidth(_tileWidth),
              _tileHeight(_tileHeight) {}


    const htgs::m_data_t<V> &getOutputdata() const {
        return _outputdata;
    }

    /// \brief Get global tile row index
    /// \return Global tile row index
    uint32_t getGlobalRow() const { return _globalRow; }

    /// \brief Get global tile column index
    /// \return Global tile column index
    uint32_t getGlobalCol() const { return _globalCol; }

    int32_t getTileWidth() const {
        return _tileWidth;
    }

    int32_t getTileHeight() const {
        return _tileHeight;
    }

    ~ConvOutMemoryData() {
        VLOG(5) << "ConvOutMemoryData destroyed : " << getGlobalRow() << "," << getGlobalCol();
    }

};

#endif //EGT_CONVOUTMEMORYDATA_H
