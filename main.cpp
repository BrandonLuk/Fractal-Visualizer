/*
* Graphic visualizer of various fractal sets.
* 
* Uses GLFW and OpenGL.
* 
* Requires AVX2 capable CPU.
* 
* Brandon Luk 2021
* 
* Fractals:
*   Mandelbrot
*   Julia
*   Burning Ship
* 
* Color schemes:
*   Simple(fast, but noisy with more detail)
*   Histogram(slow, scales with detail better)
* 
* CONTROLS:
*   W, A, S, D - Pan up, left, down, and right
*   Q, E - Zoom out and in, respectively
*   R - Reset fractal parameters(zoom, pan) to default 
*   F - Switch between fractal sets
*   I - Switch between standard and AVX instruction sets on applicable fractals
*   C - Switch between color sets
*   -, = - Decrease and increase fractal iteration limits, respectively
* 
*   Mouse scrollwheel can be used to zoom in/out while following the mouse cursor.
*/


#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl2.h"

#include <immintrin.h> // AVX instruction set

#include "color.h"
#include "fractal.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

unsigned int WINDOW_WIDTH = 1920;
unsigned int WINDOW_HEIGHT = 1080;

////////////////////////////////////////////////////////////
/// OpenGL resources
////////////////////////////////////////////////////////////

GLuint gl_textureId;
GLuint gl_pbo;
GLubyte* gl_textureBufData;

////////////////////////////////////////////////////////////
/// Fratal resources
////////////////////////////////////////////////////////////

Fractal fractal;
ColorGenerator cg;

bool update_fractal = false; // Keeps track of when the fractal has changed, so that we dont render the same fractal multiple times
bool use_AVX = true;


////////////////////////////////////////////////////////////
/// GLFW callbacks
////////////////////////////////////////////////////////////

/*
* Mouse scrollwheel forward -> zoom in, backward -> zoom out.
*/
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    update_fractal = true;

    double cursor_x_pos_from_top_left, cursor_y_pos_from_top_left;
    glfwGetCursorPos(window, &cursor_x_pos_from_top_left, &cursor_y_pos_from_top_left);

    fractal.followingZoom((int)yoffset, (int)cursor_x_pos_from_top_left, (int)cursor_y_pos_from_top_left, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Panning
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
        fractal.panUp();
    else if (key == GLFW_KEY_A && action == GLFW_PRESS)
        fractal.panLeft();
    else if (key == GLFW_KEY_S && action == GLFW_PRESS)
        fractal.panDown();
    else if (key == GLFW_KEY_D && action == GLFW_PRESS)
        fractal.panRight();

    // Zooming
    else if (key == GLFW_KEY_E && action == GLFW_PRESS)
        fractal.stationaryZoom(1, WINDOW_WIDTH, WINDOW_HEIGHT);

    else if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        fractal.stationaryZoom(-1, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Reset
    else if (key == GLFW_KEY_R && action == GLFW_PRESS)
        fractal.reset();

    // Switch between fractal sets
    else if (key == GLFW_KEY_F && action == GLFW_PRESS)
        fractal.selectNextFractal();

    // Switch from standard instructions to AVX2
    else if (key == GLFW_KEY_I && action == GLFW_PRESS)
        use_AVX = !use_AVX;

    // Change color generation color_mode
    else if (key == GLFW_KEY_C && action == GLFW_PRESS)
        cg.switchMode();

    // Iteration control
    else if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
        fractal.increaseIterations();

    else if (key == GLFW_KEY_MINUS && action == GLFW_PRESS)
        fractal.decreaseIterations();

    // The callback will be called even if a key we dont care about was pressed. In that case just return without changing the update flag.
    else
        return;


    update_fractal = true;
}


/*
* Generate our OpenGL texture and PBO.
*/
void initGL()
{
    glShadeModel(GL_FLAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);

    gl_textureBufData = new GLubyte[(size_t)WINDOW_WIDTH * WINDOW_HEIGHT * 4];

    glGenTextures(1, &gl_textureId);
    glBindTexture(GL_TEXTURE_2D, gl_textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)gl_textureBufData);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(1, &gl_pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, (size_t)WINDOW_HEIGHT * WINDOW_WIDTH * 4, 0, GL_STREAM_DRAW);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

/*
* Initialize the Dear ImGUI menu
*/
void initGUI(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();
}

/*
* Generate a new fractal if needed, then render the fractal as a textured fullscreen quad in OpenGL.
*/
void renderFractal()
{
    if (update_fractal)
    {
        update_fractal = false;
        
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_pbo);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, 4 * WINDOW_HEIGHT * WINDOW_WIDTH, 0, GL_STREAM_DRAW);

        int* ptr = (int*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        if (ptr)
        {
            fractal.generate(ptr, WINDOW_WIDTH, WINDOW_HEIGHT, cg, use_AVX);

            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        }


        glBindTexture(GL_TEXTURE_2D, gl_textureId);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_pbo);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw fullscreen quad with texture on it
    glBindTexture(GL_TEXTURE_2D, gl_textureId);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
}

/*
* 
*/
void renderGUI()
{
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Menu");

    // AVX color_mode checkbox
    if (ImGui::Checkbox("AVX", &use_AVX))
    {
        update_fractal = true;
    }

    // Fractal selection combo box
    int fractal_combo_current = static_cast<int>(fractal.fractal_mode);
    if (ImGui::Combo("Fractal", &fractal_combo_current, "Mandelbrot\0Julia\0Burning Ship\0\0"))
    {
        fractal.selectFractal(fractal_combo_current);
        update_fractal = true;
    }

    // Reset button
    if (ImGui::Button("Reset"))
    {
        fractal.reset();
        update_fractal = true;
    }

    // Zoom out/in buttons
    ImGui::Text("Zoom: ");
    ImGui::SameLine();
    if (ImGui::Button("-### zoom out"))
    {
        fractal.stationaryZoom(-1, WINDOW_WIDTH, WINDOW_HEIGHT);
        update_fractal = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("+### zoom in"))
    {
        fractal.stationaryZoom(1, WINDOW_WIDTH, WINDOW_HEIGHT);
        update_fractal = true;
    }

    // Iterations
    if (fractal.fractal_mode == Fractal::FractalSets::MANDELBROT)
    {
        ImGui::Text("Iterations: %u", fractal.mandelbrot_max_iter);
    }
    else if (fractal.fractal_mode == Fractal::FractalSets::JULIA)
    {
        ImGui::Text("Iterations: %u", fractal.julia_max_iter);
    }
    else
    {
        ImGui::Text("Iterations: %u", fractal.bship_max_iter);
    }
    ImGui::SameLine();
    if (ImGui::Button("-### decrease iterations"))
    {
        fractal.decreaseIterations();
        update_fractal = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("+### increase iterations"))
    {
        fractal.increaseIterations();
        update_fractal = true;
    }

    // Color selection
    ImGui::Text("Color options:");

    int color_combo_current = static_cast<int>(cg.color_mode);
    if (ImGui::Combo("Color generator", &color_combo_current, "Simple\0Histogram\0\0"))
    {
        cg.selectMode(color_combo_current);
        update_fractal = true;
    }
    if (static_cast<ColorGenerator::Generators>(color_combo_current) == ColorGenerator::Generators::SIMPLE)
    {
        ImGui::Text("RGB modifiers:");
        if (ImGui::SliderFloat("Red", &cg.simple_red_modifier, -3.141f, 3.141f, "%.3f", ImGuiSliderFlags_None)) update_fractal = true;
        if (ImGui::SliderFloat("Green", &cg.simple_green_modifier, -3.141f, 3.141f, "%.3f", ImGuiSliderFlags_None)) update_fractal = true;
        if (ImGui::SliderFloat("Blue", &cg.simple_blue_modifier, -3.141f, 3.141f, "%.3f", ImGuiSliderFlags_None)) update_fractal = true;
    }
    else if (static_cast<ColorGenerator::Generators>(color_combo_current) == ColorGenerator::Generators::HISTOGRAM)
    {
        ImVec4 strong_color = ImVec4((float)cg.strong.r / 255.0f, (float)cg.strong.g / 255.0f, (float)cg.strong.b / 255.0f, 0.0f);
        ImVec4 weak_color = ImVec4((float)cg.weak.r / 255.0f, (float)cg.weak.g / 255.0f, (float)cg.weak.b / 255.0f, 1.0f);

        if (ImGui::ColorEdit3("Strong", (float*)&strong_color, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoOptions))
        {
            cg.strong.r = strong_color.x * 255.0f;
            cg.strong.g = strong_color.y * 255.0f;
            cg.strong.b = strong_color.z * 255.0f;
            update_fractal = true;
        }
        if (ImGui::ColorEdit3("Weak", (float*)&weak_color, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoOptions))
        {
            cg.weak.r = weak_color.x * 255.0f;
            cg.weak.g = weak_color.y * 255.0f;
            cg.weak.b = weak_color.z * 255.0f;
            update_fractal = true;
        }
    }

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed color_mode window and its OpenGL context */
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Fractal Visualizer - Brandon L.", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glewInit();

    initGL();
    initGUI(window);

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    update_fractal = true;

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        // Poll for input events
        glfwPollEvents();

        // Render the fractal and GUI
        renderFractal();
        renderGUI();

        // Swap GL buffers
        glfwSwapBuffers(window);

        // Go to sleep and wait for some input
        glfwWaitEvents();
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteBuffers(1, &gl_pbo);
    delete gl_textureBufData;
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}