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

/// @file BlobMerger.h
/// @author Alexandre Bardakoff - Timothy Blattner
/// @date  4/6/17
/// @brief Blob merger task


#ifndef FEATURECOLLECTION_BLOBMERGER_H
#define FEATURECOLLECTION_BLOBMERGER_H

#include <tiffio.h>
#include <htgs/api/ITask.hpp>
#include <cstdint>
#include <utility>
#include <FastImage/FeatureCollection/tools/UnionFind.h>
#include <egt/FeatureCollection/Data/ListBlobs.h>
#include <egt/utils/FeatureExtraction.h>

namespace egt {
/// \namespace egt EGT namespace

/**
  * @class BlobMerger BlobMerger.h <FastImage/FeatureCollection/Tasks/BlobMerger.h>
  *
  * @brief Merges multiple egt::ViewAnalyse to build the Feature Collection.
  *
  *  Merge the different analyse with a disjoint-set data structure to represent
  *  each blobs and merge them easily:
  *  https://en.wikipedia.org/wiki/Disjoint-set_data_structure

  **/
    template<class T>
    class BlobMerger : public htgs::ITask<ViewAnalyse, ListBlobs> {
    public:
        /// \brief BlobMerger constructor
        /// \param imageHeight Image Height
        /// \param imageWidth ImageWidth
        /// \param nbTiles Number of tiles in the image5
        BlobMerger(uint32_t imageHeight, uint32_t imageWidth, uint32_t nbTiles,  EGTOptions *options, SegmentationOptions* segmentationOptions)
                : ITask(1), imageHeight(imageHeight), imageWidth(imageWidth), _nbTiles(nbTiles), options(options), segmentationOptions(segmentationOptions) {
            _blobs = new ListBlobs();
            _holes = new ListBlobs();
        }

        //TODO _blobs is returned as result, _holes is not => make _blobs shared_ptr and _holes a unique_ptr
        //and remove destructor
        virtual ~BlobMerger() {
            delete _holes;
        }

        /// \brief Get the view analyse, merge them into one analyse and merge all
        /// blob to build the feature collection
        /// \param data View analyse
        void executeTask(std::shared_ptr<ViewAnalyse> data) override {

            // for each new tile, collect blobs
            for (auto blobListCoordToMerge : data->getToMerge()) {
                this->_toMerge[blobListCoordToMerge.first]
                        .merge(blobListCoordToMerge.second);
            }
            auto viewBlob = data->getBlobs();
            this->_blobs->_blobs.merge(viewBlob);

            //collect holes
            for (auto blobListCoordToMerge : data->getHolesToMerge()) {
                this->_holesToMerge[blobListCoordToMerge.first]
                        .merge(blobListCoordToMerge.second);
            }
            auto holes = data->getHoles();
            this->_holes->_blobs.merge(holes);

            _count++;

            //merge the blobs when all tiles have been received
            if (_count == _nbTiles) {

                auto startMerge = std::chrono::high_resolution_clock::now();

                VLOG(1) << "merging " << this->_holes->_blobs.size() << " holes...";
                VLOG(1) << "merging " << this->_blobs->_blobs.size() << " blobs...";

                _count = 0;
                //flag set to true because we should remove holes not linked to anything because we have already removed background info
                merge(_holesToMerge,_holes, true);
                filterHoles();
                merge(_toMerge,_blobs, false);
                filterObjects();


                VLOG(1) << "after last merge, we have : " << _blobs->_blobs.size() << " blobs left";

                auto endMerge = std::chrono::high_resolution_clock::now();
                VLOG(1) << "    Merge blobs: "
                        << std::chrono::duration_cast<std::chrono::milliseconds>(endMerge - startMerge).count()
                        << " mS";

                this->addResult(_blobs);
            }
        }

        /// \brief Get the name of the task
        /// \return Task name
        std::string getName() override { return "Blob Merge"; }

        /// \brief Task copy, should be a singleton, to send bask itself
        /// \return itself
        BlobMerger *copy() override { return this; }

    private:
        /// \brief Retrieve a blob from a coordinate, nullptr is to blob is
        /// corresponding
        /// \param row Row
        /// \param col Column
        Blob *getBlobFromCoord(ListBlobs *blobs, const int32_t &row, const int32_t &col) const {
            // Iterate over the blobs
            for (auto blob : blobs->_blobs) {
                // Test if the pixel is in the blob
                if (blob->isPixelinFeature(row,col)) {
                    return blob;
                }
            }
            return nullptr;
        }

        /**
         * Filter holes : Small holes are filled. Bigger holes are considered background.
         */
        void filterHoles() {
            uint32_t nbHolesTooSmall = 0;
            auto originalNbOfHoles = _holes->_blobs.size();

            auto meanIntensities = new std::unordered_map<Blob*, T >();
            computeMeanIntensity<T>(_holes->_blobs, options, meanIntensities);


            auto i = _holes->_blobs.begin();
            while (i != _holes->_blobs.end()) {
                auto blob = (*i);
                //transform small holes into blobs
                if(
                    (blob->getCount() < segmentationOptions->MIN_HOLE_SIZE || blob->getCount() > segmentationOptions->MAX_HOLE_SIZE)
//                    && (meanIntensities->at(blob) > 200 && meanIntensities->at(blob) < 600)
                  ) {
                        this->_blobs->_blobs.push_back(blob);
                        auto row = blob->getStartRow();
                        auto col = blob->getStartCol();

                        //we need to merge this hole into its containing blob
                        //looking around its start point, we should find a valid blob to merge with
                        std::list<Coordinate> coords{};
                        if(row - 1 >= 0 && !blob->isPixelinFeature(row -1, col)) {
                            coords.emplace_back(row - 1, col);
                        }
                        if(row < imageHeight && !blob->isPixelinFeature(row + 1, col)) {
                            coords.emplace_back(row + 1, col);
                        }
                        if(col - 1 >= 0 && !blob->isPixelinFeature(row, col - 1)) {
                            coords.emplace_back(row, col - 1);
                        }
                        if(col + 1 < imageWidth && !blob->isPixelinFeature(row, col + 1)) {
                            coords.emplace_back(row, col + 1);
                        }
                        assert(!coords.empty()); //otherwise we won't be able to merge with another blob!
                        this->_toMerge[blob].merge(coords);
                        nbHolesTooSmall++;
                        i = _holes->_blobs.erase(i);
                        VLOG(4) << "Transform hole at (" << row << "," << col << ") into object blob.";
                }
                //turn holes into background
                else {
                    delete blob;
                    i = _holes->_blobs.erase(i);
                }
            }

            VLOG(3) << "original number of holes : " << originalNbOfHoles;
            VLOG(3) << "nb of holes filled : " << nbHolesTooSmall;

        }

        /**
         * Filter objects : Remove small objects.
         */
        void filterObjects() {

            uint32_t nbBlobsTooSmall = 0;

            auto originalNbOfBlobs = _blobs->_blobs.size();

            auto i = _blobs->_blobs.begin();
            while (i != _blobs->_blobs.end()) {
                //We removed objects that are still too small after the merge occured.
                if((*i)->getCount() < segmentationOptions->MIN_OBJECT_SIZE) {
                    nbBlobsTooSmall++;
                    delete (*i);
                    i = _blobs->_blobs.erase(i);
                }
                else {
                    i++;
                }
            }

            auto nbBlobs = _blobs->_blobs.size();
            VLOG(3) << "original nb of objects : " << originalNbOfBlobs;
            VLOG(3) << "nb of small objects that have been removed : " << nbBlobsTooSmall;
            VLOG(3) << "total nb of objects after filtering: " <<nbBlobs;
        }

        /// \brief Merge all the blobs from all the view analyser
        void merge(std::map<Blob *, std::list<Coordinate >> &toMerge, ListBlobs *blobs, bool deleteLonelyBlobs) {
            fc::UnionFind<Blob>
                    uf{};

            std::map<Blob *, std::set<Blob *>>
                    parentSons{};

            // Apply the UF algorithm to every linked blob
            for (const auto &blobCoords : toMerge) {
                for (auto coord : blobCoords.second) {
                    if (auto other = getBlobFromCoord(blobs, coord.first, coord.second)) {
                            uf.unionElements(blobCoords.first, other);
                    }
                }
            }
            // Clear merge data structure. We won't use it anymore.
            toMerge.clear();


            // Building a map from the union find result.
            // Associate every blob to it parent
            //or to itself if it is alone
            for (auto blob : blobs->_blobs) {
                parentSons[uf.find(blob)].insert(blob);
            }

            //Merge connected blobs
            //One blob is considered the parent of all the others.
            for (auto &pS : parentSons) {
                auto parent = pS.first;
                auto sons = pS.second;
                VLOG(4) << "nb of sons: " << sons.size(); //for debug

                if (sons.size() == 1) {
                    // if we have a lonely hole to merge, it is because it was connected to the background
                    // so we remove it.
                    if((*sons.begin())->isToMerge() && deleteLonelyBlobs){
                        for(auto s : sons){
                            delete s;
                            blobs->_blobs.remove(s);
                        }
                        sons.clear();
                    }
                    continue;
                }

                //To merge several blobs, we calculate the resulting bounding box and fill a bitmask of the same dimensions.
                auto bb = calculateBoundingBox(sons);
                double size = ceil((bb.getHeight() * bb.getWidth()) / 32.);
                auto *bitMask = new uint32_t[(uint32_t) size]();

                //the parent will contain the merged feature
                parent->addToBitMask(bitMask, bb);

                for (auto son : sons) {
                    if (son == parent) {
                        continue;
                    }
                    son->addToBitMask(bitMask, bb);
                    parent->setCount(parent->getCount() + son->getCount());
                    delete son; //we keep only the parent, we can delete the sons
                    blobs->_blobs.remove(son);
                }

                delete[] parent->getFeature()->getBitMask();
                delete parent->getFeature();
                auto *feature = new Feature(parent->getTag(), bb, bitMask);

                //In order to maintain consistency - this will be unecessary if we send back feature instead
                parent->setRowMin(bb.getUpperLeftRow());
                parent->setRowMax(bb.getBottomRightRow());
                parent->setColMin(bb.getUpperLeftCol());
                parent->setColMax(bb.getBottomRightCol());

                parent->setFeature(feature);
            }
        }

        BoundingBox calculateBoundingBox(std::set<Blob *> &sons) {

            uint32_t upperLeftRow = std::numeric_limits<int32_t>::max(),
                    upperLeftCol = std::numeric_limits<int32_t>::max(),
                    bottomRightRow = 0,
                    bottomRightCol = 0;

            for (auto son : sons) {
                auto bb = son->getFeature()->getBoundingBox();
                upperLeftRow = (bb.getUpperLeftRow() < upperLeftRow) ? bb.getUpperLeftRow() : upperLeftRow;
                upperLeftCol = (bb.getUpperLeftCol() < upperLeftCol) ? bb.getUpperLeftCol() : upperLeftCol;
                bottomRightRow = (bb.getBottomRightRow() > bottomRightRow) ? bb.getBottomRightRow() : bottomRightRow;
                bottomRightCol = bb.getBottomRightCol() > bottomRightCol ? bb.getBottomRightCol() : bottomRightCol;
            }

            return BoundingBox(upperLeftRow, upperLeftCol, bottomRightRow, bottomRightCol);
        }

        uint32_t
                _nbTiles = 0;                 ///< Images number of tiles

        std::map<Blob *, std::list<Coordinate >>
                _toMerge{};                   ///< Merge structure

        ListBlobs *
                _blobs{};                       ///< Blobs list


        uint32_t imageWidth{}, imageHeight{};

        std::map<Blob *, std::list<Coordinate >>
                _holesToMerge{};                   ///< Merge structure

        ListBlobs *
                _holes{};                       ///< Holes list

        uint32_t
                _count = 0;                   ///< Number of views analysed

        SegmentationOptions* segmentationOptions{};

        EGTOptions *options{};
    };
}

#endif //FEATURECOLLECTION_BLOBMERGER_H
