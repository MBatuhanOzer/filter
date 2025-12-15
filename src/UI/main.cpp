#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <Windows.h>
#include <commdlg.h> // For common dialog boxes

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "filter.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F // We use this magic number because Microsoft's GL.h does not have it. It is ancient.
#endif

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h> // For hwndOwner for File Dialog

struct AppState {
    GLuint textureID;
    Image image;
    bool imageLoaded;
};

// Opens a Windows file dialog to select an image file
std::string OpenFileDialog(GLFWwindow* window) {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(window); // To prevent using main window before selecting file
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return std::string(); 
}

std::string SaveFileDialog(GLFWwindow* window) {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(window);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);

    // Specific filters for the formats stb_image_write supports
    ofn.lpstrFilter = "PNG File (*.png)\0*.png\0JPG File (*.jpg)\0*.jpg\0BMP File (*.bmp)\0*.bmp\0TGA File (*.tga)\0*.tga\0";
    ofn.nFilterIndex = 1; 

    // Default extension
    ofn.lpstrDefExt = "png";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return std::string();
}

void SaveImageByExtension(const std::string& filepath, const Image& image) {
    size_t dotPos = filepath.find_last_of('.');
    std::string ext = "";
    if (dotPos != std::string::npos) {
        ext = filepath.substr(dotPos);
    }

    // I am being lazy for easier comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    int result = 0;
    if (ext == ".png") {
        result = stbi_write_png(filepath.c_str(), image.width, image.height, image.channels, image.data, image.width * image.channels);
    }
    else if (ext == ".jpg" || ext == ".jpeg") {
        result = stbi_write_jpg(filepath.c_str(), image.width, image.height, image.channels, image.data, 90);
    }
    else if (ext == ".bmp") {
        result = stbi_write_bmp(filepath.c_str(), image.width, image.height, image.channels, image.data);
    }
    else if (ext == ".tga") {
        result = stbi_write_tga(filepath.c_str(), image.width, image.height, image.channels, image.data);
    }
    else {
        // Fallback: If extension is unknown, save as PNG
        result = stbi_write_png(filepath.c_str(), image.width, image.height, image.channels, image.data, image.width * image.channels);
    }
}

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
    outputImg.data = (unsigned char*)calloc(inputImg.width * inputImg.height, inputImg.channels);

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

        // I want them to persist between frames
        static std::string statusMessage = "";
        static ImVec4 statusColor = ImVec4(1, 1, 1, 1);

        if (ImGui::Button("Open Image...", ImVec2(-1, 0))) {
            Image image = { 0 };
            std::string selectedPath = OpenFileDialog(window);

            if (!selectedPath.empty()) {
                image.data = stbi_load(selectedPath.c_str(), (int*)&image.width, (int*)&image.height, (int*)&image.channels, 0);

                if (!image.data) {
                    statusMessage = "Failed to load image.";
                    statusColor = ImVec4(1, 0, 0, 1); 
                }
                else {
                    LoadImageToTexture(&appState, &image);

                    statusMessage = "Loaded: " + selectedPath;
                    statusColor = ImVec4(0, 1, 0, 1); 
                }
            }
        }

        if (!statusMessage.empty()) {
            ImGui::TextColored(statusColor, "%s", statusMessage.c_str());
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

        if (!appState.imageLoaded) ImGui::BeginDisabled();
        if (ImGui::Button("Save to disk...", ImVec2(-1, 0))) {
            std::string savePath = SaveFileDialog(window);

            if (!savePath.empty()) {
                SaveImageByExtension(savePath, appState.image); 
            }
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