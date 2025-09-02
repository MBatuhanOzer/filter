#ifndef FILTER_H
#define FILTER_H

struct Image {
    unsigned char* data;
    unsigned int width, height;
    unsigned int channels;
};

enum Work_Type {
	GRAYSCALE,
	INVERSE,
	SEPIA,
	BOX_BLUR,
	GAUSSIAN_BLUR,
	EDGE,
	SCALE_UP,
	SCALE_DOWN
};

void filter_engine_start();
void filter_engine_stop();

void grayscale(Image* input, Image* output);
void invert(Image* input, Image* output);
void sepia(Image* input, Image* output);

#endif