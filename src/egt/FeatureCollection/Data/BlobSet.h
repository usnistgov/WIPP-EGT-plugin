//
// Created by gerardin on 9/18/19.
//

#ifndef NEWEGT_BLOBGROUP_H
#define NEWEGT_BLOBGROUP_H

#include <htgs/api/IData.hpp>
#include "Blob.h"

namespace egt {

    struct BlobSet : public htgs::IData {

        BlobSet(std::set<Blob *> &_blobs) : _blobs(_blobs) {}

        std::set<Blob*> _blobs{};


    };
}
#endif //NEWEGT_BLOBGROUP_H
