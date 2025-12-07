#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "filter.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F // We use this magic number because Microsoft's GL.h does not have it. It is ancient.
#endif

struct AppState {
    GLuint textureID;
    Image image;
    bool imageLoaded;
};

// Updates image data in existing OpenGL texture
void UpdateTexture(GLuint textureID, Image* image) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    if (image->channels == 3) glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, GL_RGB, GL_UNSIGNED_BYTE, image->data);
    else if (image->channels == 4) glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->width, image->height, GL_RGBA, GL_UNSIGNED_BYTE, image->data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Load initial image from disk
void LoadImageToTexture(AppState* appState, Image* image) {
    appState->image.width = image->width;
    appState->image.height = image->height;
    appState->image.channels = image->channels;
    appState->imageLoaded = true;

    // Frees the old image
    if (appState->image.data) free(appState->image.data);
    appState->image.data = image->data;

    // Destroys the old OpenGL Texture
    if (appState->textureID != 0) glDeleteTextures(1, &appState->textureID);

    // Creates the new OpengGL Texture
    glGenTextures(1, &appState->textureID);
    glBindTexture(GL_TEXTURE_2D, appState->textureID);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload pixels
    if (appState->image.channels == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, appState->image.width, appState->image.height, 0, GL_RGB, GL_UNSIGNED_BYTE, appState->image.data);
	else if (appState->image.channels == 4) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, appState->image.width, appState->image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, appState->image.data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return;
}

// Wrapper around filter functions
void ApplyFilter(Filter_Engine engine, AppState* appState, Work_Type type) {
    if (!appState->imageLoaded) return;

	// Input Image Initialization
    Image inputImg;
    inputImg.data = appState->image.data;
    inputImg.width = appState->image.width;
    inputImg.height = appState->image.height;
    inputImg.channels = appState->image.channels;

	// Output Image Intýalization
    Image outputImg;
    outputImg.width = inputImg.width;
    outputImg.height = inputImg.height;
    outputImg.channels = inputImg.channels;
    outputImg.data = (unsigned char*)calloc(inputImg.width * inputImg.height, 4);

    switch (type) {
    case GRAYSCALE:     filter_engine_grayscale(engine, &inputImg, &outputImg); break;
    case INVERT:        filter_engine_invert(engine, &inputImg, &outputImg); break;
    case SEPIA:         filter_engine_sepia(engine, &inputImg, &outputImg); break;

    default: break;
    }

    // TODO: Do a non UI blocking wait
    filter_engine_wait(engine);

	free(appState->image.data);      // Simple free is okay because stbi_image_free is just free internally
    appState->image.data = outputImg.data; 

    // Update OpenGL Texture so we see the result
    UpdateTexture(appState->textureID, &appState->image);
}


int main(void) {
	// Window Setup
    if (!glfwInit()) return 1;
    const char* glsl_version = "#version 130";
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Filter Engine", NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Restrict the frame rate to monitor refresh rate

    // ImGui Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize Filter Engine
    Filter_Engine engine = filter_engine_create();
    filter_engine_initialize(engine, DEFAULT, DEFAULT);

    // App State
    AppState appState = { 0 };
    char loadPath[256] = "";
    char savePath[256] = "";

    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Main Window", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

        ImGui::Columns(2, "layout_cols", false);
        ImGui::SetColumnWidth(0, 300); 

        ImGui::Text("File Operations");
        ImGui::Separator();

        Image image;
        ImGui::InputText("Path", loadPath, IM_ARRAYSIZE(loadPath));
        if (ImGui::Button("Load Image", ImVec2(-1, 0))) {
            image.data = stbi_load(loadPath, (int*)&image.width, (int*)&image.height, (int*)&image.channels, 0);
            if (!image.data) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to load image.");
            }
            LoadImageToTexture(&appState, &image);
        }

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Text("Filters");
        ImGui::Separator();

        // Only enable buttons if image is loaded
        if (!appState.imageLoaded) ImGui::BeginDisabled();

        if (ImGui::Button("Grayscale", ImVec2(-1, 0))) {
            ApplyFilter(engine, &appState, GRAYSCALE);
        }
        if (ImGui::Button("Invert Colors", ImVec2(-1, 0))) {
            ApplyFilter(engine, &appState, INVERT);
        }
        if (ImGui::Button("Sepia Tone", ImVec2(-1, 0))) {
            ApplyFilter(engine, &appState, SEPIA);
        }

        if (!appState.imageLoaded) ImGui::EndDisabled();

        ImGui::Dummy(ImVec2(0, 20));
        ImGui::Text("Export");
        ImGui::Separator();
        ImGui::InputText("Save As", savePath, IM_ARRAYSIZE(savePath));

        if (!appState.imageLoaded) ImGui::BeginDisabled();
        if (ImGui::Button("Save to Disk", ImVec2(-1, 0))) {
            
        }
        if (!appState.imageLoaded) ImGui::EndDisabled();

        ImGui::NextColumn();

        if (appState.imageLoaded) {
            // Calculate aspect ratio to fit image in window
            float availW = ImGui::GetContentRegionAvail().x;
            float availH = ImGui::GetContentRegionAvail().y;
            float scale = 1.0f;

            float imgRatio = (float)appState.image.width / (float)appState.image.height;
            float winRatio = availW / availH;

            if (imgRatio > winRatio) {
                scale = availW / (float)appState.image.width;
            }
            else {
                scale = availH / (float)appState.image.height;
            }

            ImGui::Image((void*)(intptr_t)appState.textureID,
                ImVec2(appState.image.width * scale, appState.image.height * scale));
        }
        else {
            ImGui::Text("No image loaded.");
        }

        ImGui::End(); 
        // End Frame

        // Render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    filter_engine_destroy(engine);
    if (appState.image.data) free(appState.image.data);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}