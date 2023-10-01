#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "render.h"
#include "util.h"
#include "backtrace.h"

void errorCallback(int code, const char* description) {
    char standard[20];
    sprintf(standard, "Hello C %ld\n", __STDC_VERSION__);
}

void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(int argc, char **argv) {
    init_exceptions(true);

    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    glfwSetErrorCallback(errorCallback);

    char standard[20];
    sprintf(standard, "Hello C %ld", __STDC_VERSION__);

    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

    window = glfwCreateWindow(640, 480, standard, NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, onKey);

    struct RenderContext renderContext = init_render();

    while (!glfwWindowShouldClose(window)) {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    free_render(&renderContext);
    glfwTerminate();
}

