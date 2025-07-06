#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../libs/stb_image_write.h"
#include "filter.h"
#include <stdio.h>
#include <string.h>
#include <windows.h> 

typedef struct Image {
	unsigned char *data;
	int width, height;
	int channels;
} Image;

enum extension {
    PNG,
    JPG,
    UNKNOWN
};

void turn_grayscale(unsigned char* image, unsigned int pixel_count);
extension validate_path(const char* inputImagePath, const char* outputImagePath);

// Thread related struct and function
typedef struct ThreadArgs {
    unsigned char* image;
    unsigned int pixel_count;
} ThreadArgs;
DWORD WINAPI turn_grayscale_thread(LPVOID args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    turn_grayscale(threadArgs->image, threadArgs->pixel_count);
    return 0;
}

void invert_color(const char* inputImagePath, const char* outputImagePath) {
    extension ext = validate_path(inputImagePath, outputImagePath);
    if (UNKNOWN == ext) {
        fprintf(stderr, "Invalid file paths or extensions.\n");
        return;
    }
    Image img = { 0 };
    img.data = stbi_load(inputImagePath, &img.width, &img.height, &img.channels, 4);
    if (NULL == img.data) {
        fprintf(stderr, "Error loading image: %s\n", stbi_failure_reason());
        return;
    }
    for (int i = 0; i < img.width * img.height * img.channels; i++) {
		img.data[i] = 255 - img.data[i];
	}
    
    switch (ext) {
        case PNG:
            if (!stbi_write_png(outputImagePath, img.width, img.height, 4, img.data, img.width * 4)) {
                fprintf(stderr, "Error writing image: %s\n", outputImagePath);
            }
            break;
        case JPG:
            if (!stbi_write_jpg(outputImagePath, img.width, img.height, 4, img.data, 100)) {
                fprintf(stderr, "Error writing image: %s\n", outputImagePath);
            }
            break;
        default:
            fprintf(stderr, "Unsupported file extension.\n");
            break;
    }
    
    stbi_image_free(img.data);
    return;
}

void grayscale(const char* inputImagePath, const char* outputImagePath) {
    extension ext = validate_path(inputImagePath, outputImagePath);
    if (UNKNOWN == ext) {
        fprintf(stderr, "Invalid file paths or extensions.\n");
        return;
    }
    Image img = { 0 };
    img.data = stbi_load(inputImagePath, &img.width, &img.height, &img.channels, 4);
    if (NULL == img.data) {
        fprintf(stderr, "Error loading image: %s\n", stbi_failure_reason());
        return;
    }
    if (img.channels < 3) {
        fprintf(stderr, "File cannot be grayscaled. It must have at least 3 channels (RGB).\n");
        stbi_image_free(img.data);
        return;
    }
    unsigned int pixel_count = (unsigned int)((double)(img.width * img.height)/ 4.0);
    ThreadArgs threadArgs1;
    // thread 1
    threadArgs1.image = img.data;
    threadArgs1.pixel_count = pixel_count;
    HANDLE thread1 = CreateThread(NULL, 0, turn_grayscale_thread, &threadArgs1, 0, NULL);
    // thread 2
    ThreadArgs threadArgs2;
    threadArgs2.pixel_count = pixel_count;
    threadArgs2.image = img.data + 4 * pixel_count;
    HANDLE thread2 = CreateThread(NULL, 0, turn_grayscale_thread, &threadArgs2, 0, NULL);
    // thread 3
    ThreadArgs threadArgs3;
    threadArgs3.pixel_count = pixel_count;
    threadArgs3.image = img.data + 8 * pixel_count;
    HANDLE thread3 = CreateThread(NULL, 0, turn_grayscale_thread, &threadArgs3, 0, NULL);
    turn_grayscale(img.data + 12 * pixel_count, pixel_count);
    WaitForSingleObject(thread1, INFINITE);
    WaitForSingleObject(thread2, INFINITE);
    WaitForSingleObject(thread3, INFINITE);
    CloseHandle(thread1);
    CloseHandle(thread2);
    CloseHandle(thread3);
    switch (ext) {
        case PNG:
            if (!stbi_write_png(outputImagePath, img.width, img.height, 4, img.data, img.width * 4)) {
                fprintf(stderr, "Error writing image: %s\n", outputImagePath);
            }
            break;
        case JPG:
            if (!stbi_write_jpg(outputImagePath, img.width, img.height, 4, img.data, 100)) {
                fprintf(stderr, "Error writing image: %s\n", outputImagePath);
            }
            break;
        default:
            fprintf(stderr, "Unsupported file extension.\n");
            break;
    }
    stbi_image_free(img.data);
    return;
}

void turn_grayscale(unsigned char* image, unsigned int pixel_count) {
    for (int i = 0; i < pixel_count; ++i) {
        int idx = i << 2;
        unsigned char r = image[idx];
        unsigned char g = image[idx + 1];
        unsigned char b = image[idx + 2];

        unsigned char gray = (unsigned char)(
            0.299f * r + 0.587f * g + 0.114f * b
        );
        image[idx] = gray;       
        image[idx + 1] = gray;   
        image[idx + 2] = gray;   
    }
}

extension validate_path(const char* inputImagePath, const char* outputImagePath) {
    const char* ext = strrchr(inputImagePath, '.');
    if (ext == NULL) return UNKNOWN;
    if (strcmp(ext, ".png") == 0) {
        if (strstr(outputImagePath, ".png") != NULL) return PNG;
    } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
        if (strstr(outputImagePath, ".jpg") != NULL || strstr(outputImagePath, ".jpeg") != NULL) return JPG;
    }
    return UNKNOWN;
}



