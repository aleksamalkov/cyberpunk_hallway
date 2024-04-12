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
#include <learnopengl/model_edited.h>

#include <iostream>
#include <stdexcept>
#include <vector>

// executes an action on the exit from scopes
// from the book "A Tour Of C++" by Bjarne Stroustrup
template <class F>
struct Final_action {
    explicit Final_action(F f) :act(f) {}
    ~Final_action() { act(); }
    F act;
};

// takes an action to be executed on the exit from scope
// from the book "A Tour Of C++" by Bjarne Stroustrup
template <class F>
[[nodiscard]] auto finally(F f)
{
    return Final_action{f};
}

struct Settings
{
    glm::vec3 ambient {0.05f, 0.05f, 0.05f};
    glm::vec3 diffuse {0.8f, 2.8f, 0.8f};
    glm::vec3 specular {3.0f, 2.5f, 1.0f};
    glm::vec3 ambient1 {0.05f, 0.05f, 0.05f};
    glm::vec3 diffuse1 {1.8f, 0.8f, 0.8f};
    glm::vec3 specular1 {3.5f, 2.0f, 1.0f};
    float constant = 1.0f;
    float linear = 0.9f;
    float quadratic = 0.24f;
    float shininess = 32.0f;
    float gamma = 2.2f;
    float exposure = 0.8f;
    float height = 0.01f;
    int min_layers = 4;
    int max_layers = 8;
    bool bloom = true;
    int blur_amount = 4;
    float view_angle = 60;
};

unsigned load_texture(const std::string& filename, bool gamma_correction = false) {
    unsigned texture{};
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::cout << "Loading texture " + filename + " ";
    int width{}, height{}, channels{};
    unsigned char* data = stbi_load(FileSystem::getPath("resources/textures/" + filename).c_str(), &width, &height, &channels, 3);
    if (!data) {
        throw std::runtime_error("Can't find texture " + filename);
    }

    if (channels == 1) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
        std::cout << " asGL_RED" << std::endl;
    } else  if (channels == 3) {
        if (gamma_correction) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            std::cout << "as GL_SRGB" << std::endl;
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            std::cout << "as GL_RGB" << std::endl;
        }
    } else if (channels == 4) {
        if (gamma_correction) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            std::cout << "as GL_SRGB_ALPHA" << std::endl;
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            std::cout << " as GL_RGBA" << std::endl;
        }
    } else {
        throw std::runtime_error("Texture has " + std::to_string(channels) + " channels");
    }
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

class TextureGroup {
public:
    explicit TextureGroup(unsigned diffuse, unsigned normal = 0, unsigned specular = 0, unsigned height = 0, unsigned emission = 0)
        : m_diffuse{diffuse}, m_normal{normal}, m_specular{specular}, m_height{height}, m_emission{emission}
    {
    }

    void bind(Shader& shader) const
    {
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(shader.ID, "texture_diffuse1"), 0);
        glBindTexture(GL_TEXTURE_2D, m_diffuse);
        glActiveTexture(GL_TEXTURE1);
        glUniform1i(glGetUniformLocation(shader.ID, "texture_specular1"), 1);
        if (m_specular) {
            glBindTexture(GL_TEXTURE_2D, m_specular);
        } else {
            glBindTexture(GL_TEXTURE_2D, m_diffuse);
        }
        glActiveTexture(GL_TEXTURE2);
        glUniform1i(glGetUniformLocation(shader.ID, "texture_normal1"), 2);
        if (m_normal) {
            glBindTexture(GL_TEXTURE_2D, m_normal);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glActiveTexture(GL_TEXTURE3);
        glUniform1i(glGetUniformLocation(shader.ID, "texture_height1"), 3);
        if (m_height) {
            glBindTexture(GL_TEXTURE_2D, m_height);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glActiveTexture(GL_TEXTURE4);
        glUniform1i(glGetUniformLocation(shader.ID, "texture_emission1"), 4);
        if (m_emission) {
            glBindTexture(GL_TEXTURE_2D, m_height);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    static void unbind()
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

private:
    unsigned m_diffuse;
    unsigned m_normal;
    unsigned m_specular;
    unsigned m_height;
    unsigned m_emission;
};

class Plane {
public:
    Plane(const std::vector<glm::vec3>& vertex_pos, TextureGroup textures, float texture_size = 2.0f)
        : m_textures{textures}
    {
        init(vertex_pos, texture_size);
    }

    void draw(Shader& shader) const
    {
        shader.use();
        bind(shader);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        unbind();
    }

    Plane(const Plane&) = delete;
    Plane& operator=(const Plane&) = delete;

    ~Plane()
    {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
    }

    Plane(Plane&& p) noexcept
        : m_textures(p.m_textures), m_VAO{p.m_VAO}, m_VBO{p.m_VBO}
    {
        p.m_VAO = 0;
        p.m_VBO = 0;
    }

private:
    void init(const std::vector<glm::vec3>& vertex_pos, float texture_size)
    {
        const float tex_coord_x = glm::distance(vertex_pos[0], vertex_pos[1]) / texture_size;
        const float tex_coord_y = glm::distance(vertex_pos[1], vertex_pos[2]) / texture_size;
        const glm::vec3 normal = glm::normalize(glm::cross(vertex_pos[0] - vertex_pos[1], vertex_pos[0] - vertex_pos[2]));

        const std::vector<glm::vec2> tex_pos {{0.0f, 0.0f}, {tex_coord_x, 0}, {tex_coord_x, tex_coord_y}, {0.0f, tex_coord_y}};

        glm::vec3 tangent{};
        {
            glm::vec3 edge1 = vertex_pos[2] - vertex_pos[1];
            glm::vec3 edge2 = vertex_pos[3] - vertex_pos[1];
            glm::vec2 deltaUV1 = tex_pos[2] - tex_pos[1];
            glm::vec2 deltaUV2 = tex_pos[3] - tex_pos[1];
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        }

        constexpr int vertex_size = 3 + 3 + 2 + 3;
        const float vertices[] {
                // first triangle
                vertex_pos[0].x, vertex_pos[0].y, vertex_pos[0].z, normal.x, normal.y, normal.z, tex_pos[0].x, tex_pos[0].y, // bottom left
                tangent.x, tangent.y, tangent.z,
                vertex_pos[1].x, vertex_pos[1].y, vertex_pos[1].z, normal.x, normal.y, normal.z, tex_pos[1].x, tex_pos[1].y, // bottom right
                tangent.x, tangent.y, tangent.z,
                vertex_pos[2].x, vertex_pos[2].y, vertex_pos[2].z, normal.x, normal.y, normal.z, tex_pos[2].x, tex_pos[2].y, // top right
                tangent.x, tangent.y, tangent.z,
                // second triangle
                vertex_pos[2].x, vertex_pos[2].y, vertex_pos[2].z, normal.x, normal.y, normal.z, tex_pos[2].x, tex_pos[2].y, // top right
                tangent.x, tangent.y, tangent.z,
                vertex_pos[3].x, vertex_pos[3].y, vertex_pos[3].z, normal.x, normal.y, normal.z, tex_pos[3].x, tex_pos[3].y, // top left
                tangent.x, tangent.y, tangent.z,
                vertex_pos[0].x, vertex_pos[0].y, vertex_pos[0].z, normal.x, normal.y, normal.z, tex_pos[0].x, tex_pos[0].y, // bottom left
                tangent.x, tangent.y, tangent.z
        };
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_size * sizeof(float), (void*)nullptr);
        glEnableVertexAttribArray(0);
        // normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_size * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // texture coordinates attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertex_size * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        // tangent attribute
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, vertex_size * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void bind(Shader& shader) const
    {
        m_textures.bind(shader);
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    }

    static void unbind()
    {
        TextureGroup::unbind();
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    TextureGroup m_textures;
    unsigned m_VAO{};
    unsigned m_VBO{};
};

struct State
{
    Camera camera{glm::vec3{2.5f, 1.5f, -8.0f}, glm::vec3{0.f, 1.f, 0.f}, 90.f};
    double mouse_pos_x{};
    double mouse_pos_y{};
    bool gui_enabled{};
    int window_width{1280};
    int window_height{720};
    bool is_mouse_initialized{};
};

std::vector<Plane> generate_hallway(float width, float height, float length, TextureGroup floor_tex, TextureGroup wall_tex, TextureGroup ceiling_tex)
{
    constexpr float texture_size = 3.f;
    const glm::vec3 bottom_left_front{0.f, 0.f, 0.f};
    const glm::vec3 bottom_right_front{width, 0.f, 0.f};
    const glm::vec3 top_right_front{width, height, 0.f};
    const glm::vec3 top_left_front{0.f, height, 0.f};
    const glm::vec3 bottom_left_back{0.f, 0.f, -length};
    const glm::vec3 bottom_right_back{width, 0.f, -length};
    const glm::vec3 top_right_back{width, height, -length};
    const glm::vec3 top_left_back{0.f, height, -length};


    std::vector<Plane> planes;
    planes.reserve(6);
    // floor
    planes.push_back(Plane({bottom_left_front, bottom_right_front, bottom_right_back, bottom_left_back}, floor_tex, texture_size));
    // walls
    planes.push_back(Plane({bottom_right_front, bottom_left_front, top_left_front, top_right_front}, wall_tex, texture_size)); // front
    planes.push_back(Plane({bottom_left_front, bottom_left_back, top_left_back, top_left_front}, wall_tex, texture_size)); // left
    planes.push_back(Plane({bottom_right_back, bottom_right_front, top_right_front, top_right_back}, wall_tex, texture_size)); // right
    planes.push_back(Plane({bottom_left_back, bottom_right_back, top_right_back, top_left_back}, wall_tex, texture_size)); // back
    // ceiling
    planes.push_back(Plane({top_right_front, top_left_front, top_left_back, top_right_back}, ceiling_tex, texture_size));

    return planes;
}

class FPS_counter
{
public:
    double next_frame()
    {
        const double current_frame = glfwGetTime();
        const double delta = current_frame - m_last_frame;
        frame_rate = 1.0 / delta;
        m_last_frame = current_frame;
        return delta;
    }
    double frame_rate{};
private:
    double m_last_frame{glfwGetTime()};
};

void draw_gui(Settings& settings, const FPS_counter& fps_counter)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("FPS");
    ImGui::SetWindowCollapsed(false, ImGuiCond_Once);
    ImGui::SetWindowPos({10, 10}, ImGuiCond_Once);
    ImGui::SetWindowSize({100, 50}, ImGuiCond_Once);
    ImGui::Text("%s", std::to_string(fps_counter.frame_rate).c_str());
    ImGui::End();

    ImGui::Begin("Settings");
    ImGui::SetWindowPos({10, 70}, ImGuiCond_Once);
    ImGui::SetWindowSize({550, 550}, ImGuiCond_Once);
    ImGui::SetWindowCollapsed(false, ImGuiCond_Once);
    ImGui::DragFloat("gamma", &settings.gamma, 0.005, 0.0f, 4.0f);
    ImGui::DragFloat("exposure", &settings.exposure, 0.005, 0.0f, 4.0f);
    ImGui::DragFloat("view_angle", &settings.view_angle, 0.1, 20.0f, 90.0f);
    ImGui::ColorEdit3("ambient", reinterpret_cast<float *>(&settings.ambient));
    ImGui::ColorEdit3("diffuse", reinterpret_cast<float *>(&settings.diffuse));
    ImGui::ColorEdit3("specular", reinterpret_cast<float *>(&settings.specular));
    ImGui::ColorEdit3("ambient1", reinterpret_cast<float *>(&settings.ambient1));
    ImGui::ColorEdit3("diffuse1", reinterpret_cast<float *>(&settings.diffuse1));
    ImGui::ColorEdit3("specular1", reinterpret_cast<float *>(&settings.specular1));
    ImGui::DragFloat("attenuation.constant", &settings.constant, 0.005, 0.0, 2.0);
    ImGui::DragFloat("attenuation.linear", &settings.linear, 0.005, 0.0, 2.0);
    ImGui::DragFloat("attenuation.quadratic", &settings.quadratic, 0.005, 0.0, 2.0);
    ImGui::DragFloat("shininess", &settings.shininess, 0.25, 0.0, 1000.0);
    ImGui::DragFloat("height", &settings.height, 0.00005, 0.00f, 0.5f);
    ImGui::DragInt("min_layers", &settings.min_layers, 0.1, 1, 512);
    ImGui::DragInt("max_layers", &settings.max_layers, 0.1, 1, 512);
    ImGui::Checkbox("bloom", &settings.bloom);
    ImGui::DragInt("bloom blur amount", &settings.blur_amount, 0.2, 0, 50);
    ImGui::Text("Keybindings:");
    ImGui::BulletText("Q or F1 - open/close settings and help");
    ImGui::BulletText("W A S D - move");
    ImGui::BulletText("ESC - close settings or window");
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

class Framebuffer
{
public:
    Framebuffer(int width, int height, bool create_depth_buffer = true)
        : m_width(width), m_height(height)
    {
        glGenFramebuffers(1, &m_framebuffer);

        glGenTextures(1, &m_color_buffer);
        glBindTexture(GL_TEXTURE_2D, m_color_buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (create_depth_buffer) {
            glGenRenderbuffers(1, &m_depth_buffer);
            glBindRenderbuffer(GL_RENDERBUFFER, m_depth_buffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color_buffer, 0);
        if (create_depth_buffer) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_buffer);
        }
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: Framebuffer not complete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    ~Framebuffer()
    {
        glDeleteTextures(1, &m_color_buffer);
        if (m_depth_buffer)
            glDeleteRenderbuffers(1, &m_depth_buffer);
        glDeleteFramebuffers(1, &m_framebuffer);
    }

    void bind() // NOLINT(*-make-member-function-const): Can be used to change framebuffer
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    }

    static void unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    unsigned int color_buffer() // NOLINT(*-make-member-function-const): Can be used to change color buffer
    {
        return m_color_buffer;
    }

    void update_size(int width, int height)
    {
        if (width != m_width || height != m_height) {
            resize(width, height);
            m_width = width;
            m_height = height;
        }
    }

private:
    void resize(int width, int height) // NOLINT(*-make-member-function-const): Changes framebuffer
    {
        glBindTexture(GL_TEXTURE_2D, m_color_buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        if (m_depth_buffer) {
            glBindRenderbuffer(GL_RENDERBUFFER, m_depth_buffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }
    }

    int m_width{};
    int m_height{};
    unsigned int m_framebuffer{};
    unsigned int m_color_buffer{};
    unsigned int m_depth_buffer{};
};

void process_input(GLFWwindow *window, State& state, float delta_time);
void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main() {
    Settings settings;
    State state;

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    auto terminate_glfw = finally([&]{glfwTerminate();});
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(state.window_width, state.window_height, "LearnOpenGL", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glfwSetWindowUserPointer(window, &state);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    auto imgui_destroy = finally([&]{
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    });

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    std::cout << "\nCompiling shaders..." << std::endl;
    std::cout << "Compiling main shader" << std::endl;
    Shader shader("resources/shaders/shader.vs", "resources/shaders/shader.fs");
    std::cout << "Compiling no normal mapping shader" << std::endl;
    Shader no_normal_shader("resources/shaders/shader.vs", "resources/shaders/no_normal_mapping.fs");
    std::cout << "Compiling bright fragment extraction shader" << std::endl;
    Shader bright_shader("resources/shaders/screen.vs", "resources/shaders/bright.fs");
    std::cout << "Compiling blur shaders" << std::endl;
    Shader blur_vertical_shader("resources/shaders/screen.vs", "resources/shaders/blur_vertical.fs");
    Shader blur_horizontal_shader("resources/shaders/screen.vs", "resources/shaders/blur_horizontal.fs");
    std::cout << "Compiling tone mapping shader" << std::endl;
    Shader screen_shader("resources/shaders/screen.vs", "resources/shaders/screen.fs");

    // load models
    // -----------
    std::cout << "\nLoading models..." << std::endl;

    stbi_set_flip_vertically_on_load(false);

    std::cout << "Loading lamp model" << std::endl;
    Model light_model(FileSystem::getPath("resources/objects/lamp/lamp.obj"), true);
    std::cout << "Loading arcade model" << std::endl;
    Model arcade_model(FileSystem::getPath("resources/objects/rusty_japanese_arcade/rusty_japanese_arcade.obj"), true);
    std::cout << "Loading trash model" << std::endl;
    Model trash_model(FileSystem::getPath("resources/objects/trash/trash.obj"), true);
    std::cout << "Loading door model" << std::endl;
    Model door_model(FileSystem::getPath("resources/objects/door/door.obj"), true);
    std::cout << "Loading vending machine model" << std::endl;
    Model vending_model(FileSystem::getPath("resources/objects/ramen_vending_machine/vending.obj"), true);
    std::cout << "Loading poster model" << std::endl;
    Model poster_model(FileSystem::getPath("resources/objects/poster/poster.obj"), true);
    std::cout << "Loading bottle model" << std::endl;
    Model bottle_model(FileSystem::getPath("resources/objects/broken_glass_bottle/bottle.obj"), false);

    stbi_set_flip_vertically_on_load(true);

    std::cout << "\nLoading textures..." << std::endl;

    const auto floor_diffuse_texture = load_texture("Checker_Tiles/Checker_Tiles_DIFF.png", true);
    const auto floor_normal_texture = load_texture("Checker_Tiles/Checker_Tiles_NRM.png");
    const auto floor_specular_texture = load_texture("Checker_Tiles/Checker_Tiles_SPEC.png");
    const auto floor_height_texture = load_texture("Checker_Tiles/Checker_Tiles_Height.png");

    const auto wall_diffuse_texture = load_texture("Dirty_Concrete/Dirty_Concrete_DIFF.png", true);
    const auto wall_normal_texture = load_texture("Dirty_Concrete/Dirty_Concrete_NRM.png");
    const auto wall_specular_texture = load_texture("Dirty_Concrete/Dirty_Concrete_SPEC.png");
    const auto wall_height_texture = load_texture("Dirty_Concrete/Dirty_Concrete_DISP.png");

    auto delete_textures = finally([&]{
        glDeleteTextures(1, &floor_diffuse_texture);
        glDeleteTextures(1, &floor_normal_texture);
        glDeleteTextures(1, &floor_specular_texture);
        glDeleteTextures(1, &floor_height_texture);

        glDeleteTextures(1, &wall_diffuse_texture);
        glDeleteTextures(1, &wall_normal_texture);
        glDeleteTextures(1, &wall_specular_texture);
        glDeleteTextures(1, &wall_height_texture);
    });

    TextureGroup floor {floor_diffuse_texture, floor_normal_texture, floor_specular_texture, floor_height_texture};
    TextureGroup wall {wall_diffuse_texture, wall_normal_texture, wall_specular_texture, wall_height_texture};
    const float hallway_width = 5.f;
    const float hallway_height = 4.f;
    const float hallway_length = 10.f;
    std::vector<Plane> planes = generate_hallway(hallway_width, hallway_height, hallway_length, floor, wall, wall);

    Framebuffer hdr_buffer{state.window_width, state.window_height};
    Framebuffer bright_buffer{state.window_width, state.window_height, false};
    Framebuffer blur_buffer{state.window_width, state.window_height, false};

    Plane screen_plane({{-1.f, -1.f, 0.f}, {1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}, {-1.f, 1.f, 0.f}}, TextureGroup{hdr_buffer.color_buffer()});
    Plane bright_plane({{-1.f, -1.f, 0.f}, {1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}, {-1.f, 1.f, 0.f}}, TextureGroup{bright_buffer.color_buffer(), 0, hdr_buffer.color_buffer()});
    Plane blur_plane({{-1.f, -1.f, 0.f}, {1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}, {-1.f, 1.f, 0.f}}, TextureGroup{blur_buffer.color_buffer()});

    // draw in wireframe
//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    std::cout << "\nLoading done\n" << std::endl;

    constexpr glm::vec3 clear_color{0.0f, 0.0f, 0.0f};

    FPS_counter fps_counter;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        const double delta_time = fps_counter.next_frame();

        // input
        // -----
        process_input(window, state, static_cast<float>(delta_time));
        hdr_buffer.update_size(state.window_width, state.window_height);
        bright_buffer.update_size(state.window_width, state.window_height);
        blur_buffer.update_size(state.window_width, state.window_height);


        // render to framebuffer
        // ---------------------
        hdr_buffer.bind();
        glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const auto projection = glm::perspective(glm::radians(settings.view_angle),
                                                 static_cast<float>(state.window_width) / static_cast<float>(state.window_height),
                                                 0.1f, 100.0f);

        const auto view = state.camera.GetViewMatrix();

        shader.use();
        shader.setMat4("model", glm::mat4(1.0f));
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        // point light 1
        shader.setVec3("lights[0].position", glm::vec3{hallway_width - 0.5f, hallway_height - 0.5f, -3.f * hallway_length / 4.f});
        shader.setVec3("lights[0].ambient", settings.ambient1.r, settings.ambient1.g, settings.ambient1.b);
        shader.setVec3("lights[0].diffuse", settings.diffuse1.r, settings.diffuse1.g, settings.diffuse1.b);
        shader.setVec3("lights[0].specular", settings.specular1.r, settings.specular1.g, settings.specular1.b);
        shader.setFloat("lights[0].constant", settings.constant);
        shader.setFloat("lights[0].linear", settings.linear);
        shader.setFloat("lights[0].quadratic", settings.quadratic);
        // point light 2
        shader.setVec3("lights[1].position", glm::vec3{hallway_width / 2, hallway_height - 0.5f, -0.5f});
        shader.setVec3("lights[1].ambient", settings.ambient.r, settings.ambient.g, settings.ambient.b);
        shader.setVec3("lights[1].diffuse", settings.diffuse.r, settings.diffuse.g, settings.diffuse.b);
        shader.setVec3("lights[1].specular", settings.specular.r, settings.specular.g, settings.specular.b);
        shader.setFloat("lights[1].constant", settings.constant);
        shader.setFloat("lights[1].linear", settings.linear);
        shader.setFloat("lights[1].quadratic", settings.quadratic);

        shader.setFloat("shininess", settings.shininess);
        shader.setFloat("heightScale", settings.height);
        shader.setFloat("minLayers", static_cast<float>(settings.min_layers));
        shader.setFloat("maxLayers", static_cast<float>(settings.max_layers));
        shader.setVec3("viewPos", state.camera.Position);

        no_normal_shader.use();
        no_normal_shader.setMat4("view", view);
        no_normal_shader.setMat4("projection", projection);
        // point light 1
        no_normal_shader.setVec3("lights[0].position", glm::vec3{hallway_width - 0.5f, hallway_height - 0.5f, -3.f * hallway_length / 4.f});
        no_normal_shader.setVec3("lights[0].ambient", settings.ambient1.r, settings.ambient1.g, settings.ambient1.b);
        no_normal_shader.setVec3("lights[0].diffuse", settings.diffuse1.r, settings.diffuse1.g, settings.diffuse1.b);
        no_normal_shader.setVec3("lights[0].specular", settings.specular1.r, settings.specular1.g, settings.specular1.b);
        no_normal_shader.setFloat("lights[0].constant", settings.constant);
        no_normal_shader.setFloat("lights[0].linear", settings.linear);
        no_normal_shader.setFloat("lights[0].quadratic", settings.quadratic);
        // point light 2
        no_normal_shader.setVec3("lights[1].position", glm::vec3{hallway_width / 2, hallway_height - 0.5f, -0.5f});
        no_normal_shader.setVec3("lights[1].ambient", settings.ambient.r, settings.ambient.g, settings.ambient.b);
        no_normal_shader.setVec3("lights[1].diffuse", settings.diffuse.r, settings.diffuse.g, settings.diffuse.b);
        no_normal_shader.setVec3("lights[1].specular", settings.specular.r, settings.specular.g, settings.specular.b);
        no_normal_shader.setFloat("lights[1].constant", settings.constant);
        no_normal_shader.setFloat("lights[1].linear", settings.linear);
        no_normal_shader.setFloat("lights[1].quadratic", settings.quadratic);

        no_normal_shader.setFloat("shininess", settings.shininess);
        no_normal_shader.setVec3("viewPos", state.camera.Position);

        shader.use();

        for (auto& plane : planes) {
            plane.draw(shader);
        }

        {
            // arcade machine
            shader.use();
            TextureGroup::unbind();
            glm::mat4 model(1.f);
            model = glm::translate(model, glm::vec3(0.55f, 0.f, -hallway_length / 3.f));
            model = glm::rotate(model, glm::pi<float>() / 2.f, glm::vec3(0.f, 1.f, 0.f));
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
            shader.setMat4("model", model);
            arcade_model.Draw(shader);
        }

        {
            // trash
            shader.use();
            TextureGroup::unbind();
            glm::mat4 model(1.f);
            model = glm::translate(model, glm::vec3(hallway_width / 2.f, 0.f, -1.f));
            shader.setMat4("model", model);
            trash_model.Draw(shader);
        }

        {
            // doors
            no_normal_shader.use();
            TextureGroup::unbind();

            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(hallway_width - 0.1f, 0.f, -hallway_length / 2.f));
            model = glm::rotate(model, -glm::pi<float>() / 2.f, glm::vec3(0.f, 1.f, 0.f));
            no_normal_shader.setMat4("model", model);
            door_model.Draw(no_normal_shader);

            no_normal_shader.setMat4("model",
                           glm::rotate(
                                   glm::translate(glm::mat4(1.f),
                                                  glm::vec3(hallway_width - 0.1f, 0.f, -hallway_length / 4.f)),
                                   -glm::pi<float>() / 2.f,
                                   glm::vec3(0.f, 1.f, 0.f)
                           )
            );
            door_model.Draw(no_normal_shader);

            no_normal_shader.setMat4("model", glm::translate(glm::mat4(1.f), glm::vec3(hallway_width / 2.0f, 0.f, -hallway_length + 0.1f)));
            door_model.Draw(no_normal_shader);
        }

        {
            // ramen machine
            shader.use();
            TextureGroup::unbind();
            glm::mat4 model(1.f);
            model = glm::translate(model, glm::vec3(0.4, 0.f, - 2.f * hallway_length / 3.f));
            model = glm::rotate(model, glm::pi<float>() / 2.f, glm::vec3(0.f, 1.f, 0.f));
            model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));
            shader.setMat4("model", model);
            vending_model.Draw(shader);
        }

        {
            // poster
            shader.use();
            TextureGroup::unbind();
            glm::mat4 model(1.f);
            model = glm::translate(model, glm::vec3(hallway_width - 0.03f, hallway_height / 2.f, - 3.f * hallway_length / 4.f));
            model = glm::rotate(model, -glm::pi<float>() / 2.f, glm::vec3(0.f, 1.f, 0.f));
            model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));
            shader.setMat4("model", model);
            poster_model.Draw(shader);
        }

        {
            // bottle
            shader.use();
            TextureGroup::unbind();
            glm::mat4 model(1.f);
            model = glm::translate(model, glm::vec3(hallway_width / 3.f, 0.07f, -hallway_length / 2.f));
            model = glm::rotate(model, glm::pi<float>() / 2.f, glm::vec3(1.f, 0.f, 0.f));
            model = glm::rotate(model, -glm::pi<float>() / 12.f, glm::vec3(0.f, 0.f, 1.f));
            model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
            shader.setMat4("model", model);
            bottle_model.Draw(shader);
        }

        {
            // lamp
            shader.use();
            TextureGroup::unbind();
            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3{hallway_width / 2, hallway_height, -0.2f});
            model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.f, 0.f, 0.f));
            model = glm::scale(model, glm::vec3(0.03, 0.03, 0.03));
            shader.setMat4("model", model);
            light_model.Draw(shader);

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3{hallway_width - 0.2f, hallway_height, -3.f * hallway_length / 4.f});
            model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.f, 0.f, 0.f));
            model = glm::rotate(model, glm::pi<float>() / 2, glm::vec3(0.f, 1.f, 0.f));
            model = glm::scale(model, glm::vec3(0.03, 0.03, 0.03));
            shader.setMat4("model", model);
            light_model.Draw(shader);
        }

        Framebuffer::unbind();

        if (settings.bloom) {
            // extract bright fragments
            // ------------------------
            bright_buffer.bind();
            glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            bright_shader.use();
            screen_plane.draw(bright_shader);
            Framebuffer::unbind();

            // blur bright fragments
            // ---------------------
            blur_buffer.bind();
            glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            Framebuffer::unbind();

            const int blur_amount = settings.blur_amount;
            for (int i = 0; i < blur_amount; i++) {
                blur_buffer.bind();
                bright_plane.draw(blur_horizontal_shader);
                Framebuffer::unbind();

                bright_buffer.bind();
                blur_plane.draw(blur_vertical_shader);
                Framebuffer::unbind();
            }
        } else {
            bright_buffer.bind();
            glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            Framebuffer::unbind();
        }

        // tone-mapping
        // ------------
        glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        screen_shader.use();
        screen_shader.setFloat("gamma", settings.gamma);
        screen_shader.setFloat("exposure", settings.exposure);
        bright_plane.draw(screen_shader);


        if (state.gui_enabled) {
            draw_gui(settings, fps_counter);
        }

        // glfw: swap buffers and poll IO events
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void process_input(GLFWwindow *window, State& state, float delta_time){
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

void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos)
{
    auto* state = static_cast<State*>(glfwGetWindowUserPointer(window));

    if (!state->is_mouse_initialized) {
        state->mouse_pos_x = xpos;
        state->mouse_pos_y = ypos;
        state->is_mouse_initialized = true;
    }

    const double x_offset = xpos - state->mouse_pos_x;
    state->mouse_pos_x = xpos;
    const double y_offset = state->mouse_pos_y - ypos;
    state->mouse_pos_y = ypos;

    if (!state->gui_enabled) {
        state->camera.ProcessMouseMovement(static_cast<float>(x_offset), static_cast<float>(y_offset));
    }
}

void key_callback(GLFWwindow *window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods) {
    auto* state = static_cast<State*>(glfwGetWindowUserPointer(window));

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (state->gui_enabled) {
            state->gui_enabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetWindowShouldClose(window, true);
        }
    }

    if ((key == GLFW_KEY_F1 || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
        state->gui_enabled = !state->gui_enabled;
        if (state->gui_enabled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    auto* state = static_cast<State*>(glfwGetWindowUserPointer(window));
    state->window_width = width;
    state->window_height = height;
    glViewport(0, 0, width, height);
}