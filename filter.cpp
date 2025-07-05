#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"
#include "filter.h"

#include <iostream>
#include <string>
#include <regex>

enum extension {
    PNG,
    JPG,
    UNKNOWN
};

extension validate_path(const std::string& inputImagePath, const std::string& outputImagePath);

void invert(const std::string& inputImagePath, const std::string& outputImagePath) {
    extension ext = validate_path(inputImagePath, outputImagePath);
    if (UNKNOWN == ext) throw "Invalid file paths or extensions.";
    int width, height, channels;
    unsigned char* image = stbi_load(inputImagePath.c_str(), &width, &height, &channels, 4);
     if (!image) {
        throw "Error loading image: " + inputImagePath;
        return;
    }
    for (int i = 0; i < width * height * 4; ++i) {
        image[i] = 255 - image[i];
    } 
    switch (ext)
    {
        case PNG:
            if (!stbi_write_png(outputImagePath.c_str(), width, height, 4, image, width * 4)) throw "Error writing image: " + outputImagePath;
            break;
        case JPG:
            if (!stbi_write_jpg(outputImagePath.c_str(), width, height, 4, image, 100)) throw "Error writing image: " + outputImagePath;
            break;
        default:
            throw "Unsupported file extension.";
    }
    stbi_image_free(image);
}

void grayscale(const std::string& inputImagePath, const std::string& outputImagePath) {
    extension ext = validate_path(inputImagePath, outputImagePath);
    if (UNKNOWN == ext) throw "Invalid file paths or extensions.";
    int width, height, channels;
    unsigned char* image = stbi_load(inputImagePath.c_str(), &width, &height, &channels, 4);
    if (!image) throw "Error loading image: " + inputImagePath;
    if (channels < 3) {
        throw "File cannot be grayscaled. It must have at least 3 channels (RGB).";
        stbi_image_free(image);
        return;
    }
    for (int i = 0; i < width * height; ++i) {
        int idx = i << 2;
        unsigned char r = image[idx];
        unsigned char g = image[idx + 1];
        unsigned char b = image[idx + 2];

        unsigned char gray = static_cast<unsigned char>(
            0.299f * r + 0.587f * g + 0.114f * b
        );
        image[idx] = gray;       
        image[idx + 1] = gray;   
        image[idx + 2] = gray;   
    }

    switch (ext)
    {
        case PNG:
            if (!stbi_write_png(outputImagePath.c_str(), width, height, 4, image, width * 4)) throw "Error writing image: " + outputImagePath;
            break;
        case JPG:
            if (!stbi_write_jpg(outputImagePath.c_str(), width, height, 4, image, 100)) throw "Error writing image: " + outputImagePath;
            break;
        default:
            throw "Unsupported file extension.";
    }
    stbi_image_free(image);
}

extension validate_path(const std::string& inputImagePath, const std::string& outputImagePath) {
    std::regex imageRegex(".*\\.(png|jpg)$", std::regex_constants::icase);
    if (!std::regex_match(inputImagePath, imageRegex)) return UNKNOWN; 
    if (!std::regex_match(outputImagePath, imageRegex)) return UNKNOWN;
    size_t ipos = inputImagePath.find_last_of('.');
    size_t opos = outputImagePath.find_last_of('.');
    std::string inputExt = inputImagePath.substr(ipos + 1);
    std::string outputExt = outputImagePath.substr(opos + 1);
    if (inputExt != outputExt) return UNKNOWN;

    return (inputExt == "png") ? PNG : JPG;
}