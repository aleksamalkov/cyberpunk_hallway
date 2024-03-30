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
    explicit Plane(const std::vector<glm::vec3>& vertex_pos, unsigned texture, float texture_size = 1.0f)
        : m_texture{texture}
    {
        const float tex_coord_x = glm::distance(vertex_pos[0], vertex_pos[1]) / texture_size;
        const float tex_coord_y = glm::distance(vertex_pos[1], vertex_pos[2]) / texture_size;
        const glm::vec3 normal = glm::cross(vertex_pos[0] - vertex_pos[2], vertex_pos[0] - vertex_pos[1]);
        constexpr int vertex_size = 3 + 3 + 2;
        const float vertices[] {
                vertex_pos[0].x, vertex_pos[0].y, vertex_pos[0].z, normal.x, normal.y, normal.z, 0.0f,        0.0f,        // bottom left
                vertex_pos[1].x, vertex_pos[1].y, vertex_pos[1].z, normal.x, normal.y, normal.z, tex_coord_x, 0.0f,        // bottom right
                vertex_pos[2].x, vertex_pos[2].y, vertex_pos[2].z, normal.x, normal.y, normal.z, tex_coord_x, tex_coord_y, // top right
                vertex_pos[3].x, vertex_pos[3].y, vertex_pos[3].z, normal.x, normal.y, normal.z, 0.0f,        tex_coord_y  // top left
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
        // normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_size * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // texture coordinates attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertex_size * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

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

struct Light {
    glm::vec3 position;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct Material {
    float shininess;
};

std::vector<Plane> generate_hallway(float width, float height, float length, unsigned floor_texture, unsigned wall_texture, unsigned ceiling_texture)
{
    constexpr float texture_size = 3.f;
    const glm::vec3 bottom_left_front{0.f, 0.f, 0.f};
    const glm::vec3 bottom_right_front{width, 0.f, 0.f};
    const glm::vec3 top_right_front{width, height, 0.f};
    const glm::vec3 top_left_front{0.f, height, 0.f};
    const glm::vec3 bottom_left_back{0.f, 0.f, length};
    const glm::vec3 bottom_right_back{width, 0.f, length};
    const glm::vec3 top_right_back{width, height, length};
    const glm::vec3 top_left_back{0.f, height, length};

    const Plane floor = Plane({bottom_left_front, bottom_right_front, bottom_right_back, bottom_left_back}, floor_texture, texture_size);
    const Plane front_wall = Plane({bottom_right_front, bottom_left_front, top_left_front, top_right_front}, wall_texture, texture_size);
    const Plane left_wall = Plane({bottom_left_front, bottom_left_back, top_left_back, top_left_front}, wall_texture, texture_size);
    const Plane right_wall = Plane({bottom_right_back, bottom_right_front, top_right_front, top_right_back}, wall_texture, texture_size);
    const Plane back_wall = Plane({bottom_left_back, bottom_right_back, top_right_back, top_left_back}, wall_texture, texture_size);
    const Plane ceiling = Plane({top_right_front, top_left_front, top_left_back, top_right_back}, ceiling_texture, texture_size);

    return {floor, front_wall, left_wall, right_wall, back_wall, ceiling};
}

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
//    glEnable(GL_CULL_FACE);

    // build and compile shaders
    // -------------------------
    Shader shader("resources/shaders/shader.vs", "resources/shaders/shader.fs");

    // load models
    // -----------
    const auto floor_texture = load_texture("container.jpg");
    std::vector<Plane> planes = generate_hallway(5.f, 5.f, 10.f, floor_texture, floor_texture, floor_texture);

    // draw in wireframe
//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    constexpr glm::vec3 clear_color{0.0f, 0.0f, 0.0f};
    state.camera.Position += glm::vec3{2.5f, 2.5f, 5.0f};


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

        shader.use();
        shader.setMat4("model", glm::mat4(1.0f));
        shader.setMat4("view", state.camera.GetViewMatrix());
        shader.setMat4("projection", projection);
        // point light 1
        shader.setVec3("lights[0].position", glm::vec3{1.f, 1.f, 1.f});
        shader.setVec3("lights[0].ambient", 0.05f, 0.05f, 0.05f);
        shader.setVec3("lights[0].diffuse", 0.8f, 0.8f, 0.8f);
        shader.setVec3("lights[0].specular", 1.0f, 1.0f, 1.0f);
        shader.setFloat("lights[0].constant", 1.0f);
        shader.setFloat("lights[0].linear", 0.35f);
        shader.setFloat("lights[0].quadratic", 0.44f);
        // point light 2
        shader.setVec3("lights[1].position", glm::vec3{4.f, 4.f, 6.f});
        shader.setVec3("lights[1].ambient", 0.05f, 0.05f, 0.05f);
        shader.setVec3("lights[1].diffuse", 0.8f, 0.8f, 0.8f);
        shader.setVec3("lights[1].specular", 1.0f, 1.0f, 1.0f);
        shader.setFloat("lights[1].constant", 1.0f);
        shader.setFloat("lights[1].linear", 0.35f);
        shader.setFloat("lights[1].quadratic", 0.44f);

        shader.setFloat("material.shininess", 32);
        shader.setVec3("viewPos", state.camera.Position);

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