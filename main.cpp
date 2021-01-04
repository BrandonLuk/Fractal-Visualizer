/*
* Graphic visualizer of various fractal sets.
* 
* Uses GLFW and OpenGL.
* 
* Requires AVX2 capable CPU.
* 
* Brandon Luk 2020
* 
* Fractals:
*   Mandelbrot w/wo AVX
*   Julia
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
*   Mouse scrollwheen can be used to zoom in/out while following the mouse cursor
*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <immintrin.h> // AVX instruction set

#include "color.h"
#include "fractal.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

unsigned int WINDOW_WIDTH = 1280;
unsigned int WINDOW_HEIGHT = 720;

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

////////////////////////////////////////////////////////////
/// Timing resources
////////////////////////////////////////////////////////////

std::chrono::high_resolution_clock::time_point last;
std::chrono::high_resolution_clock::time_point now;
std::chrono::duration<double> time_delta;

////////////////////////////////////////////////////////////
/// Key press flags
////////////////////////////////////////////////////////////

bool w_key_pressed = false;
bool a_key_pressed = false;
bool s_key_pressed = false;
bool d_key_pressed = false;


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

    fractal.followingZoom(yoffset, cursor_x_pos_from_top_left, cursor_y_pos_from_top_left, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Panning
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
        w_key_pressed = true;
    else if (key == GLFW_KEY_W && action == GLFW_RELEASE)
        w_key_pressed = false;

    else if (key == GLFW_KEY_A && action == GLFW_PRESS)
        a_key_pressed = true;
    else if (key == GLFW_KEY_A && action == GLFW_RELEASE)
        a_key_pressed = false;

    else if (key == GLFW_KEY_S && action == GLFW_PRESS)
        s_key_pressed = true;
    else if (key == GLFW_KEY_S && action == GLFW_RELEASE)
        s_key_pressed = false;

    else if (key == GLFW_KEY_D && action == GLFW_PRESS)
        d_key_pressed = true;
    else if (key == GLFW_KEY_D && action == GLFW_RELEASE)
        d_key_pressed = false;

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
        fractal.switchFractal();

    // Switch from standard instructions to AVX2
    else if (key == GLFW_KEY_I && action == GLFW_PRESS)
        fractal.switchInstruction();

    // Change color generation mode
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, gl_textureBufData);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(1, &gl_pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, (size_t)WINDOW_HEIGHT * WINDOW_WIDTH * 4, 0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

/*
* Pan the window frame depending on which keys are pressed. Use the time passed(delta) as a dampener to smooth out the panning.
*/
void panWindowFrame(double delta)
{
    if (w_key_pressed)
        fractal.panUp(delta);
    if (a_key_pressed)
        fractal.panLeft(delta);
    if (s_key_pressed)
        fractal.panDown(delta);
    if (d_key_pressed)
        fractal.panRight(delta);
}

/*
* Generate the colored fractal and pass it to OpenGL which will draw it on a textured quad.
*/
void render()
{
    update_fractal = false;
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, (size_t)WINDOW_WIDTH * WINDOW_HEIGHT * 4, 0, GL_STREAM_DRAW);
    GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

    int* ptr_i = (int*)ptr;
    //START_TIMER
    fractal.generate(ptr_i, WINDOW_WIDTH, WINDOW_HEIGHT, cg);
    //END_TIMER
    
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    glBindTexture(GL_TEXTURE_2D, gl_textureId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT);


    // Draw fullscreen quad with texture on it
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Fractal Visualizer - Brandon Luk", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glewInit();

    initGL();

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    update_fractal = true;

    last = std::chrono::high_resolution_clock::now();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        
        if (update_fractal)
        {
            render();
            glfwSwapBuffers(window);
        }

        /* Poll for and process events */
        now = std::chrono::high_resolution_clock::now();
        time_delta = now - last;
        last = now;

        glfwPollEvents();
        panWindowFrame(time_delta.count());
    }

    glDeleteBuffers(1, &gl_pbo);
    delete gl_textureBufData;
    glfwTerminate();
    return 0;
}