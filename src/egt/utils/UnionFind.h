//
// Created by gerardin on 9/18/19.
//

#ifndef NEWEGT_UNIONFIND_H
#define NEWEGT_UNIONFIND_H

#include <set>
#include <list>
#include <egt/FeatureCollection/Data/Blob.h>

namespace egt {
/// \namespace fc FeatureCollection namespace

/// \class UnionFind UnionFind.h <FastImage/FeatureCollection/tools/UnionFind.h>
/// \brief Templatized Union-Find algorithm as described
/// https://en.wikipedia.org/wiki/Disjoint-set_data_structure
/// \tparam T File type
    template<class T>
    class UnionFind {
    public:
        /// \brief Find the element parent, and compress the path
        /// \param elem Element to look for the parent
        /// \return this if has not parent or parent address
        T *find(T *elem) {
            if (elem->getParent() != elem) {
                elem->setParent(find(elem->getParent()));
            }
            return elem->getParent();
        }

        /// \brief Unify to elements
        /// \param elem1 element 1 to unify
        /// \param elem2 element 2 to unify
        Blob* unionElements(T *elem1, T *elem2) {
            auto root1 = find(elem1);
            auto root2 = find(elem2);
            if (root1 != root2) {
                if (root1->getRank() < root2->getRank()) {
                    root1->setParent(root2);
                    return root2;
                } else {
                    root2->setParent(root1);
                    if (root1->getRank() == root2->getRank()) {
                        root1->setRank(root1->getRank() + 1);
                    }
                }
            }
            return root1;
        }

    };
}

#endif //NEWEGT_UNIONFIND_H
