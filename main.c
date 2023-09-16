#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

void errorCallback(int code, const char* description) {
    char standard[20];
    sprintf(standard, "Hello C %ld", __STDC_VERSION__);
    printf("");
}

int main(int argc, char **argv) {
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    glfwSetErrorCallback(errorCallback);

    char standard[20];
    sprintf(standard, "Hello C %ld", __STDC_VERSION__);

    window = glfwCreateWindow(640, 480, standard, NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}
