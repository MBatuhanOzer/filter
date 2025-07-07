
#include "filter.h"
#include <stdio.h>
#include <string.h>
#include <windows.h> 

// Function declarations
void turn_grayscale(unsigned char* image, unsigned int pixel_count);

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

void invert_color(Image *img) {
    for (int i = 0; i < img->width * img->height * 4; i++) {
		img->data[i] = 255 - img->data[i];
	}

    return;
}

void grayscale(Image *img) {
    unsigned int pixel_count = (unsigned int)((double)(img->width * img->height)/ 4.0);
    ThreadArgs threadArgs1 = { 0 };
    // thread 1
    threadArgs1.image = img->data;
    threadArgs1.pixel_count = pixel_count;
    HANDLE thread1 = CreateThread(NULL, 0, turn_grayscale_thread, &threadArgs1, 0, NULL);
    // thread 2
    ThreadArgs threadArgs2 = { 0 };
    threadArgs2.pixel_count = pixel_count;
    threadArgs2.image = img->data + 4 * pixel_count;
    HANDLE thread2 = CreateThread(NULL, 0, turn_grayscale_thread, &threadArgs2, 0, NULL);
    // thread 3
    ThreadArgs threadArgs3 = { 0 };
    threadArgs3.pixel_count = pixel_count;
    threadArgs3.image = img->data + 8 * pixel_count;
    HANDLE thread3 = CreateThread(NULL, 0, turn_grayscale_thread, &threadArgs3, 0, NULL);
    turn_grayscale(img->data + 12 * pixel_count, pixel_count);
    WaitForSingleObject(thread1, INFINITE);
    WaitForSingleObject(thread2, INFINITE);
    WaitForSingleObject(thread3, INFINITE);
    CloseHandle(thread1);
    CloseHandle(thread2);
    CloseHandle(thread3);
    
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





