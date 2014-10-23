//=======================================================================
// Copyright (c) 2014 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef ICDAR_READER_HPP
#define ICDAR_READER_HPP

#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>
#include <memory>
#include <string>

#include "jpeglib.h"

namespace icdar {

struct ICDAR_Rectangle {
    std::size_t left;
    std::size_t top;
    std::size_t right;
    std::size_t bottom;
    std::string text;
};

struct ICDAR_pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

template<template<typename...> class Container>
struct ICDAR_label {
    Container<ICDAR_Rectangle> rectangles;
};

template<template<typename...> class Container>
struct ICDAR_image {
    std::size_t width;
    std::size_t height;
    Container<ICDAR_pixel> pixels;

    ICDAR_image(std::size_t width, std::size_t height) : width(width), height(height), pixels(width * height) {}
};

template<template<typename...> class Container = std::vector, template<typename...> class Sub = std::vector>
struct ICDAR_dataset {
    Container<ICDAR_image<Sub>> training_images;
    Container<ICDAR_image<Sub>> test_images;
    Container<ICDAR_label<Sub>> training_labels;
    Container<ICDAR_label<Sub>> test_labels;

    void resize_training(std::size_t new_size){
        if(training_images.size() > new_size){
            training_images.resize(new_size);
            training_labels.resize(new_size);
        }
    }

    void resize_test(std::size_t new_size){
        if(test_images.size() > new_size){
            test_images.resize(new_size);
            test_labels.resize(new_size);
        }
    }
};

template<template<typename...> class Container = std::vector, template<typename...> class Sub = std::vector>
Container<ICDAR_image<Sub>> read_images(const std::string& directory, const std::string& prefix, std::size_t first, std::size_t total, std::size_t limit = 0){
    auto max = limit == 0 ? total : std::min(total, limit);

    Container<ICDAR_image<Sub>> images;
    images.reserve(max);

    for(std::size_t image = first; image < first + max; ++image){
        std::string name = prefix + std::to_string(image) + ".jpg";
        std::string path = directory + "/" + name;

        auto in_file = fopen(path.c_str(), "rb");

        if(!in_file){
            std::cout << "Error reading image: " << path << std::endl;
            break;
        }

        struct jpeg_decompress_struct cinfo;

        jpeg_error_mgr jerr;
        cinfo.err = jpeg_std_error(&jerr);

        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, in_file);
        jpeg_read_header(&cinfo, true);
        jpeg_start_decompress(&cinfo);

        auto row_stride = cinfo.output_width * cinfo.output_components;

        images.emplace_back(cinfo.output_width, cinfo.output_height);

        JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

        while (cinfo.output_scanline < cinfo.output_height) {
            jpeg_read_scanlines(&cinfo, buffer, 1);

            for(std::size_t i = 0; i < cinfo.output_width; ++i){
                auto& pixel = images.back().pixels[(cinfo.output_scanline - 1) * cinfo.output_width + i];

                pixel.r = buffer[0][i*cinfo.output_components];
                pixel.g = buffer[0][i*cinfo.output_components+1];
                pixel.b = buffer[0][i*cinfo.output_components+2];
            }
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(in_file);
    }

    return images;
}

template<template<typename...> class Container = std::vector, template<typename...> class Sub = std::vector>
Container<ICDAR_label<Sub>> read_labels(const std::string& directory, const std::string& prefix, std::size_t first, std::size_t total, bool csv, std::size_t limit = 0){
    auto max = limit == 0 ? total : std::min(total, limit);

    Container<ICDAR_label<Sub>> labels;
    labels.reserve(max);

    for(std::size_t label = first; label < first + max; ++label){
        std::string name = prefix + std::to_string(label) + ".txt";
        std::string path = directory + "/" + name;

        std::ifstream file(path);

        if(!file){
            std::cout << "Error reading label: " << path << std::endl;
            break;
        }

        labels.emplace_back();

        std::string line;
        while (std::getline(file, line)){
            ICDAR_Rectangle rect;

            char sep = ',';

            if(!csv){
                sep = ' ';
            }

            auto start = 0;
            auto pos = line.find(sep, start);

            rect.left = std::stoll(std::string(line.begin() + start, line.begin() + pos));

            start = pos+1;
            pos = line.find(sep, start);
            rect.top = std::stoll(std::string(line.begin() + start, line.begin() + pos));

            start = pos+1;
            pos = line.find(sep, start);
            rect.right = std::stoll(std::string(line.begin() + start, line.begin() + pos));

            start = pos+1;
            pos = line.find(sep, start);
            rect.bottom = std::stoll(std::string(line.begin() + start, line.begin() + pos));

            rect.text = std::string(line.begin() + pos + 2, line.end());

            labels.back().rectangles.emplace_back(std::move(rect));
        }
    }

    return labels;
}

template<template<typename...> class Container = std::vector, template<typename...> class Sub = std::vector>
ICDAR_dataset<Container, Sub> read_2013_dataset(const std::string& train_directory, const std::string& test_directory, std::size_t training_limit = 0, std::size_t test_limit = 0){
    ICDAR_dataset<Container, Sub> dataset;

    dataset.training_labels = read_labels<Container, Sub>(train_directory, "gt_", 100, 229, false, training_limit);
    dataset.training_images = read_images<Container, Sub>(train_directory, "", 100, 229, training_limit);

    dataset.test_labels = read_labels<Container, Sub>(test_directory, "gt_img_", 1, 233, true, test_limit);
    dataset.test_images = read_images<Container, Sub>(test_directory, "img_", 1, 233, test_limit);

    return std::move(dataset);
}

} //end of icdar namespace

#endif //ICDAR_READER_HPP