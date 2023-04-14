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
#include <random>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void DrawAllStationeryModels(std::vector<Model> &statModels, Shader &shader);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct MainModelState {
    glm::vec3 mmPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 mmRotation = glm::vec3(1.0f, .0f, .0f);
    float mmSpeed = 0.01f;
    float mmUp = 0.0f;
    float mmUpSens = 3.5f;
    float mmAngle = -90.f;
    float mmTurnAngle = 0.f;

    MainModelState() = default;
};

struct ProgramState {
    // camera options
    Camera *camera = nullptr;
    bool firstMouse = true;
    float lastX = SCR_WIDTH / 2.0f;
    float lastY = SCR_HEIGHT / 2.0f;

    ProgramState() = default;
};
ProgramState *programState;
MainModelState *mainModelState;
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

    // default ProgramState setup
    programState = new ProgramState;
    mainModelState = new MainModelState;
    fps_camera = new FPSCamera(glm::vec3(.5f, .8f, -3.0f), glm::vec3(0.0f, 1.0f, 0.0f), 90.f);
    tpp_camera = new TPPCamera(glm::vec3(.5f, .8f, 3.0f), mainModelState->mmPosition,
                               glm::vec3(0.0f, 1.0f, 0.0f), -90.f, 40.f);
    programState->camera = tpp_camera;


    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Random number generator
    std::default_random_engine dre;
    double lowerBound = -25;
    double upperBound = 25;
    std::uniform_real_distribution<double> uniformDouble(lowerBound,upperBound);

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);

    // build and compile shaders
    Shader axisShader("resources/shaders/axisshader.vs", "resources/shaders/axisshader.fs");
    Shader airBalloonShader("resources/shaders/airballoonshader.vs", "resources/shaders/airballoonshader.fs");
    Shader grassPlaneShader("resources/shaders/grassplaneshader.vs", "resources/shaders/grassplaneshader.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    // models:
    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model)
    stbi_set_flip_vertically_on_load(false);
    // stationery models
    Model tree_house("resources/objects/tree_house/10783_TreeHouse_v7_LOD3.obj");
    Model pisa_tower("resources/objects/pisa_tower/10076_pisa_tower_v1_max2009_it0.obj");
    Model big_ben("resources/objects/big_ben/10059_big_ben_v2_max2011_it1.obj");
    Model christ_redeemer("resources/objects/christ_redeemer/12331_Christ_Rio_V1_L1.obj");
    stbi_set_flip_vertically_on_load(true);
    Model liberty_statue("resources/objects/liberty_statue/LibertStatue.obj");
    Model tree("resources/objects/tree/Tree.obj");
    // main model
    Model hot_air_balloon("resources/objects/hot_air_balloon/11809_Hot_air_balloon_l2.obj");

    std::vector<Model> stationery_models = {
            tree_house, pisa_tower, big_ben, christ_redeemer, liberty_statue, tree
    };

    // simple models:
    // axis
    std::vector<float> axisVertices = {
            // line
            -4.f, 0.f, 0.f,
            4.f, 0.f, 0.f,
            // arrow
            4.0, 0.0f, 0.0f,
            3.6, .2f, 0.0f,
            4.0, 0.0f, 0.0f,
            3.6, -.2f, 0.0f,
    };
    std::vector<glm::vec3> axisColor = {
            glm::vec3(1.f, 0.f, 0.f),
            glm::vec3(0.f, 1.f, 0.f),
            glm::vec3(0.f, 0.f, 1.f)
    };
    SimpleModel axisSModel(axisVertices);

    // grass plane
    std::vector<float> grass_plane_vertices = {
            // positions            // normals         // texcoords
            30.0f, 0.0f,  30.0f,  0.0f, 1.0f, 0.0f,  20.0f,  0.0f,
            -30.0f, 0.0f,  30.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -30.0f, 0.0f, -30.0f,  0.0f, 1.0f, 0.0f,   0.0f, 20.0f,

            30.0f, 0.0f,  30.0f,  0.0f, 1.0f, 0.0f,  20.0f,  0.0f,
            -30.0f, 0.0f, -30.0f,  0.0f, 1.0f, 0.0f,   0.0f, 20.0f,
            30.0f, 0.0f, -30.0f,  0.0f, 1.0f, 0.0f,  20.0f, 20.0f
    };
    SimpleModel grassPlaneSModel(grass_plane_vertices, true, true);
    grassPlaneSModel.AddTexture("resources/textures/plane.jpg", "texture1", 0, grassPlaneShader);
    // grass
    std::vector<float> grass_vertices = {
            1.f, 1.f, 0.0f, 1.0f, 1.0f,
            1.f, 0.f, 0.0f, 1.0f, 0.0f,
            0.f, 1.f, 0.0f, 0.0f, 1.0f,

            1.f, 0.f, 0.0f, 1.0f, 0.0f,
            0.f, 0.f, 0.0f, 0.0f, 0.0f,
            0.f, 1.f, 0.0f, 0.0f, 1.0f
    };
    SimpleModel grassSModel(grass_vertices, false, true);
    grassSModel.AddTexture("resources/textures/grass.png", "texture1", 0, grassPlaneShader);
    std::vector<glm::vec3> grass_translate;
    for(int i=0; i<1000; i++)
    {
        grass_translate.emplace_back(uniformDouble(dre), 0.f, uniformDouble(dre));
    }

    // skybox
    std::vector<float> skybox_vertices = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };
    vector<std::string> faces{
                    FileSystem::getPath("resources/textures/skybox/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox/back.jpg")
    };
    SimpleModel skyboxSModel(skybox_vertices);
    skyboxSModel.AddCubemaps(faces, "skybox", 0, skyboxShader);

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

        // view and model
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera->GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        // drawing grass plane model
        glEnable(GL_CULL_FACE);
        grassPlaneShader.use();
        grassPlaneShader.setMat4("projection", projection);
        grassPlaneShader.setMat4("view", view);
        grassPlaneShader.setMat4("model", model);
        grassPlaneSModel.Draw(GL_TRIANGLES);
        glDisable(GL_CULL_FACE);
        for(int i=0; i<grass_translate.size(); ++i)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, grass_translate[i]);
            if(i<grass_translate.size()/2)
                model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
            grassPlaneShader.setMat4("model", model);
            grassSModel.Draw(GL_TRIANGLES);
        }

        // drawing axis
        model = glm::mat4(1.0f);
        axisShader.use();
        axisShader.setMat4("projection", projection);
        axisShader.setMat4("view", view);
        for(int i=0; i<3; ++i)
        {
            model = glm::rotate(model, glm::radians(90.f), axisColor[i]);
            axisShader.setMat4("model", model);
            axisShader.setVec3("LineColor", axisColor[i]);
            axisSModel.Draw(GL_LINES);
        }

        // drawing balloon model
        model = glm::mat4(1.0f);
        airBalloonShader.use();
        airBalloonShader.setMat4("projection", projection);
        airBalloonShader.setMat4("view", view);
        model = glm::translate(model, mainModelState->mmPosition);
        model = glm::rotate(model, glm::radians(mainModelState->mmAngle), mainModelState->mmRotation);
        model = glm::rotate(model, glm::radians(mainModelState->mmTurnAngle), glm::vec3(0.f, 0.f, 1.f));
        model = glm::scale(model, glm::vec3(.0009f, .0009f, 0.0007f));
        airBalloonShader.setMat4("model", model);
        hot_air_balloon.Draw(airBalloonShader);
        mainModelState->mmAngle += mainModelState->mmAngle >=-90.f ? -0.04 : 0.04;
        mainModelState->mmRotation.z += mainModelState->mmRotation.z >= 0.f ? -0.001 : 0.001;
        //  drawing other static models
        DrawAllStationeryModels(stationery_models, airBalloonShader);

        // drawing skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera->GetViewMatrix()));
        skyboxShader.setMat4("projection", projection);
        skyboxShader.setMat4("view", view);
        skyboxSModel.Draw(GL_TRIANGLES, true);
        glDepthFunc(GL_LESS);

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
    delete programState;
    delete mainModelState;
    // if we put content of Destroy() method into ~SimpleModel destructor, glfwTerminate() causes SEGFAULT
    // probably glfwTerminate() is freeing by itself those VAOs and VBOs
    axisSModel.Destroy();
    grassPlaneSModel.Destroy();
    grassSModel.Destroy();
    skyboxSModel.Destroy();
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        if(mainModelState->mmAngle > -100.f)
        {
            mainModelState->mmAngle -= 0.1f;
        }
        if(mainModelState->mmTurnAngle > 0.f)
        {
            mainModelState->mmTurnAngle -= .5f;
        }
        if(mainModelState->mmTurnAngle < 0.f)
        {
            mainModelState->mmTurnAngle += .5f;
        }
        mainModelState->mmPosition.z += mainModelState->mmSpeed;
        programState->camera->updateCameraVectors(mainModelState->mmPosition);
        programState->camera->ProcessKeyboard(FORWARD, deltaTime);
        std::cout << "x " << mainModelState->mmPosition.x << "y " << mainModelState->mmPosition.y << "z " << mainModelState->mmPosition.z << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        if(mainModelState->mmAngle < -75.f)
        {
            mainModelState->mmAngle += 0.1f;
        }
        mainModelState->mmPosition.z -= mainModelState->mmSpeed;
        programState->camera->updateCameraVectors(mainModelState->mmPosition);

        programState->camera->ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        if(mainModelState->mmRotation.z > -0.3f)
        {
            mainModelState->mmRotation.z -= 0.002f;
            if(mainModelState->mmTurnAngle < 90.f)
            {
                mainModelState->mmTurnAngle += .5f;
            }
        }
        mainModelState->mmPosition.x += mainModelState->mmSpeed;
        programState->camera->updateCameraVectors(mainModelState->mmPosition);

        programState->camera->ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        if(mainModelState->mmRotation.z < 0.3f)
        {
            mainModelState->mmRotation.z += 0.002f;
            if(mainModelState->mmTurnAngle > -90.f)
            {
                mainModelState->mmTurnAngle -= .5f;
            }
        }
        mainModelState->mmPosition.x -= mainModelState->mmSpeed;
        programState->camera->updateCameraVectors(mainModelState->mmPosition);

        programState->camera->ProcessKeyboard(RIGHT, deltaTime);
    }

    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        mainModelState->mmUp += 0.01f;
        mainModelState->mmPosition.y = glm::log(mainModelState->mmUp+1.0f) * mainModelState->mmUpSens;
        programState->camera->updateCameraVectors(mainModelState->mmPosition);
    }

    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        mainModelState->mmUp = mainModelState->mmUp>=0 ? mainModelState->mmUp-0.01f : 0.0f;
        if(mainModelState->mmUp >= 0.f)
            mainModelState->mmPosition.y = glm::log(mainModelState->mmUp+1.0f) * mainModelState->mmUpSens;
        programState->camera->updateCameraVectors(mainModelState->mmPosition);
    }
//    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
//    {
//        programState->camera = fps_camera;
//        programState->camera->updateCameraVectors(mainModelState->mmPosition);
//    }
//    if(glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
//    {
//        programState->camera = tpp_camera;
//        programState->camera->updateCameraVectors(mainModelState->mmPosition);
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
        programState->camera->updateCameraVectors(mainModelState->mmPosition);
//    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera->ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
}

void DrawAllStationeryModels(std::vector<Model> &statModels, Shader &shader)
{
    // 0:tree_house, 1:pisa_tower, 2:big_ben, 3:christ_redeemer, 4:liberty_statue
    glm::mat4 model = glm::mat4(1.0f);
    // tree_house
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.f, 0.f, 3.f));
    model = glm::rotate(model, glm::radians(-90.f), glm::vec3(1.0f, .0f, .0f));
    model = glm::scale(model, glm::vec3(.015f, .015f, 0.015f));
    shader.setMat4("model", model);
    statModels[0].Draw(shader);
    // pisa_tower
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(15.f, 0.f, 10.f));
    model = glm::rotate(model, glm::radians(-90.f), glm::vec3(1.0f, .0f, .0f));
    model = glm::scale(model, glm::vec3(.0015f, .0015f, 0.0015f));
    shader.setMat4("model", model);
    statModels[1].Draw(shader);
    // big_ben
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-20.f, 0.f, -5.f));
    model = glm::rotate(model, glm::radians(-90.f), glm::vec3(1.0f, .0f, .0f));
    model = glm::scale(model, glm::vec3(.0025f, .0025f, 0.0025f));
    shader.setMat4("model", model);
    statModels[2].Draw(shader);
    // christ_redeemer
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.f, 0.f, 15.f));
    model = glm::rotate(model, glm::radians(-90.f), glm::vec3(1.0f, .0f, .0f));
    model = glm::rotate(model, glm::radians(-90.f), glm::vec3(.0f, 0.f, 1.0f));
    model = glm::scale(model, glm::vec3(.001f, .001f, 0.001f));
    shader.setMat4("model", model);
    statModels[3].Draw(shader);
    // liberty_statue
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(5.f, 0.f, -15.f));
    model = glm::scale(model, glm::vec3(15.f, 15.f, 15.f));
    shader.setMat4("model", model);
    statModels[4].Draw(shader);
    // tree
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.5f, 0.f, 4.f));
    model = glm::scale(model, glm::vec3(0.9f, 0.9f, 0.9f));
    shader.setMat4("model", model);
    statModels[5].Draw(shader);
}
