#include <stdlib.h>
// #include <GLFW/glfw3.h>
#include <stdio.h>
#include "render.h"
#include "backtrace.h"
#include "app.h"

// void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
//     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
//         glfwSetWindowShouldClose(window, GLFW_TRUE);
//     }
// }

int main(int argc, char **argv) {
    init_exceptions(true);

    // GLFWwindow* window;

    // if (!glfwInit())
    //     return -1;

    // glfwSetErrorCallback(errorCallback);

    // glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

    // window = glfwCreateWindow(640, 480, APP_NAME, NULL, NULL);
    // if (!window) {
    //     glfwTerminate();
    //     return -1;
    // }
    // glfwMakeContextCurrent(window);
    // glfwSetKeyCallback(window, onKey);

    struct RenderContext renderContext = rc_init();

    // while (!glfwWindowShouldClose(window)) {
    //     glfwSwapBuffers(window);
    //     glfwPollEvents();
    // }

    // glfwTerminate();
    rc_cleanup(&renderContext);

    printf("Application control ending\n");
}

