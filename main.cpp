#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "color.h"
#include "fractal.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include <fstream>

#include <iomanip>


#define START_TIMER auto start = std::chrono::steady_clock::now();
#define END_TIMER   auto dur = std::chrono::steady_clock::now() - start; \
                    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() << std::endl;

unsigned int WINDOW_WIDTH = 1280;
unsigned int WINDOW_HEIGHT = 720;




GLuint gl_textureId;
GLuint gl_pbo;
GLubyte* gl_textureBufData;

Fractal fractal;
ColorGenerator cg;

bool update_fractal = false;

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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    update_fractal = true;
    if (yoffset > 0)
        fractal.zoom(yoffset);
    else
        fractal.zoom(yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    update_fractal = true;
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
}



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

void panWindowFrame()
{
    if (w_key_pressed)
        fractal.panUp();
    if (a_key_pressed)
        fractal.panLeft();
    if (s_key_pressed)
        fractal.panDown();
    if (d_key_pressed)
        fractal.panRight();

}

void render()
{
    update_fractal = false;
    

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, (size_t)WINDOW_WIDTH * WINDOW_HEIGHT * 4, 0, GL_STREAM_DRAW);
    GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    //glColor3f(1.0f, 1.0f, 1.0f);

    // ABGR
    int* ptr_i = (int*)ptr;
    fractal.coloredMandelbrotMatrix(ptr_i, WINDOW_WIDTH, WINDOW_HEIGHT, cg);
    

    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    glBindTexture(GL_TEXTURE_2D, gl_textureId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT);

    //glBindTexture(GL_TEXTURE_2D, gl_textureId);
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
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glewInit();

    initGL();

    //glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);


    /*glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
    glDisable(GL_DEPTH_TEST);*/

    update_fractal = true;

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        
        if (update_fractal)
        {
            render();
            glfwSwapBuffers(window);
        }

        /* Swap front and back buffers */
        

        /* Poll for and process events */
        glfwPollEvents();
        panWindowFrame();
    }

    glfwTerminate();
    return 0;
}