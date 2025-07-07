#include "filter.h"

#include <iostream>
#include <string>
#include <regex>

void invert_color(Image *img) {
    for (int i = 0; i < img->width * img->height * 4; ++i) {
        img->data[i] = 255 - img->data[i];
    } 
    return;
}

void grayscale(Image *img) {
    for (int i = 0; i < img->width * img->height; ++i) {
        int idx = i << 2;
        unsigned char r = img->data[idx];
        unsigned char g = img->data[idx + 1];
        unsigned char b = img->data[idx + 2];

        unsigned char gray = static_cast<unsigned char>(
            0.299f * r + 0.587f * g + 0.114f * b
        );
        img->data[idx] = gray;       
        img->data[idx + 1] = gray;   
        img->data[idx + 2] = gray;   
    }
    return;
}