#ifndef FILTER_H
#define FILTER_H
#include <windows.h>
#include <stdint.h>

#define DEFAULT 0
struct _Filter_Engine;

typedef _Filter_Engine* Filter_Engine;
	
typedef struct Image {
    unsigned char* data;
    uint32_t width, height;
    uint32_t channels;
} Image;

enum Work_Type {
	GRAYSCALE,
	INVERT,
	SEPIA,
	BOX_BLUR,
	GAUSSIAN_BLUR,
	EDGE,
	SCALE_UP,
	SCALE_DOWN
};


Filter_Engine filter_engine_create();														  // Creates and initializes the filter engine.
void filter_engine_initialize(Filter_Engine engine, size_t Arena_Size, size_t Thread_Count); // Initializes the filter engine with specified arena size and thread count. Use DEFAULT for default values.
void filter_engine_destroy(Filter_Engine engine);											  // Destroys the filter engine and frees resources.
void filter_engine_wait(Filter_Engine engine);												  // Waits for all filter operations to complete.

void filter_engine_grayscale(Filter_Engine engine, Image* input, Image* output);
void filter_engine_invert(Filter_Engine engine, Image* input, Image* output);
void filter_engine_sepia(Filter_Engine engine, Image* input, Image* output);

#endif