#include "filter.h"
#include <stdio.h>
#include <stdlib.h> // for calloc, free
#include <stdint.h>
#include <string.h>
#include <Windows.h> // for threading and synchronization
#include <assert.h>

const size_t DEFAULT_ARENA_SIZE = 64;

const uint32_t THRESHOLD = 100;

const uint32_t WORK_ITEM_ROWS = 50;

// Structures for threading and work management
typedef struct Thread_Context {
	HANDLE* threads;
	size_t thread_count;
};

typedef struct Work_Item {
	unsigned char* image;
	unsigned char* output;
	uint32_t width, height;
	Filter_Function function;
} Work_Item;

typedef struct Work_Context {
	Work_Item* works;
	uint32_t work_count;
	volatile uint32_t work_done;
	volatile uint32_t work_index;
} Work_Context;

typedef struct Work_Context_Node {
	Work_Context context;
	Work_Context_Node* next;
} Work_Context_Node;

typedef struct Work_Context_Node_Arena {
	Work_Context_Node* nodes;
	Work_Context_Node* free_list;
	size_t size;
} Work_Context_Node_Arena;

typedef struct Work_Context_Controller {
	Work_Context_Node_Arena arena;
	Work_Context_Node* head;
	Work_Context_Node* tail;
	CRITICAL_SECTION cs;
	CONDITION_VARIABLE cv_start;
	CONDITION_VARIABLE cv_done;
	BOOL shutdown;
} Work_Context_Controller;

// Generic filter function type
typedef void (*Filter_Function)(Work_Item* work);

static DWORD _stdcall thread_proc(void* params) {
	Filter_Engine* engine = (Filter_Engine*)params;
	Work_Context_Controller* controller = &engine->wc_controller;
	BOOLEAN stay = false;
	while (true) {
		Work_Context_Node* node = NULL;
		EnterCriticalSection(&controller->cs);
		if (!stay) SleepConditionVariableCS(&controller->cv_start, &controller->cs, INFINITE);
		stay = false;
		if (controller->shutdown) {
			LeaveCriticalSection(&controller->cs);
			return 0;
		}
		node = controller->head;
		LeaveCriticalSection(&controller->cs);
		if (node != NULL) {
			Thread_Work_Context* context = &node->context;
			while (context->work_index < context->work_count) {
				uint32_t index = InterlockedIncrement(&context->work_index) - 1;
				if (index < context->work_count) {
					context->works[index].function(&context->works[index]);
					InterlockedIncrement(&context->work_done);
				}
			}
			if (InterlockedCompareExchange(&context->work_done, context->work_count, context->work_count) == context->work_count) {
				stay = true;
				work_context_node_dequeue(controller);
			}
		}
	}
}

// Arena management functions for work context nodes for better performance
static void work_context_node_arena_initialize(Work_Context_Node_Arena* arena, size_t size) {
	assert(size > 1);
	Work_Context_Node* arena_arr = (Work_Context_Node*)calloc(size, sizeof(Work_Context_Node));
	if (arena_arr == NULL) {
		fprintf(stderr, "Failed to allocate memory for context node arena\n");
		exit(EXIT_FAILURE);
	}
	arena->nodes = arena_arr;
	for (size_t i = 0; i < size - 1; ++i) {
		arena->free_list[i].next = &arena[i + 1];
	}
	arena->free_list[size - 1].next = NULL;
	arena->size = size;

	return;
}

static void work_context_node_arena_destroy(Work_Context_Node_Arena* arena) {
	free(arena->nodes);
	arena->nodes = NULL;
	arena->free_list = NULL;
	arena->size = 0;

	return;
}

static Work_Context_Node* work_context_node_arena_allocate(Work_Context_Node_Arena* arena) {
	if (arena->free_list == NULL) {
		assert(false);
		// TODO: Choose to expand the arena or fail 
	}
	Work_Context_Node* node = arena->free_list;
	arena->free_list = arena->free_list->next;
	node->next = NULL;
	return node;
}

static void work_context_node_arena_free(Work_Context_Node_Arena* arena, Work_Context_Node* node) {
	node->context.work_count = 0;
	node->context.work_done = 0;
	node->context.work_index = 0;
	node->context.works = NULL;
	Work_Context_Node* temp = arena->free_list;
	arena->free_list = node;
	arena->free_list->next = temp;
}

// Functions to manage the work context queue
static void work_context_node_enqueue(Work_Context_Controller* controller, Work_Context_Node* node) {
	EnterCriticalSection(&controller->cs);
	if (controller->tail != NULL) controller->tail->next = node;
	else controller->tail = node;
	if (controller->head == NULL) controller->head = node;
	controller->tail = node;
	WakeAllConditionVariable(&controller->cv_start);
	LeaveCriticalSection(&controller->cs);

	return;
}

static void work_context_node_dequeue(Work_Context_Controller* controller) {
	EnterCriticalSection(&controller->cs);
	Work_Context_Node* node = controller->head;
	if (node == NULL) return;
	controller->head = controller->head->next;
	if (controller->head == NULL) {
		controller->tail = NULL;
		WakeAllConditionVariable(&controller->cv_done);
	} else {
		WakeAllConditionVariable(&controller->cv_start);
	}
	LeaveCriticalSection(&controller->cs);
	work_context_node_arena_free(controller->arena, node);	
}

// Function to create a thread work context for processing an image
static Work_Context work_context_create(Image* input, unsigned char* output, Work_Type type) {
	Work_Context context = { 0 };
	Filter_Function function;
	function = get_filter_function(type);
	assert(function != NULL && "Check get_filter_function()");
	uint32_t count = input->height/ WORK_ITEM_ROWS;
	assert(count > 0) && "You forgot to add a suffiicent threshold";
	uint32_t remainder = input->height % WORK_ITEM_ROWS;
	context.work_count = count;
	context.work_index = 0;
	context.work_done = 0;
	context.works = (Work_Item*)calloc(context.work_count, sizeof(Work_Item));
	for (uint32_t i = 0; i < count; ++i) {
		context.works[i].image = input->data + (i * WORK_ITEM_ROWS * img->channels);
		context.works[i].output = output-data + (i * WORK_ITEM_ROWS * img->channels);
		context.works[i].width = input->width;
		context.works[i].height = WORK_ITEM_ROWS;
		context.works[i].function = function;
	}
	if (remainder > 0) context.works[count - 1].height += remainder;

	return context;
}

// Function to destroy a thread work context
static void work_context_destroy(Work_Context context) {
	free(context.works);
}

static void invert_color_work(Work_Item* work) {
	for (uint32_t i = 0; i < work->width * work->height; ++i) {
		uint32_t idx = i * 3; // Assuming RGB format, each pixel has 3 channels
		work->output[idx] = 255 - work->image[idx];
		work->output[idx + 1] = 255 - work->image[idx + 1];
		work->output[idx + 2] = 255 - work->image[idx + 2];
	}
}

static void grayscale_work(Work_Item* work) {
	for (uint32_t i = 0; i < work->pixel_count; ++i) {
		uint32_t idx = i * 3; // Assuming RGB format, each pixel has 3 channels
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
	for (uint32_t i = 0; i < work->pixel_count; ++i) {
		uint32_t idx = i * 3; // Assuming RGB format, each pixel has 3 channels
		unsigned char r = work->image[idx];
		unsigned char g = work->image[idx + 1];
		unsigned char b = work->image[idx + 2];
		work->output[idx] = (unsigned char)min(255, (int)(0.393f * r + 0.769f * g + 0.189f * b));
		work->output[idx + 1] = (unsigned char)min(255, (int)(0.349f * r + 0.686f * g + 0.168f * b));
		work->output[idx + 2] = (unsigned char)min(255, (int)(0.272f * r + 0.534f * g + 0.131f * b));
	}
}

// Function to get the appropriate filter function based on the work type
static inline Filter_Function get_filter_function(Work_Type type) {
	switch (type) {
		case GRAYSCALE: return grayscale_work;
		case INVERSE: return invert_color_work;
		case SEPIA: return sepia_work;
		default: return NULL;
	}
}

// Helper function for single step filters by Work_Type enum. Built to prevent code duplication.
static void general_filter_helper(Filter_Engine* engine, Image* input, Image* output, Work_Type type) {
	assert(input->channels == 3);
	if (input->height <= THRESHOLD) {
		Work_Item work = { input->data, output->data, input->width, input->height, get_filter_function(type) };
		work->function(&work);
		return;
	}
	Work_Context context = work_context_create(input, output->data, type);
	Work_Context_Node* node = work_context_node_arena_allocate(&engine->wc_controller.arena);
	node->context = context;
	work_context_node_enqueue(&engine->wc_controller, node);

	return;
}

//------------------------------------------------------API Functions------------------------------------------------------//


// Mandatory function to call before using the filter engine
Filter_Engine filter_engine_create() {
	Filter_Engine engine;
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	size_t thread_count = sys_info.dwNumberOfProcessors;
	engine.t_context.thread_count = thread_count;
	engine.t_context.threads = (HANDLE*)calloc(thread_count, sizeof(HANDLE));
	size_t arena_size = DEFAULT_ARENA_SIZE;
	work_context_node_arena_initialize(&engine.wc_controller.arena, arena_size);
	engine.wc_controller.head = NULL;
	engine.wc_controller.tail = NULL;
	engine.wc_controller.shutdown = FALSE;
	InitializeCriticalSection(&engine.wc_controller.cs);
	InitializeConditionVariable(&engine.wc_controller.cv_start);
	InitializeConditionVariable(&engine.wc_controller.cv_done);
	for (size_t i = 0; i < thread_count; ++i) {
		engine.t_context.threads[i] = CreateThread(NULL, 0, thread_proc, &engine, 0, NULL);
		if (engine.t_context.threads[i] == NULL) {
			fprintf(stderr, "Failed to create thread %zu\n", i);
			exit(EXIT_FAILURE);
		}
	}

	return engine;
}

void filter_engine_initialize(Filter_Engine* engine, size_t Arena_Size, size_t Thread_Count) {
	if (Arena_Size == DEFAULT) Arena_Size = DEFAULT_ARENA_SIZE;
	if (Thread_Count == DEFAULT) {
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);
		Thread_Count = sys_info.dwNumberOfProcessors;
	}
	engine->t_context.thread_count = Thread_Count;
	engine->t_context.threads = (HANDLE*)calloc(Thread_Count, sizeof(HANDLE));
	work_context_node_arena_initialize(&engine->wc_controller.arena, Arena_Size);
	engine->wc_controller.head = NULL;
	engine->wc_controller.tail = NULL;
	engine->wc_controller.shutdown = FALSE;
	InitializeCriticalSection(&engine->wc_controller.cs);
	InitializeConditionVariable(&engine->wc_controller.cv_start);
	InitializeConditionVariable(&engine->wc_controller.cv_done);
	for (size_t i = 0; i < Thread_Count; ++i) {
		engine->t_context.threads[i] = CreateThread(NULL, 0, thread_proc, engine, 0, NULL);
		if (engine->t_context.threads[i] == NULL) {
			fprintf(stderr, "Failed to create thread %zu\n", i);
			exit(EXIT_FAILURE);
		}
	}

	return;
}

void filter_engine_destroy(Filter_Engine* engine) {
	filter_engine_wait(engine);
	EnterCriticalSection(&engine->wc_controller.cs);
	engine->wc_controller.shutdown = TRUE;
	WakeAllConditionVariable(&engine->wc_controller.cv_start);
	LeaveCriticalSection(&engine->wc_controller.cs);
	for (size_t i = 0; i < engine->t_context.thread_count; ++i) {
		WaitForSingleObject(engine->t_context.threads[i], INFINITE);
		CloseHandle(engine->t_context.threads[i]);
	}
	free(engine->t_context.threads);
	work_context_node_arena_destroy(&engine->wc_controller.arena);
	DeleteCriticalSection(&engine->wc_controller.cs);
	memset(engine, 0, sizeof(Filter_Engine));

	return;
}

void filter_engine_wait(Filter_Engine* engine) {
	EnterCriticalSection(&engine->wc_controller.cs);
	if (engine->wc_controller.head != NULL) {
		SleepConditionVariableCS(&engine->wc_controller.cv_done, &engine->wc_controller.cs, INFINITE);
	}
	LeaveCriticalSection(&engine->wc_controller.cs);
}

// Function to invert the colors of an image
void filter_engine_invert(Filter_Engine engine, Image* input, Image* output) {
	general_filter_helper(&engine, input, output, INVERT);

	return;
}

// Function to convert an image to grayscale
void filter_engine_grayscale(Filter_Engine engine, Image* input, Image* output) {
	general_filter_helper(&engine, input, output, GRAYSCALE);

	return;
}

// Function to convert an image to sepia
void filter_engine_sepia(Filter_Engine engine, Image* input, Image* output) {
	general_filter_helper(&engine, input, output, SEPIA);

	return;
}
