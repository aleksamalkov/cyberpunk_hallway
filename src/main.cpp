#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <stdexcept>
#include <vector>

unsigned load_texture(const std::string& filename)
{
    unsigned texture{};
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width{}, height{}, channels{};
    unsigned char* data = stbi_load(FileSystem::getPath("resources/textures/" + filename).c_str(), &width, &height, &channels, 0);
    if (!data) {
        throw std::runtime_error("Can't find texture " + filename);
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

class Plane {
public:
    explicit Plane(const std::vector<glm::vec3>& vertex_pos, unsigned texture, float texture_size_coeff = 1.0f)
        : m_texture{texture}
    {
        const float texture_coords = 1.0f / texture_size_coeff;
        constexpr int vertex_size = 5;
        const float vertices[] {
                vertex_pos[0].x, vertex_pos[0].y, vertex_pos[0].z, 0.0f,           0.0f,           // bottom left
                vertex_pos[1].x, vertex_pos[1].y, vertex_pos[1].z, texture_coords, 0.0f,           // bottom right
                vertex_pos[2].x, vertex_pos[2].y, vertex_pos[2].z, texture_coords, texture_coords, // top right
                vertex_pos[3].x, vertex_pos[3].y, vertex_pos[3].z, 0.0f,           texture_coords  // top left
        };
        const unsigned indices[] = {
            0, 1, 2, // first triangle
            2, 3, 0  // second triangle
        };

        // TODO: destruct them later
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        bind();

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_size * sizeof(float), (void*)nullptr);
        glEnableVertexAttribArray(0);
        // texture coordinates attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertex_size * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        unbind();
    }

    void draw(Shader shader)
    {
        shader.use();
        bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        unbind();
    }

private:
    void bind() const
    {
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    }

    static void unbind()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    unsigned m_texture{};
    unsigned m_VBO{};
    unsigned m_VAO{};
    unsigned m_EBO{};
};

struct State
{
    Camera camera{};
    double mouse_pos_x{};
    double mouse_pos_y{};
};

void process_input(GLFWwindow *window, State& state, float delta_time);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

// screen dimensions
constexpr float screen_width = 800.0f;
constexpr float screen_height = 600.0f;

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(screen_width, screen_height, "LearnOpenGL", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    State state;
    glfwGetCursorPos(window, &state.mouse_pos_x, &state.mouse_pos_y);
    glfwSetWindowUserPointer(window, &state);
    glfwSetCursorPosCallback(window, mouse_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader shader("resources/shaders/shader.vs", "resources/shaders/shader.fs");

    // load models
    // -----------
    const auto texture = load_texture("container.jpg");
    std::vector<Plane> planes;
    constexpr float hallway_length = 10.f / 2.f;
    constexpr float hallway_height = 5.f / 2.f;
    constexpr float hallway_width = 5.f / 2.f;
    planes.emplace_back(
            std::vector<glm::vec3>{
                    {-hallway_width, -hallway_height, -hallway_length},
                    {hallway_width,  -hallway_height, -hallway_length},
                    {hallway_width,  -hallway_height, hallway_length},
                    {-hallway_width, -hallway_height, hallway_length},
            }, texture, 0.7
    );

    // draw in wireframe
//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    constexpr glm::vec3 clear_color{0.0f, 0.0f, 0.0f};

    // timing
    double last_frame = glfwGetTime();

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        const double current_frame = glfwGetTime();
        const double delta_time = current_frame - last_frame;
        last_frame = current_frame;

        // input
        // -----
        process_input(window, state, static_cast<float>(delta_time));


        // render
        // ------
        glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const auto projection = glm::perspective(glm::radians(45.0f), screen_width / screen_height, 0.1f, 100.0f);
        const auto view = state.camera.GetViewMatrix();

        shader.setMat4("model", glm::mat4(1.0f));
        shader.setMat4("view", state.camera.GetViewMatrix());
        shader.setMat4("projection", projection);

        for (auto& plane : planes) {
            plane.draw(shader);
        }


        // glfw: swap buffers and poll IO events
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void process_input(GLFWwindow *window, State& state, float delta_time){
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    auto& camera = state.camera;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, delta_time);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, delta_time);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, delta_time);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, delta_time);
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    auto* state = static_cast<State*>(glfwGetWindowUserPointer(window));

    const double x_offset = xpos - state->mouse_pos_x;
    state->mouse_pos_x = xpos;
    const double y_offset = state->mouse_pos_y - ypos;
    state->mouse_pos_y = ypos;
    state->camera.ProcessMouseMovement(static_cast<float>(x_offset), static_cast<float>(y_offset));
}