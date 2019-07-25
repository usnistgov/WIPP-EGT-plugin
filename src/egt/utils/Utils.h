//
// Created by Gerardin, Antoine D. (Assoc) on 12/19/18.
//

#ifndef PYRAMIDBUILDING_UTILS_H
#define PYRAMIDBUILDING_UTILS_H

#include <iostream>
#include <algorithm>
#include <glog/logging.h>
#include <iomanip>

namespace egt {

    std::string getFileExtension(const std::string &path) {
        std::string extension = path.substr(path.find_last_of('.') + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        return extension;
    }

    template<class T>
    void printArray(std::string title, T *data, uint32_t w, uint32_t h) {
        std::ostringstream oss;

        oss << title << std::endl;
        for (size_t i = 0; i < h; ++i) {
            for (size_t j = 0; j < w; ++j) {
                oss << std::setw(6) << (data[i * w + j]) << " ";
            }
            oss << std::endl;
        }
        oss << std::endl;

        VLOG(3) << oss.str();
    }


    template<class T>
    void printBoolArray(std::string title, T *data, uint32_t w, uint32_t h) {
        std::ostringstream oss;

        oss << title << std::endl;
        for (size_t i = 0; i < h; ++i) {
            for (size_t j = 0; j < w; ++j) {
                oss << std::setw(1) << ((data[i * w + j] > 0) ? 1 : 0) << " ";
            }
            oss << std::endl;
        }
        oss << std::endl;

        VLOG(0) << oss.str();
    }
}

#endif //PYRAMIDBUILDING_UTILS_H
