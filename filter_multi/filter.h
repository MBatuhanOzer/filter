#ifndef FILTER_H
#define FILTER_H

struct Image {
    unsigned char* data;
    int width, height;
    int channels;
};

void grayscale(Image *img);
void invert_color(Image *img);

#endif