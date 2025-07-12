#include <stdio.h>
#include <string.h>
#include "filter.h"
#define STB_IMAGE_IMPLEMENTATION
#include "./libs/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./libs/stb_image_write.h"

// Channels for use in image processing
#define CHANNELS 3

enum OperationType {
	OP_NONE,
	OP_GRAYSCALE,
	OP_INVERSE
};

enum Extension {
	PNG,
	JPG,
	UNKNOWN
};

OperationType operation_type_from_string(const char* op);
Extension validate_path(const char* inputImagePath, const char* outputImagePath);

int main(int argc, char** argv) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s [-g|-i] <input_file> <output_file>\n", argv[0]);
		return 1;
	}
	OperationType operation = operation_type_from_string(argv[1]);
	if (operation == OP_NONE) {
		fprintf(stderr, "Invalid operation. Use '-g' for grayscale or '-i' for inverse.\n");
		return 1;
	}

	Extension ext = validate_path(argv[2], argv[3]);
	if (UNKNOWN == ext) {
		fprintf(stderr, "Invalid file paths or extensions.\n");
		return 1;
	}
	Image img = { 0 };
	img.data = stbi_load(argv[2], &img.width, &img.height, NULL, CHANNELS);
	img.channels = CHANNELS;
	if (NULL == img.data) {
		fprintf(stderr, "Error loading image: %s\n", stbi_failure_reason());
		return 1;
	}

	switch (operation) {
	case(OP_GRAYSCALE):
		grayscale(&img);
		break;
	case (OP_INVERSE):
		invert_color(&img);
		break;
	default:
		fprintf(stderr, "Unknown operation.\n");
		stbi_image_free(img.data);
		return 1;
	}

	switch (ext) {
	case PNG:
		if (!stbi_write_png(argv[3], img.width, img.height, CHANNELS, img.data, img.width * CHANNELS)) {
			fprintf(stderr, "Error writing image: %s\n", argv[3]);
		}
		else {
			printf("Image saved successfully as %s\n", argv[3]);
		}
		break;
	case JPG:
		if (!stbi_write_jpg(argv[3], img.width, img.height, CHANNELS, img.data, 100)) {
			fprintf(stderr, "Error writing image: %s\n", argv[3]);
		}
		else {
			printf("Image saved successfully as %s\n", argv[3]);
		}
		break;
	default:
		fprintf(stderr, "Unsupported file extension.\n");
		break;
	}
	stbi_image_free(img.data);

	return 0;
}

Extension validate_path(const char* inputImagePath, const char* outputImagePath) {
	const char* ext = strrchr(inputImagePath, '.');
	if (ext == NULL) return UNKNOWN;
	if (strcmp(ext, ".png") == 0) {
		if (strstr(outputImagePath, ".png") != NULL) return PNG;
	}
	else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
		if (strstr(outputImagePath, ".jpg") != NULL || strstr(outputImagePath, ".jpeg") != NULL) return JPG;
	}
	return UNKNOWN;
}

OperationType operation_type_from_string(const char* op) {
	if (strcmp(op, "-g") == 0) {
		return OP_GRAYSCALE;
	}
	else if (strcmp(op, "-i") == 0) {
		return OP_INVERSE;
	}
	return OP_NONE;
}