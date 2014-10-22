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

namespace icdar {

struct ICDAR_Rectangle {
    std::string text;
    std::size_t left;
    std::size_t top;
    std::size_t right;
    std::size_t bottom;
};

struct icdar_pixel {
    std::size_t r;
    std::size_t g;
    std::size_t b;
};

template<template<typename...> class Container>
struct ICDAR_label {
    Container<ICDAR_Rectangle> rectangles;
};

template<template<typename...> class Container>
struct ICDAR_image {
    Sub<ICDAR_pixel> rectangles;
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

} //end of icdar namespace

#endif //ICDAR_READER_HPP