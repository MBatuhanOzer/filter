#include <stdio.h>
#include <stdint.h>
#include <windows.h> // For high-resolution timing
#define STB_IMAGE_IMPLEMENTATION
#include "../dependencies/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../dependencies/stb/stb_image_write.h"
#include "../src/filter-engine/filter.h"

// C++ specific libraries
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
	double elapsed_s, elapsed_ms;
	uint64_t freq, start_time, end_time;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);
	Filter_Engine engine = filter_engine_create();
	assert(engine && "There should be engine to begin.");
	QueryPerformanceCounter((LARGE_INTEGER*)&end_time);
	elapsed_s = ((double)end_time - start_time) / (double)freq;
	elapsed_ms = elapsed_s * 1000.0;
	printf("Filter engine initialization took %.3fms\n", elapsed_ms);
	fs::path input_dir = "../../../tests/images/input";
	if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
		fprintf(stderr, "Input directory does not exist or is not a directory: %s\n", input_dir.string().c_str());
		return -1;
	}
	fs::path output_dir = "../../../tests/images/output";
	if (!fs::exists(output_dir)) {
		fs::create_directories(output_dir);
	}
	for (const auto& entry : fs::directory_iterator(input_dir)) {
		if (entry.is_regular_file()) {
			fs::path input_path = entry.path();
			std::string filename = input_path.filename().string();
			std::string output_path = (output_dir / filename).string();
			Image input_image;
			input_image.data = stbi_load(input_path.string().c_str(), (int*)&input_image.width, (int*)&input_image.height, (int*)&input_image.channels, 3);
			if (input_image.data == NULL) {
				fprintf(stderr, "Failed to load image: %s\n", input_path.string().c_str());
				continue;
			}
			Image output_image;
			output_image.data = (unsigned char*)calloc(input_image.width * input_image.height * input_image.channels, sizeof(unsigned char));
			output_image.width = input_image.width;
			output_image.height = input_image.height;
			output_image.channels = input_image.channels;
			if (output_image.data == NULL) {
				fprintf(stderr, "Failed to allocate memory for output image: %s\n", output_path.c_str());
				stbi_image_free(input_image.data);
				continue;
			}

			QueryPerformanceCounter((LARGE_INTEGER*)&start_time);
			filter_engine_invert(engine, &input_image, &output_image);
			filter_engine_wait(engine);
			QueryPerformanceCounter((LARGE_INTEGER*)&end_time);
			elapsed_s = ((double)end_time - start_time) / (double)freq;
			elapsed_ms = elapsed_s * 1000.0;
			printf("Inversion of %s took %.3fms\n", filename.c_str(), elapsed_ms);
			stbi_write_jpg(output_path.c_str(), output_image.width, output_image.height, output_image.channels, output_image.data, 100);

			QueryPerformanceCounter((LARGE_INTEGER*)&start_time);
			filter_engine_grayscale(engine, &input_image, &output_image);
			filter_engine_wait(engine);
			QueryPerformanceCounter((LARGE_INTEGER*)&end_time);
			elapsed_s = ((double)end_time - start_time) / (double)freq;
			elapsed_ms = elapsed_s * 1000.0;
			printf("Grayscale of %s took %.3fms\n", filename.c_str(), elapsed_ms);
			stbi_write_jpg(output_path.c_str(), output_image.width, output_image.height, output_image.channels, output_image.data, 100);

			QueryPerformanceCounter((LARGE_INTEGER*)&start_time);
			filter_engine_sepia(engine, &input_image, &output_image);
			filter_engine_wait(engine);
			QueryPerformanceCounter((LARGE_INTEGER*)&end_time);
			elapsed_s = ((double)end_time - start_time) / (double)freq;
			elapsed_ms = elapsed_s * 1000.0;
			printf("Sepia of %s took %.3fms\n", filename.c_str(), elapsed_ms);
			stbi_write_jpg(output_path.c_str(), output_image.width, output_image.height, output_image.channels, output_image.data, 100);

			stbi_image_free(input_image.data);
			free(output_image.data);
		}
	}

	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);
	filter_engine_destroy(engine);
	QueryPerformanceCounter((LARGE_INTEGER*)&end_time);
	elapsed_s = ((double)end_time - start_time) / (double)freq;
	elapsed_ms = elapsed_s * 1000.0;
	printf("Filter engine destruction took %.3fms\n", elapsed_ms);

	return 0;
}