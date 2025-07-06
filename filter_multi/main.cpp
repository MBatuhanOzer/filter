#include <stdio.h>
#include <string.h>
#include "filter.h"

struct Image {
	unsigned char *data;
	int width, height;
	int channels;
};

enum OperationType {
	OP_NONE,
	OP_GRAYSCALE,
	OP_INVERSE
};

OperationType operation_type_from_string(const char *op) {
	if (strcmp(op, "-g") == 0) {
		return OP_GRAYSCALE;
	} else if (strcmp(op, "-i") == 0) {
		return OP_INVERSE;
	}
	return OP_NONE;
}

int main(int argc, char **argv) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s [-g|-i] <input_file> <output_file>\n", argv[0]);
		return 1;
	}

	OperationType operation = operation_type_from_string(argv[1]);


	if (operation == OP_NONE) {
		fprintf(stderr, "Invalid operation. Use 'grayscale' or 'inverse'.\n");
		return 1;
	}

	switch (operation) {
		case(OP_GRAYSCALE):
			grayscale(argv[2], argv[3]);
			break;
		case (OP_INVERSE):
			invert_color(argv[2], argv[3]);
			break;
		default:
			fprintf(stderr, "Unknown operation.\n");
			break;
	}

	return 0;
}