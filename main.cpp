#include "filter.h"
#include <iostream>
#include <string>
#include <regex>


int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <FLAG> <input_image> <output_image_path?>" << std::endl;
        return 1;
    }
    std::string flag = argv[1];
    std::string inputImagePath = argv[2];
    std::string outputImagePath;
    if (argc == 3) outputImagePath = inputImagePath;
    else outputImagePath = argv[3];

    std::regex flagRegex("^-([a-zA-Z])$");
    if (!std::regex_match(flag, flagRegex)) {
        std::cerr << "Incorrect flag usage. Usage: -<char>" << std::endl;
        return 1;
    } 
    char filterFlag = flag[1];
    try {
        switch(filterFlag) {
            case 'i': invert(inputImagePath, outputImagePath); break;
            case 'g': grayscale(inputImagePath, outputImagePath); break;
            default: std::cerr << "Unknown flag: -" << filterFlag << std::endl; return 1;
        }
    } catch (const std::string& e) {
        std::cerr << "Error: " << e << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Filter applied successfully." << std::endl;
    std::cout << "Output Image: " << outputImagePath << std::endl;
    return 0;
}