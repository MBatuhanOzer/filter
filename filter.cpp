#include "filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h> 
#include <assert.h>

// Threading threshold
constexpr uint64_t THRESHOLD = 720 * 1280; // Should not be less than WORK_ITEM_PIXELS

// Pixels per work item
constexpr unsigned int WORK_ITEM_PIXELS = 50000;

// Thread related structs and functions
typedef struct Work_Item {
	unsigned char* image;
	unsigned int pixel_count;
} WorkItem;

typedef struct Thread_Work_Context {
	Work_Item* works;
	unsigned int work_count;
	volatile int64_t work_index;
} Thread_Work_Context;

void grayscale_work(Work_Item* work);

DWORD WINAPI turn_grayscale_thread(void* params) {
	Thread_Work_Context* context = (Thread_Work_Context*)params;
	while (true) {
		int64_t our_item = InterlockedIncrement64(&context->work_index);
		our_item--;
		if (our_item >= context->work_count) break;
		Work_Item* work = &context->works[our_item];
		grayscale_work(work);
	}
	return 0;
}

Thread_Work_Context create_thread_work_context(Image* img) {
	Thread_Work_Context context = { 0 };
	unsigned int count = (img->width * img->height) / WORK_ITEM_PIXELS;
	unsigned int remainder = (img->width * img->height) % WORK_ITEM_PIXELS;
	context.work_count = count;
	context.work_index = 0;
	context.works = (Work_Item*)calloc(context.work_count, sizeof(Work_Item));
	for (int i = 0; i < count; ++i) {
		context.works[i].image = img->data + (i * WORK_ITEM_PIXELS * img->channels);
		context.works[i].pixel_count = WORK_ITEM_PIXELS;
	}
	if (remainder > 0) context.works[count - 1].pixel_count += remainder;

	return context;
}

void destroy_thread_work_context(Thread_Work_Context context) {
	free(context.works);
}

// Function to invert the colors of an image
void invert_color(Image* img) {
	for (int i = 0; i < img->width * img->height * img->channels; i++) {
		img->data[i] = 255 - img->data[i];
	}

	return;
}

// Function to convert an image to grayscale using multiple threads
void grayscale(Image* img) {
	assert(img->channels == 3); // If the image is not RGB, this will not work correctly.
	if (img->width * img->height <= THRESHOLD) {
		Work_Item work = { img->data, img->width * img->height };
		grayscale_work(&work);
		return;
	}
	assert(WORK_ITEM_PIXELS < THRESHOLD); // Ensure that the work item size is less than the threshold.
	Thread_Work_Context context = create_thread_work_context(img);
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	unsigned int thread_num = min((int)sys_info.dwNumberOfProcessors, context.work_count / 4);
	HANDLE *threads = (HANDLE *)calloc(thread_num, sizeof(HANDLE));
	for (unsigned int i = 0; i < thread_num; ++i) {
		threads[i] = CreateThread(NULL, 0, turn_grayscale_thread, &context, 0, NULL);
	}
	for (int i = 0; i < thread_num; i += MAXIMUM_WAIT_OBJECTS) {
		int count = min(thread_num - i, MAXIMUM_WAIT_OBJECTS);
		DWORD result = WaitForMultipleObjects(count, threads + i, TRUE, INFINITE);
		assert(result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS);
	}
	for (unsigned int i = 0; i < thread_num; ++i) {
		CloseHandle(threads[i]);
	}
	destroy_thread_work_context(context);

	return;
}

void grayscale_work(Work_Item* work) {
	for (int i = 0; i < work->pixel_count; ++i) {
		int idx = i * 3; // Assuming RGB format, each pixel has 3 channels
		unsigned char r = work->image[idx];
		unsigned char g = work->image[idx + 1];
		unsigned char b = work->image[idx + 2];
		unsigned char gray = (unsigned char)(0.299f * r + 0.587f * g + 0.114f * b);
		work->image[idx] = gray;
		work->image[idx + 1] = gray;
		work->image[idx + 2] = gray;
	}
}

















