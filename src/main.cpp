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

void DrawSkybox(Shader &shader, const SimpleModel &skyboxModel, glm::mat4 projection);
void DrawGrassGround(Shader &shader, SimpleModel &grassPlane, SimpleModel &grass, std::vector<glm::vec3> &grassPos, glm::mat4 projection);
void DrawAllStationeryModels(std::vector<Model> &statModels, Shader &shader);
void DrawAxis(Shader &shader, const SimpleModel &axisSModel, const std::vector<glm::vec3> &axisColor, glm::mat4 projection);
void SetDirectionalLightParameters(Shader &shader);
void DrawImGuiInfoWindows();
void DrawCVarAndAxis(GLFWwindow *window, Shader &shader, const SimpleModel &axisSModel, const std::vector<glm::vec3> &axisColor, glm::mat4 projection);
void DrawAirBalloon(Shader &shader, Model &mm, glm::mat4 projection);
void AirBalloonIdleEvent(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct MainModelState {
    glm::vec3 mmPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 mmRotation = glm::vec3(1.0f, .0f, .0f);
    float mmSpeed = 0.5f;
    float mmUp = 0.0f;
    float mmUpSens = 4.5f;
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

    bool isCVars = false;

    // Directional Light is in this scenario Sun and its parameters should be the same for all objects on the scene
    glm::vec3 dirLight = glm::vec3(0.1f, -1.2f, 1.f);
    glm::vec3 dirAmbient = glm::vec3(0.54f, 0.54f, 0.5f);
    glm::vec3 dirDiffuse = glm::vec3(0.95f, 0.9f, 0.65f);
    glm::vec3 dirSpecular = glm::vec3(0.3f, 0.3f, 0.3f);

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
    tpp_camera = new TPPCamera(glm::vec3(.0f, .0f, 0.f), mainModelState->mmPosition,
                               glm::vec3(0.0f, 1.0f, 0.0f), -90.f, 40.f);
    programState->camera = tpp_camera;


    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void) io;
    ImGui::StyleColorsDark();
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
    Shader modelShader("resources/shaders/modelshader.vs", "resources/shaders/modelshader.fs");
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

    std::vector<Model> stationery_models
    {
            tree_house, pisa_tower, big_ben, christ_redeemer, liberty_statue, tree
    };

    // simple models:
    // axis
    std::vector<float> axisVertices
    {
            // line
            -4.f, 0.f, 0.f,
            4.f, 0.f, 0.f,
            // arrow
            4.0, 0.0f, 0.0f,
            3.6, .2f, 0.0f,
            4.0, 0.0f, 0.0f,
            3.6, -.2f, 0.0f,
    };
    std::vector<glm::vec3> axisColor
    {
            glm::vec3(1.f, 0.f, 0.f),
            glm::vec3(0.f, 1.f, 0.f),
            glm::vec3(0.f, 0.f, 1.f)
    };
    SimpleModel axisSModel(axisVertices);

    // grass plane
    std::vector<float> grass_plane_vertices
    {
        // positions          // normals         // texcoords
        30.0f, 0.0f,  30.0f,  0.0f, 1.0f, 0.0f,  20.0f,  0.0f,
        -30.0f, 0.0f,  30.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -30.0f, 0.0f, -30.0f,  0.0f, 1.0f, 0.0f,   0.0f, 20.0f,

        30.0f, 0.0f,  30.0f,  0.0f, 1.0f, 0.0f,  20.0f,  0.0f,
        -30.0f, 0.0f, -30.0f,  0.0f, 1.0f, 0.0f,   0.0f, 20.0f,
        30.0f, 0.0f, -30.0f,  0.0f, 1.0f, 0.0f,  20.0f, 20.0f
    };
    SimpleModel grassPlaneSModel(grass_plane_vertices, true, true);
    grassPlaneSModel.AddTexture("resources/textures/plane.jpg", "material.diffuse", 0, grassPlaneShader, GL_REPEAT);
    grassPlaneSModel.AddTexture("resources/textures/plane_specular.png", "material.specular", 1, grassPlaneShader, GL_REPEAT);
    grassPlaneSModel.AddTexture("resources/textures/plane_ambient.jpg", "material.ambient", 2, grassPlaneShader, GL_REPEAT);
    // grass
    std::vector<float> grass_vertices
    {
        1.f, 1.f, 0.0f, 1.0f, 1.0f,
        1.f, 0.f, 0.0f, 1.0f, 0.0f,
        0.f, 1.f, 0.0f, 0.0f, 1.0f,

        1.f, 0.f, 0.0f, 1.0f, 0.0f,
        0.f, 0.f, 0.0f, 0.0f, 0.0f,
        0.f, 1.f, 0.0f, 0.0f, 1.0f
    };
    SimpleModel grassSModel(grass_vertices, false, true);
    grassSModel.AddTexture("resources/textures/grass.png", "material.diffuse", 0, grassPlaneShader);
    grassSModel.AddTexture("resources/textures/grass.png", "material.ambient", 1, grassPlaneShader);
    std::vector<glm::vec3> grass_translate;
    for(int i=0; i<1000; i++)
    {
        grass_translate.emplace_back(uniformDouble(dre), 0.f, uniformDouble(dre));
    }

    // skybox
    std::vector<float> skybox_vertices
    {
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
    vector<std::string> faces
    {
        FileSystem::getPath("resources/textures/skybox/right.jpg"),
        FileSystem::getPath("resources/textures/skybox/left.jpg"),
        FileSystem::getPath("resources/textures/skybox/top.jpg"),
        FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
        FileSystem::getPath("resources/textures/skybox/front.jpg"),
        FileSystem::getPath("resources/textures/skybox/back.jpg")
    };
    SimpleModel skyboxSModel(skybox_vertices);
    skyboxSModel.AddCubemaps(faces, "skybox", 0, skyboxShader);

    // declare before loop
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model = glm::mat4(1.0f);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ImGui frame init
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // input
        processInput(window);
        // render
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // projection, view
        projection = glm::perspective(glm::radians(programState->camera->Zoom),
                                      (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        // drawing grass plane model
        DrawGrassGround(grassPlaneShader, grassPlaneSModel, grassSModel, grass_translate, projection);

        SetDirectionalLightParameters(modelShader);
        // drawing other static models
        DrawAllStationeryModels(stationery_models, modelShader);
        // drawing balloon model
        DrawAirBalloon(modelShader, hot_air_balloon, projection);
        AirBalloonIdleEvent(window);

        // drawing skybox
        DrawSkybox(skyboxShader, skyboxSModel, projection);

        // drawing ImGui windows
        DrawImGuiInfoWindows();
        DrawCVarAndAxis(window, axisShader, axisSModel, axisColor, projection);

        // ImGui render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    if(programState->isCVars && programState->camera == tpp_camera)
        return;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        if(programState->camera == fps_camera)
        {
            programState->camera->ProcessKeyboard(FORWARD, deltaTime);
        }
        else if(mainModelState->mmPosition.y >=0.5)
        {
            if (mainModelState->mmAngle > -100.f)
                mainModelState->mmAngle -= 0.1f;
            if (mainModelState->mmTurnAngle > 0.f)
                mainModelState->mmTurnAngle -= .5f;
            if (mainModelState->mmTurnAngle < 0.f)
                mainModelState->mmTurnAngle += .5f;
            mainModelState->mmPosition.z += mainModelState->mmSpeed * deltaTime;
            programState->camera->updateCameraVectors(mainModelState->mmPosition);
        }
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        if(programState->camera == fps_camera)
        {
            programState->camera->ProcessKeyboard(BACKWARD, deltaTime);
        }
        else if(mainModelState->mmPosition.y >=0.5)
        {
            if (mainModelState->mmAngle < -75.f)
                mainModelState->mmAngle += 0.1f;
            mainModelState->mmPosition.z -= mainModelState->mmSpeed * deltaTime;
            programState->camera->updateCameraVectors(mainModelState->mmPosition);
        }
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        if(programState->camera == fps_camera)
        {
            programState->camera->ProcessKeyboard(LEFT, deltaTime);
        }
        else if(mainModelState->mmPosition.y >=0.5)
        {
            if (mainModelState->mmRotation.z > -0.3f)
            {
                mainModelState->mmRotation.z -= 0.002f;
                if (mainModelState->mmTurnAngle < 90.f)
                    mainModelState->mmTurnAngle += .5f;
            }
            mainModelState->mmPosition.x += mainModelState->mmSpeed * deltaTime;
            programState->camera->updateCameraVectors(mainModelState->mmPosition);
        }
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        if(programState->camera == fps_camera)
        {
            programState->camera->ProcessKeyboard(RIGHT, deltaTime);
        }
        else if(mainModelState->mmPosition.y >=0.5)
        {
            if (mainModelState->mmRotation.z < 0.3f)
            {
                mainModelState->mmRotation.z += 0.002f;
                if (mainModelState->mmTurnAngle > -90.f)
                    mainModelState->mmTurnAngle -= .5f;
            }
            mainModelState->mmPosition.x -= mainModelState->mmSpeed * deltaTime;
            programState->camera->updateCameraVectors(mainModelState->mmPosition);
        }
    }

    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && programState->camera == tpp_camera)
    {
        mainModelState->mmUp += 0.01f;
        mainModelState->mmPosition.y = glm::log(mainModelState->mmUp+1.0f) * mainModelState->mmUpSens;
        programState->camera->updateCameraVectors(mainModelState->mmPosition);
    }

    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && programState->camera == tpp_camera)
    {
        mainModelState->mmUp = mainModelState->mmUp>=0 ? mainModelState->mmUp-0.01f : 0.0f;
        if(mainModelState->mmUp >= 0.f)
            mainModelState->mmPosition.y = glm::log(mainModelState->mmUp+1.0f) * mainModelState->mmUpSens;
        programState->camera->updateCameraVectors(mainModelState->mmPosition);
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if(programState->isCVars && programState->camera == tpp_camera)
        return;

    if (programState->firstMouse)
    {
        programState->lastX = xpos;
        programState->lastY = ypos;
        programState->firstMouse = false;
    }
    float xoffset = xpos - programState->lastX;
    float yoffset = programState->lastY - ypos; // reversed since y-coordinates go from bottom to top
    programState->lastX = xpos;
    programState->lastY = ypos;
    programState->camera->ProcessMouseMovement(xoffset, yoffset);
    programState->camera->updateCameraVectors(mainModelState->mmPosition);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if(programState->isCVars)
        return;

    programState->camera->ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // if the cvar window is opened, then just close it, but don't exit the game. If it is closed, then shut down the game
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        if(!programState->isCVars)
            glfwSetWindowShouldClose(window, true);
        programState->isCVars = false;
    }

    if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS)
        programState->isCVars = !programState->isCVars;
}

void DrawSkybox(Shader &shader, const SimpleModel &skyboxModel, glm::mat4 projection)
{
    glDepthFunc(GL_LEQUAL);
    shader.use();
    glm::mat4 view = glm::mat4(glm::mat3(programState->camera->GetViewMatrix()));
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    skyboxModel.Draw(GL_TRIANGLES, true);
    glDepthFunc(GL_LESS);
}

void DrawGrassGround(Shader &shader, SimpleModel &grassPlane, SimpleModel &grass, std::vector<glm::vec3> &grassPos, glm::mat4 projection)
{
    glm::mat4 view = programState->camera->GetViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);
    shader.use();

    // grass is using custom light parameters because it doesn't have any additional tex maps
    shader.setVec3("viewPos", programState->camera->Position);
    shader.setVec3("dirLight.direction", programState->dirLight);
    shader.setVec3("dirLight.ambient", 1.f, 1.f, 1.f);
    shader.setVec3("dirLight.diffuse", 1.f, 1.f, 1.f);
    for(int i=0; i<grassPos.size(); ++i)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, grassPos[i]);
        if(i<grassPos.size()/2)
            model = glm::rotate(model, glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        shader.setMat4("model", model);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        grass.Draw(GL_TRIANGLES);
    }
    // ground plane
    SetDirectionalLightParameters(shader);
    model = glm::mat4(1.0f);
    glEnable(GL_CULL_FACE);
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setMat4("model", model);
    grassPlane.Draw(GL_TRIANGLES);
    glDisable(GL_CULL_FACE);
}

void DrawAirBalloon(Shader &shader, Model &mm, glm::mat4 projection)
{
    shader.use();
    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("projection", projection);
    shader.setMat4("view", programState->camera->GetViewMatrix());
    model = glm::translate(model, mainModelState->mmPosition);
    model = glm::rotate(model, glm::radians(mainModelState->mmAngle), mainModelState->mmRotation);
    model = glm::rotate(model, glm::radians(mainModelState->mmTurnAngle), glm::vec3(0.f, 0.f, 1.f));
    model = glm::scale(model, glm::vec3(.0009f, .0009f, 0.0007f));
    shader.setMat4("model", model);
    mm.Draw(shader);
}

void AirBalloonIdleEvent(GLFWwindow *window)
{
    mainModelState->mmAngle += mainModelState->mmAngle >=-90.f ? -0.04 : 0.04;
    mainModelState->mmRotation.z += mainModelState->mmRotation.z >= 0.f ? -0.001 : 0.001;
    int space = glfwGetKey(window, GLFW_KEY_SPACE);
    int shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
    if(mainModelState->mmPosition.y >= 0.5 && (space != GLFW_PRESS || shift != GLFW_PRESS))
    {
        mainModelState->mmPosition.y += sin(glfwGetTime())*0.05f*deltaTime;
        programState->camera->updateCameraVectors(mainModelState->mmPosition);
    }
    else if(mainModelState->mmPosition.y <= 0.5 && mainModelState->mmPosition.y >= 0.0)
    {
        mainModelState->mmPosition.y -= 0.001f;
                programState->camera->updateCameraVectors(mainModelState->mmPosition);
    }
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

void DrawAxis(Shader &shader, const SimpleModel &axisSModel, const std::vector<glm::vec3> &axisColor, glm::mat4 projection)
{
    // drawing axis
    glm::mat4 model = glm::mat4(1.0f);
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", programState->camera->GetViewMatrix());
    for(int i=0; i<3; ++i)
    {
        model = glm::rotate(model, glm::radians(90.f), axisColor[i]);
        shader.setMat4("model", model);
        shader.setVec3("LineColor", axisColor[i]);
        axisSModel.Draw(GL_LINES);
    }
}

void DrawImGuiInfoWindows()
{
    if(programState->camera == fps_camera || programState->isCVars)
        return;

    if(mainModelState->mmPosition.x<=0.0 && mainModelState->mmPosition.x>=-4.0
       && mainModelState->mmPosition.z>=0.0 && mainModelState->mmPosition.z<=4.0)
    {
        ImGui::Begin("Welcome Home!");
        ImGui::SetWindowPos(ImVec2(60.0, 500.0));
        ImGui::Text("Hello traveler!\nUse your W-A-S-D keys to move around the map.\n"
                    "You can go up and down with your SPACE and SHIFT keys.\n"
                    "Also, you can rotate your camera around the air balloon using your mouse, for better views!\n"
                    "Try to get closer to the structures around the area to find out more about them!\n");
        ImGui::End();
    }
    else if(mainModelState->mmPosition.x<=4.0 && mainModelState->mmPosition.x>=-4.0
            && mainModelState->mmPosition.z>=11.0 && mainModelState->mmPosition.z<=19.0)
    {
        ImGui::Begin("Christ the Redeemer");
        ImGui::SetWindowPos(ImVec2(60.0, 500.0));
        ImGui::Text("This is statue of Jesus Christ located in Rio de Janeiro, Brazil.\n"
                    "The statue is 30 meters high!\n"
                    "The original design of the Christ the Redeemer statue was different to what we see today.\n"
                    "It was intended for Christ to be holding a globe in one hand and a cross in the other,\nrather"
                    " than two open arms.");
        ImGui::End();
    }
    else if(mainModelState->mmPosition.x<=19.0 && mainModelState->mmPosition.x>=11.0
            && mainModelState->mmPosition.z>=6.0 && mainModelState->mmPosition.z<=14.0)
    {
        ImGui::Begin("Leaning Tower of Pisa");
        ImGui::SetWindowPos(ImVec2(60.0, 500.0));
        ImGui::Text("The Tower of Pisa is freestanding bell tower of Pisa Cathedral located in Pisa, Italy.\n"
                    "The tower is 55m high!\n"
                    "The leaning of the tower is due to both a wrong assumption and poor engineering, but still, it\n"
                    "is a miracle of physics, because there is no good reason why the tower lasted for 800 years!");
        ImGui::End();
    }

    else if(mainModelState->mmPosition.x<=-14.0 && mainModelState->mmPosition.x>=-26.0
            && mainModelState->mmPosition.z>=-11.0 && mainModelState->mmPosition.z<=1.0)
    {
        ImGui::Begin("Big Ben");
        ImGui::SetWindowPos(ImVec2(60.0, 500.0));
        ImGui::Text("Big Ben is the nickname for the Great Bell of the Elizabeth Tower located in London, England.\n"
                    "The tower itself is 96m high!\n"
                    "The name Big Ben does not refer to the clock or the tower, but to the bell inside the tower!\n"
                    "Despite that, Big Ben became the nickname for the whole clock-tower.");
        ImGui::End();
    }
    else if(mainModelState->mmPosition.x<=11.0 && mainModelState->mmPosition.x>=-1.0
            && mainModelState->mmPosition.z>=-21.0 && mainModelState->mmPosition.z<=-9.0)
    {
        ImGui::Begin("Statue of Liberty");
        ImGui::SetWindowPos(ImVec2(60.0, 500.0));
        ImGui::Text("The Statue of Liberty is a colossal copper statue, a gift from the people of France located\n"
                    "in New York City, USA.\n"
                    "The statue is 93m high!\n"
                    "It was originally intended for Egypt and it would have called Egypt Carrying the Light to Asia,\n"
                    "but the project was rejected due to its cost and the idea was recycled to be The Statue of Liberty.");
        ImGui::End();
    }
}

void DrawCVarAndAxis(GLFWwindow *window, Shader &shader, const SimpleModel &axisSModel, const std::vector<glm::vec3> &axisColor, glm::mat4 projection)
{
    if(programState->isCVars)
    {
        DrawAxis(shader, axisSModel, axisColor, projection);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        ImGui::Begin("CVARS");
        ImGui::SetWindowSize(ImVec2(450.f, 110.f));
        if(ImGui::RadioButton("TPP Camera", programState->camera == tpp_camera))
            programState->camera = tpp_camera;
        else if(ImGui::RadioButton("FPS Camera", programState->camera == fps_camera))
            programState->camera = fps_camera;
        ImGui::DragFloat("Air Balloon speed", &mainModelState->mmSpeed, 0.1f, 0.1f, 2.f);
        ImGui::End();
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

void SetDirectionalLightParameters(Shader &shader)
{
    shader.use();
    shader.setVec3("viewPos", programState->camera->Position);
    shader.setVec3("dirLight.direction", programState->dirLight);
    shader.setVec3("dirLight.ambient", programState->dirAmbient);
    shader.setVec3("dirLight.diffuse", programState->dirDiffuse);
    shader.setVec3("dirLight.specular", programState->dirSpecular);
    shader.setFloat("material.shininess", 32.f);
}
