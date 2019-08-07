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

/// @file Blob.h
/// @author Alexandre Bardakoff - Timothy Blattner
/// @date  4/6/18
/// @brief Blob object representing a part of a feature


#ifndef EGT_FEATURECOLLECTION_BLOB_H
#define EGT_FEATURECOLLECTION_BLOB_H

#include <cstdint>
#include <utility>
#include <set>
#include <map>
#include <mutex>
#include <list>
#include <iostream>
#include <FastImage/api/FastImage.h>
#include <unordered_set>
#include "Feature.h"

/// \namespace fc FeatureCollection namespace
namespace egt {

/// \brief Coordinate structure as a pair of int32_t for <row, col>
using Coordinate = std::pair<int32_t, int32_t>;


/**
  * @class Blob Blob.h <FastImage/FeatureCollection/Data/Blob.h>
  *
  * @brief Blob representing a part of a feature
  *
  * @details It is composed by a bounding box from the point (rowMin, rowMax) to
  * (rowMAx, ColMax).The bounding box delimit a sparse matrix representing the
  * pixels part of this blob. A blob has a specific id, called tag.
  **/
class Blob {
 public:
  /// \brief Blob construction and initialisation
  Blob(uint32_t row, uint32_t col)
      : _parent(this),
        _rank(0),
        _rowMin(std::numeric_limits<int32_t>::max()),
        _rowMax(0),
        _colMin(std::numeric_limits<int32_t>::max()),
        _colMax(0),
        _startRow(row),
        _startCol(col) {
    static uint32_t
        currentTag = 0;
    std::mutex
        m;
    std::lock_guard<std::mutex>
        lock(m);

    _tag = currentTag++;
    _count = 0;
  }

    virtual ~Blob() {
      if(_feature != nullptr) {
          delete[] _feature->getBitMask();
          delete _feature;
      }
    }

    /// \brief Get Blob tag
  /// \return Blob tag
  uint32_t getTag() const { return _tag; }

    bool isToMerge() const {
        return _toMerge;
    }

    void setToMerge(bool toMerge) {
        _toMerge = toMerge;
    }


    uint32_t getStartRow() const {
        return _startRow;
    }

    uint32_t getStartCol() const {
        return _startCol;
    }


    /// \brief Get sparse matrix structure
  /// \return Sparse matrix structure
  std::unordered_map<int32_t, std::unordered_set<int32_t>>
  &getRowCols() {
    return _rowCols;
  }
  /// \brief Get minimum bounding box row
  /// \return Minimum bounding box row
  int32_t getRowMin() const { return _rowMin; }

  /// \brief Get maximum bounding box row
  /// \return Maximum bounding box row
  int32_t getRowMax() const { return _rowMax; }

  /// \brief Get minimum bounding box col
  /// \return Minimum bounding box col
  int32_t getColMin() const { return _colMin; }

  /// \brief Get maximum bounding box col
  /// \return Maximum bounding box col
  int32_t getColMax() const { return _colMax; }

  /// \brief Get number of pixels in a blob
  /// \return Number of pixels in a blob
  uint64_t getCount() const { return _count; }

  /// \brief Get blob parent, used by Union find
  /// \return Blob  parent
  Blob *getParent() const { return _parent; }

  /// \brief Get blob rank, used by Union find
  /// \return Blob rank
  uint32_t getRank() const { return _rank; }

  const Feature *getFeature() const { return _feature; }

  /// \brief Test if a pixel is in a blob
  /// \param row Row asked
  /// \param col Col asked
  /// \return True is the pixel(row, col) is in the blob, else False
  bool isPixelinFeature(int32_t row, int32_t col) {
    if (isPixelInBoundingBox(row, col)) {
        if(_feature != nullptr) {
            return _feature->isImagePixelInBitMask(row, col);
        }
        return (_rowCols.count(row) != 0) && (_rowCols[row].count(col) != 0);
    }
    return false;
  }

  bool isPixelInBoundingBox(int32_t row, int32_t col){
      return (row >= _rowMin && row < _rowMax && col >= _colMin && col < _colMax);
  }


  uint64_t getBoundingBoxArea() {
      return (_colMax - _colMin) * (_rowMax - _rowMin);
  }

  /// \brief Minimum bounding box row setter
  /// \param rowMin Minimum bounding box row
  void setRowMin(int32_t rowMin) { _rowMin = rowMin; }

  /// \brief Maximum bounding box row setter
  /// \param rowMax Maximum bounding box row
  void setRowMax(int32_t rowMax) { _rowMax = rowMax; }

  /// \brief Minimum bounding box col setter
  /// \param colMin Minimum bounding box col
  void setColMin(int32_t colMin) { _colMin = colMin; }

  /// \brief Maximum bounding box col setter
  /// \param colMax Maximum bounding box col
  void setColMax(int32_t colMax) { _colMax = colMax; }

  /// \brief Count setter
  /// \param count Pixel Count
  void setCount(uint64_t count) { _count = count; }

  /// \brief Blob parent setter Used by Union find algorithm
  /// \param parent Blob parent
  void setParent(Blob *parent) { _parent = parent; }

  /// \brief Blob rank setter Used by Union find algorithm
  /// \param rank Blob rank
  void setRank(uint32_t rank) { _rank = rank; }

  /// \brief Add a pixel to the blob and update blob metadata
  /// \param row Pixel's row
  /// \param col Pixel's col
  void addPixel(int32_t row, int32_t col) {
    if (row < _rowMin)
      _rowMin = row;
    if (col < _colMin)
      _colMin = col;
    if (row >= _rowMax)
      _rowMax = row + 1;
    if (col >= _colMax)
      _colMax = col + 1;
    _count++;
    addRowCol(row, col);

  }

  /// \brief Merge 2 blobs, and delete the unused one
  /// \param blob Blob to merge with the current
  /// \return The blob merged
  Blob *mergeAndDelete(Blob *blob) {
    Blob
        *toDelete = nullptr,
        *destination = nullptr;

    if (this->getCount() >= blob->getCount()) {
      destination = this;
      toDelete = blob;
    } else {
      destination = blob;
      toDelete = this;
    }

    // Set blob metadata
    destination->setRowMin(
        std::min(destination->getRowMin(), toDelete->getRowMin()));
    destination->setColMin(
        std::min(destination->getColMin(), toDelete->getColMin()));
    destination->setRowMax(
        std::max(destination->getRowMax(), toDelete->getRowMax()));
    destination->setColMax(
        std::max(destination->getColMax(), toDelete->getColMax()));

    // Merge sparse matrix
    for (auto rowCol : toDelete->getRowCols()) {
      for (auto col : rowCol.second) {
        destination->addRowCol(rowCol.first, col);
      }
    }


    // update pixel count
    destination->setCount(toDelete->getCount() + destination->getCount());

    // Delete unused Blob
    delete (toDelete);
    // Return merged blob
    return destination;
  }

  /// transform the sparse matrix representation from addrows to feature to reduce memory footprint
  void compactBlobDataIntoFeature() {

    uint32_t
            idFeature = 0,
            rowMin = (uint32_t) this->getRowMin(),
            colMin = (uint32_t)this->getColMin(),
            rowMax = (uint32_t)this->getRowMax(),
            colMax = (uint32_t)this->getColMax(),
            ulRowL = 0,
            ulColL = 0,
            wordPosition = 0,
            bitPositionInDecimal = 0,
            absolutePosition = 0;

    //TODO change coords to uint32_t?
    BoundingBox boundingBox(
            rowMin,
            colMin,
            rowMax,
            colMax);

    auto bitMask = new uint32_t[(uint32_t) ceil((boundingBox.getHeight() * boundingBox.getWidth()) / 32.)]();

    // For every pixel in the bit mask
    for (auto row = (uint32_t) rowMin; row < rowMax; ++row) {
      for (auto col = (uint32_t) colMin; col < colMax; ++col) {
        // Test if the pixel is in the current feature (using global coordinates)
        if (this->isPixelinFeature(row, col)) {
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
          auto bitPositionInBinary = ((uint32_t) 1 << (bitPositionInDecimal - (uint32_t) 1));
          //adding the bitPosition to the word
          bitMask[wordPosition] = bitMask[wordPosition] | bitPositionInBinary;
        }
      }
    }

    _feature = new Feature(this->getTag(), boundingBox, bitMask);
    _rowCols.clear();
  }

  void addToBitMask(uint32_t* bitMask, BoundingBox &bb) {

    uint32_t
      rowMin = (uint32_t) this->getRowMin(),
      colMin = (uint32_t)this->getColMin(),
      rowMax = (uint32_t)this->getRowMax(),
      colMax = (uint32_t)this->getColMax(),
      ulRowL = 0,
      ulColL = 0,
      wordPosition = 0,
      bitPositionInDecimal = 0,
      absolutePosition = 0;

    // For every pixel in the bit mask
    for (auto row = (uint32_t) rowMin; row < rowMax; ++row) {
      for (auto col = (uint32_t) colMin; col < colMax; ++col) {
        // Test if the pixel is in the current feature (using global coordinates)
        if (this->getFeature()->isImagePixelInBitMask(row, col)) {
          // Add it to the bit mask
          ulRowL = row - bb.getUpperLeftRow(); //convert to local coordinates
          ulColL = col - bb.getUpperLeftCol();
          absolutePosition = ulRowL * bb.getWidth() + ulColL; //to 1D array coordinates
          //optimization : right-shifting binary representation by 5 is equivalent to dividing by 32
          wordPosition = absolutePosition >> (uint32_t) 5;
          //left-shifting back previous result gives the 1D array coordinates of the word beginning
          auto beginningOfWord = ((int32_t) (wordPosition << (uint32_t) 5));
          //subtracting original value gives the remainder of the division by 32.
          auto remainder = ((int32_t) absolutePosition - beginningOfWord);
          //at which position in a binary representation the bit needs to be set?
          bitPositionInDecimal = (uint32_t) abs(32 - remainder);
          //create a 32bit word with this bit set to 1.
          auto bitPositionInBinary = ((uint32_t) 1 << (bitPositionInDecimal - (uint32_t) 1));
          //adding the bitPosition to the word
          bitMask[wordPosition] = bitMask[wordPosition] | bitPositionInBinary;
        }
      }
    }
  }

  //TODO remove, we will only  create Feature directly not modifying blobs
  void setFeature(Feature *f){
    this->_feature = f;
  }

  /// \brief Print blob state
  /// \param os Output stream
  /// \param blob Blob to print
  /// \return Output stream with the blob information
  friend std::ostream &operator<<(std::ostream &os, const Blob &blob) {
    os << "Blob #" << blob._tag << std::endl;
    os << "    BB: ["
       << blob._rowMin << ", " << blob._colMin << "] -> ["
       << blob._rowMax << ", " << blob._colMax << "]" << std::endl;
    os << "    # Pixels: " << blob._count << std::endl;
    return os;
  }

 private:
  /// \brief Add a pixel to the sparse matrix
  /// \param row Pixel row
  /// \param col Pixel col
  void addRowCol(int32_t row, int32_t col) { _rowCols[row].insert(col); }

  Blob *
      _parent = nullptr;  ///< Blob parent, used by the Union find algorithm

  uint32_t
      _rank = 0,          ///< Blob rank, used by the Union find algorithm
      _tag = 0;           ///< Tag, only used to print/debug purpose

  int32_t
      _rowMin{},          ///< Minimum bounding box row (in the global coordinates of the image)
      _rowMax{},          ///< Maximum bounding box row (in the global coordinates of the image)
      _colMin{},          ///< Minimum bounding box col (in the global coordinates of the image)
      _colMax{};          ///< Maximum bounding box col (in the global coordinates of the image)

  uint64_t
      _count = 0;           ///< Number of pixel to fastened the blob merge

  std::unordered_map<int32_t, std::unordered_set<int32_t>>
      _rowCols
      {};         ///< Sparse matrix of unique coordinates composing the blob

  bool _toMerge = false;

  uint32_t _startRow = 0;
  uint32_t _startCol = 0;

  Feature *_feature = nullptr;
};

}

#endif //EGT_FEATURECOLLECTION_BLOB_H
