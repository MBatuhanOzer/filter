#include "filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <Windows.h> 
#include <assert.h>

// Structures for threading and work management
typedef struct Work_Item {
	unsigned char* image;
	unsigned char* output;
	unsigned int width, height;
	Filter_Function function;
} Work_Item;

typedef struct Thread_Work_Context {
	Work_Item* works;
	unsigned int work_count;
	volatile int64_t work_index;
} Thread_Work_Context;

typedef struct Work_Context_Node {
	Thread_Work_Context context;
	Work_Context_Queue* next;
} Work_Context_Node;

typedef struct Context_Controller {
	Work_Context_Node* head;
	Work_Context_Node* tail;
	CRITICAL_SECTION cs;
	CONDITION_VARIABLE cv_start;
	CONDITION_VARIABLE cv_done;
	BOOL shutdown;
} Context_Controller;

// Threading threshold (by rows)
constexpr uint64_t THRESHOLD = 500; // Should not be less than WORK_ITEM_ROWS

// Rows per work item
constexpr unsigned int WORK_ITEM_ROWS = 100;

static_assert(WORK_ITEM_ROWS < THRESHOLD);

unsigned int THREAD_NUM = 0;

// Thread handle array
static HANDLE* THREADS = NULL;

// Global controller for managing work contexts
static Context_Controller* CONTROLLER;

// Generic filter function type
typedef void (*Filter_Function)(Work_Item* work);

static DWORD _stdcall thread_proc(void* params) {
	while (true) {
		Work_Context_Node* node = NULL;
		EnterCriticalSection(&CONTROLLER->cs);
		if (CONTROLLER->shutdown) {
			LeaveCriticalSection(&CONTROLLER->cs);
			return 0;
		}
		while (CONTROLLER->head == NULL) {
			SleepConditionVariableCS(&CONTROLLER->cv_start, &CONTROLLER->cs, INFINITE);
		}
		if (CONTROLLER->shutdown) {
			LeaveCriticalSection(&CONTROLLER->cs);
			return 0;
		}
		node = CONTROLLER->head;
		LeaveCriticalSection(&CONTROLLER->cs);
		if (node != NULL) {
			Thread_Work_Context* context = &node->context;
			while (context->work_index < context->work_count) {
				unsigned int index = InterlockedIncrement64(&context->work_index) - 1;
				if (index < context->work_count) {
					context->works[index].function(&context->works[index]);
				}
			}
		}
	}
}

void filter_engine_start() {
	CONTROLLER.head = NULL;
	CONTROLLER.tail = NULL;
	InitializeCriticalSection(&CONTROLLER.cs);
	InitializeConditionVariable(&CONTROLLER.cv_start);
	InitializeConditionVariable(&CONTROLLER.cv_done);
	CONTROLLER.shutdown = false;
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	THREAD_NUM = sys_info.dwNumberOfProcessors;
	if (THREADS != NULL)  filter_engine_stop(); 
	THREADS = (HANDLE*)calloc(THREAD_NUM, sizeof(HANDLE));
	for (unsigned int i = 0; i < THREAD_NUM; ++i) {
		THREADS[i] = CreateThread(NULL, 0, thread_proc, NULL, 0, NULL);
		if (THREADS[i] == NULL) {
			fprintf(stderr, "Failed to create thread %u\n", i);
			exit(EXIT_FAILURE);
		}
	}
}

void filter_engine_stop() {
	if (THREADS) {
		for (unsigned int i = 0; i < _countof(THREADS); ++i) {
			if (THREADS[i]) {
				WaitForSingleObject(THREADS[i], INFINITE);
				CloseHandle(THREADS[i]);
			}
		}
		free(THREADS);
		THREADS = NULL;
	}
}

static void push_work_context(Context_Controller* controller, Thread_Work_Context context) {
	
}

static Thread_Work_Context* pop_work_context(Context_Controller* controller, Thread_Work_Context* context) {
	
}

static Thread_Work_Context create_thread_work_context(Image* input, unsigned char* output, Work_Type type) {
	Thread_Work_Context context = { 0 };
	Filter_Function function;
	function = get_filter_function(type);
	if (function == NULL) exit(EXIT_FAILURE);
	unsigned int count = input->height/ WORK_ITEM_ROWS;
	unsigned int remainder = input->height % WORK_ITEM_ROWS;
	context.work_count = count;
	context.work_index = 0;
	context.works = (Work_Item*)calloc(context.work_count, sizeof(Work_Item));
	for (int i = 0; i < count; ++i) {
		context.works[i].image = input->data + (i * WORK_ITEM_ROWS * img->channels);
		context.works[i].output = output-data + (i * WORK_ITEM_ROWS * img->channels);
		context.works[i].width = input->width;
		context.works[i].height = WORK_ITEM_ROWS;
		context.works[i].function = function;
	}
	if (remainder > 0) context.works[count - 1].height += remainder;

	return context;
}

static void destroy_thread_work_context(Thread_Work_Context context) {
	free(context.works);
}

// Function to invert the colors of an image
void invert(Image* input, Image* output) {
	assert(input->channels == 3);
	if (input->height <= THRESHOLD) {
		Work_Item work = { input->data, output->data, input->width, input->height, invert_color_work };
		work->function(&work);
		return;
	}
}

// Function to convert an image to grayscale
void grayscale(Image* input, Image* output) {
	assert(input->channels == 3);
	if (input->height <= THRESHOLD) {
		Work_Item work = { input->data, output->data, input->width, input->height, grayscale_work};
		work->function(&work);
		return;
	}
	return;
}

// Function to convert an image to sepia
void sepia(Image* input, Image* output) {
	assert(input->channels == 3);
	if (input->height <= THRESHOLD) {
		Work_Item work = { input->data, output->data, input->width, input->height, sepia_work };
		work->function(&work);
		return;
	}
	return;
}

static void invert_color_work(Work_Item* work) {
	for (int i = 0; i < work->width * work->height; ++i) {
		int idx = i * 3; // Assuming RGB format, each pixel has 3 channels
		work->output[idx] = 255 - work->image[idx];
		work->output[idx + 1] = 255 - work->image[idx + 1];
		work->output[idx + 2] = 255 - work->image[idx + 2];
	}
}

static void grayscale_work(Work_Item* work) {
	for (int i = 0; i < work->pixel_count; ++i) {
		int idx = i * 3; // Assuming RGB format, each pixel has 3 channels
		unsigned char r = work->image[idx];
		unsigned char g = work->image[idx + 1];
		unsigned char b = work->image[idx + 2];
		unsigned char gray = (unsigned char)(0.299f * r + 0.587f * g + 0.114f * b);
		work->output[idx] = gray;
		work->output[idx + 1] = gray;
		work->output[idx + 2] = gray;
	}
}

static void sepia_work(Work_Item* work) {
	for (int i = 0; i < work->pixel_count; ++i) {
		int idx = i * 3; // Assuming RGB format, each pixel has 3 channels
		unsigned char r = work->image[idx];
		unsigned char g = work->image[idx + 1];
		unsigned char b = work->image[idx + 2];
		work->output[idx] = (unsigned char)min(255, (int)(0.393f * r + 0.769f * g + 0.189f * b));
		work->output[idx + 1] = (unsigned char)min(255, (int)(0.349f * r + 0.686f * g + 0.168f * b));
		work->output[idx + 2] = (unsigned char)min(255, (int)(0.272f * r + 0.534f * g + 0.131f * b));
	}
}

static inline Filter_Function get_filter_function(Work_Type type) {
	switch (type) {
		case GRAYSCALE: return grayscale_work;
		case INVERSE: return invert_color_work;
		case SEPIA: return sepia_work;
		default: return NULL;
	}
}















