#include "filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h> 
#include <assert.h>

// Number of chunks to divide the image into for processing
constexpr int WORK_ITEMS = 5000;

// Number of threads to use for processing
constexpr int THREAD_COUNT = 250;

// Thread related structs and functions
typedef struct WorkItem {
	unsigned char* image;
	unsigned int pixel_count;
} WorkItem;
typedef struct Thread_Work_Context {
	WorkItem* works;
	unsigned int work_count;
	volatile int64_t work_index;
} Thread_Work_Context;
void grayscale_work(WorkItem* work);
DWORD WINAPI turn_grayscale_thread(void *params) {
	Thread_Work_Context* context = (Thread_Work_Context*)params;
	while (true) {
		int64_t our_item = InterlockedIncrement64(&context->work_index);
		our_item--;
		if (our_item >= context->work_count) break;
		WorkItem *work = &context->works[our_item];
		grayscale_work(work);
	}
	return 0;
}
Thread_Work_Context create_thread_work_context(Image* img) {
	Thread_Work_Context context = { 0 };
	unsigned int work_chunk = (img->width * img->height) / WORK_ITEMS;
	unsigned int remainder = (img->width * img->height) % WORK_ITEMS;
	context.work_count = WORK_ITEMS;
	context.work_index = 0;
	context.works = (WorkItem*)calloc(context.work_count, sizeof(WorkItem));
	for (int i = 0; i < WORK_ITEMS; ++i) {
		context.works[i].image = img->data + (i * work_chunk * 4);
		context.works[i].pixel_count = work_chunk;
	}
	if (remainder > 0) context.works[WORK_ITEMS - 1].pixel_count += remainder;

	return context;
}
void destroy_thread_work_context(Thread_Work_Context context) {
	free(context.works);
}

// Function to invert the colors of an image
void invert_color(Image *img) {
	for (int i = 0; i < img->width * img->height * 4; i++) {
		img->data[i] = 255 - img->data[i];
	}

	return;
}

// Function to convert an image to grayscale using multiple threads
void grayscale(Image* img) {
	Thread_Work_Context context = create_thread_work_context(img);
	HANDLE threads[THREAD_COUNT] = { 0 };
	for (unsigned int i = 0; i < THREAD_COUNT; ++i) {
		threads[i] = CreateThread(NULL, 0, turn_grayscale_thread, &context, 0, NULL);
	}
	for (int i = 0; i < THREAD_COUNT; i += MAXIMUM_WAIT_OBJECTS) {
		int count = min(THREAD_COUNT - i, MAXIMUM_WAIT_OBJECTS);
		DWORD result = WaitForMultipleObjects(count, threads + i, TRUE, INFINITE);
		assert(result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS);
	}
	for (unsigned int i = 0; i < THREAD_COUNT; ++i) {
		CloseHandle(threads[i]);
	}
	destroy_thread_work_context(context);

	return;
}

void grayscale_work(WorkItem *work) {
	for (int i = 0; i < work->pixel_count; ++i) {
		int idx = i << 2;
		unsigned char r = work->image[idx];
		unsigned char g = work->image[idx + 1];
		unsigned char b = work->image[idx + 2];
		unsigned char gray = (unsigned char)(
			0.299f * r + 0.587f * g + 0.114f * b
		);
		work->image[idx] = gray;       
		work->image[idx + 1] = gray;   
		work->image[idx + 2] = gray;   
	}
}

















