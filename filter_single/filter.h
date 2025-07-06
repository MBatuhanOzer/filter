#ifndef FILTER_H
#define FILTER_H
#include <string>

void invert (const std::string& inputImagePath, const std::string& outputImagePath);
void grayscale (const std::string& inputImagePath, const std::string& outputImagePath);

#endif