#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include "rg/SimpleModel.h"
#include "rg/TPPCamera.h"
#include "rg/FPSCamera.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct ProgramState {
    // camera options
    Camera *camera;
    bool firstMouse = true;
    float lastX = SCR_WIDTH / 2.0f;
    float lastY = SCR_HEIGHT / 2.0f;

    // air_balloon
    glm::vec3 airBalloonPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    float airBalloonUp = 0.0f;
    float airBalloonAngle = -90.f;
    float TurnAngle = 0.f;
    glm::vec3 airBalloonRotation = glm::vec3(1.0f, .0f, .0f);

    ProgramState() = default;
    ~ProgramState() { delete camera; }

};
ProgramState *programState;
FPSCamera *fps_camera;
TPPCamera *tpp_camera;

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation                                                                              full screen
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL/*glfwGetPrimaryMonitor()*/, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // default ProgramState setup
    programState = new ProgramState;
    fps_camera = new FPSCamera(glm::vec3(.5f, .8f, -3.0f), glm::vec3(0.0f, 1.0f, 0.0f), 90.f);
    tpp_camera = new TPPCamera(glm::vec3(.5f, .8f, 3.0f), programState->airBalloonPosition,
                               glm::vec3(0.0f, 1.0f, 0.0f), -90.f, 40.f);
    programState->camera = fps_camera;


    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);

    // build and compile shaders
    Shader axisShader("resources/shaders/axisshader.vs", "resources/shaders/axisshader.fs");
    Shader airBalloonShader("resources/shaders/airballoonshader.vs", "resources/shaders/airballoonshader.fs");
    Shader grassPlaneShader("resources/shaders/grassplaneshader.vs", "resources/shaders/grassplaneshader.fs");

    // models:
    // hot air balloon
    Model hot_air_balloon("resources/objects/hot_air_balloon/11809_Hot_air_balloon_l2.obj");

    // simple models:
    // axis
    std::vector<float> vertices = {
            // redline
            -4.f, 0.f, 0.f, 1.f, 0.f, 0.f,
            4.f, 0.f, 0.f, 1.f, 0.f, 0.f,
            // redarrow
            4.0, 0.0f, 0.0f, 1.f, 0.f, 0.f,
            3.6, .2f, 0.0f,1.f, 0.f, 0.f,
            4.0, 0.0f, 0.0f, 1.f, 0.f, 0.f,
            3.6, -.2f, 0.0f,1.f, 0.f, 0.f,
            // greenline
            .0f, -4.f, 0.f, .0f, 1.f, 0.f,
            .0f, 4.f, 0.f, .0f, 1.f, 0.f,
            // greenarrow
            0.0f, 4.0f, 0.0f, .0f, 1.f, 0.f,
            0.2f, 3.6f, 0.0f, .0f, 1.f, 0.f,
            0.0f, 4.0f, 0.0f, .0f, 1.f, 0.f,
            -0.2f, 3.6f, 0.0f, .0f, 1.f, 0.f,
            // blueline
            .0f, 0.f, -4.f, .0f, 0.f, 1.f,
            .0f, 0.f, 4.f, .0f, 0.f, 1.f,
            // bluearrow
            0.0f, 0.0f ,4.0f, .0f, 0.f, 1.f,
            0.0f, 0.2f ,3.6f, .0f, 0.f, 1.f,
            0.0f, 0.0f ,4.0f, .0f, 0.f, 1.f,
            0.0f, -0.2f ,3.6f, .0f, 0.f, 1.f,
    };
    SimpleModel axisSModel(vertices,true);

    // grass plane
    std::vector<float> grass_plane_vertices = {
            // positions            // normals         // texcoords
            10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  20.0f,  0.0f,
            -10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 20.0f,

            10.0f, 0.0f,  10.0f,  0.0f, 1.0f, 0.0f,  20.0f,  0.0f,
            -10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 20.0f,
            10.0f, 0.0f, -10.0f,  0.0f, 1.0f, 0.0f,  20.0f, 20.0f
    };
    SimpleModel grassSModel(grass_plane_vertices, true, true);
    grassSModel.AddTexture("resources/textures/plane.jpg", "texture1", 0, grassPlaneShader);

    // projection (most of the time there's no need to change projection, no need to be in the render loop)
    glm::mat4 projection = glm::perspective(glm::radians(programState->camera->Distance), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    axisShader.use();
    axisShader.setMat4("projection", projection);
    airBalloonShader.use();
    airBalloonShader.setMat4("projection", projection);
    grassPlaneShader.use();
    grassPlaneShader.setMat4("projection", projection);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // input
        processInput(window);
        // render
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        projection = glm::perspective(glm::radians(programState->camera->Distance), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        // drawing axis
        axisShader.use();
        axisShader.setMat4("projection", projection);
        airBalloonShader.use();
        airBalloonShader.setMat4("projection", projection);
        grassPlaneShader.use();
        grassPlaneShader.setMat4("projection", projection);
        glm::mat4 view = programState->camera->GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        // drawing grass plane model
        glEnable(GL_CULL_FACE);
        grassPlaneShader.use();
        grassPlaneShader.setMat4("view", view);
        grassSModel.Draw(GL_TRIANGLES);
        glDisable(GL_CULL_FACE);

        // drawing axis model
        axisShader.use();
        axisShader.setMat4("view", view);
        axisSModel.Draw(GL_LINES);

        // drawing balloon model
        airBalloonShader.use();
        airBalloonShader.setMat4("view", view);
        model = glm::translate(model, programState->airBalloonPosition);
        model = glm::rotate(model, glm::radians(programState->airBalloonAngle), programState->airBalloonRotation);
        model = glm::rotate(model, glm::radians(programState->TurnAngle), glm::vec3(0.f, 0.f, 1.f));
        model = glm::scale(model, glm::vec3(.0009f, .0009f, 0.0007f));
        airBalloonShader.setMat4("model", model);
        hot_air_balloon.Draw(airBalloonShader);
        programState->airBalloonAngle += programState->airBalloonAngle >=-90.f ? -0.04 : 0.04;
        programState->airBalloonRotation.z += programState->airBalloonRotation.z >= 0.f ? -0.001 : 0.001;

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    delete fps_camera;
    delete tpp_camera;
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        if(programState->airBalloonAngle > -100.f)
        {
            programState->airBalloonAngle -= 0.1f;
        }
        if(programState->TurnAngle > 0.f)
        {
            programState->TurnAngle -= .5f;
        }
        if(programState->TurnAngle < 0.f)
        {
            programState->TurnAngle += .5f;
        }
        programState->airBalloonPosition.z += 0.003f;
        programState->camera->updateCameraVectors(programState->airBalloonPosition);
        programState->camera->ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        if(programState->airBalloonAngle < -75.f)
        {
            programState->airBalloonAngle += 0.1f;
        }
        programState->airBalloonPosition.z -= 0.003f;
        programState->camera->updateCameraVectors(programState->airBalloonPosition);

        programState->camera->ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        if(programState->airBalloonRotation.z > -0.3f)
        {
            programState->airBalloonRotation.z -= 0.002f;
            if(programState->TurnAngle < 90.f)
            {
                programState->TurnAngle += .5f;
            }
        }
        programState->airBalloonPosition.x += 0.003f;
        programState->camera->updateCameraVectors(programState->airBalloonPosition);

        programState->camera->ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        if(programState->airBalloonRotation.z < 0.3f)
        {
            programState->airBalloonRotation.z += 0.002f;
            if(programState->TurnAngle > -90.f)
            {
                programState->TurnAngle -= .5f;
            }
        }
        programState->airBalloonPosition.x -= 0.003f;
        programState->camera->updateCameraVectors(programState->airBalloonPosition);

        programState->camera->ProcessKeyboard(RIGHT, deltaTime);
    }

    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        programState->airBalloonUp += 0.01f;
        programState->airBalloonPosition.y = glm::log(programState->airBalloonUp+1.0f)/3;
        programState->camera->updateCameraVectors(programState->airBalloonPosition);
    }

    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        programState->airBalloonUp = programState->airBalloonUp>=0 ? programState->airBalloonUp-0.01f : 0.0f;
        programState->airBalloonPosition.y = glm::log(programState->airBalloonUp+1.0f)/3.f;
        programState->camera->updateCameraVectors(programState->airBalloonPosition);
    }
//    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
//    {
//        programState->camera = fps_camera;
//        programState->camera->updateCameraVectors(programState->airBalloonPosition);
//    }
//    if(glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
//    {
//        programState->camera = tpp_camera;
//        programState->camera->updateCameraVectors(programState->airBalloonPosition);
//    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (programState->firstMouse) {
        programState->lastX = xpos;
        programState->lastY = ypos;
        programState->firstMouse = false;
    }

    float xoffset = xpos - programState->lastX;
    float yoffset = programState->lastY - ypos; // reversed since y-coordinates go from bottom to top
    programState->lastX = xpos;
    programState->lastY = ypos;
//    int leftClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
//    if (leftClick == GLFW_PRESS)
//    {
        programState->camera->ProcessMouseMovement(xoffset, yoffset);
        programState->camera->updateCameraVectors(programState->airBalloonPosition);
//    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera->ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
}

