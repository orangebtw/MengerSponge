#include <SGE/engine.hpp>
#include <SGE/types/backend.hpp>
#include <LLGL/Utils/VertexFormat.h>
#include <LLGL/CommandBuffer.h>

struct AppConfig {
    float sensitivity = 1.0f;
};

enum class Method : uint8_t {
    SDF = 0,
    Polygons = 1
};

class App final : public sge::IEngine {
public:
    App(const AppConfig& config) : m_config(config) {}

    bool Init() override;

private:
    void OnUpdate() override;
    void OnPostUpdate() override;
    void OnRender(const std::shared_ptr<sge::GlfwWindow>& window) override;

    void OnWindowResized(const std::shared_ptr<sge::GlfwWindow>& window, int width, int height) override {
        m_projection_matrix = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
        m_inv_projection_matrix = glm::inverse(m_projection_matrix);
    }

    void OnWindowDestroy(sge::GlfwWindow& window) override {
        if (window.GetID() == m_primary_window->GetID()) {
            Stop();
        }
    }

private:
    struct Mesh {
        LLGL::Buffer* vertex_buffer = nullptr;
        LLGL::Buffer* index_buffer = nullptr;
        size_t index_count = 0;
    };

    bool InitSDFPipeline();
    bool InitVertexPipeline();
    
private:
    glm::mat4 m_view_matrix = glm::mat4(1.0f);
    glm::mat4 m_projection_matrix = glm::mat4(1.0f);
    glm::mat4 m_inv_view_matrix = glm::mat4(1.0f);
    glm::mat4 m_inv_projection_matrix = glm::mat4(1.0f);

    glm::vec3 m_camera_pos = glm::vec3(0.0f);
    glm::vec3 m_camera_forward = glm::vec3(1.0, 0.0, 0.0);

    std::shared_ptr<sge::GlfwWindow> m_primary_window;
    LLGL::CommandBuffer* m_command_buffer = nullptr;
    LLGL::CommandQueue* m_command_queue = nullptr;
    LLGL::Buffer* m_sdf_vertex_buffer = nullptr;

    std::vector<Mesh> m_menger_sponge_meshes;

    LLGL::VertexFormat m_vertex_format;

    AppConfig m_config;

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;

    uint32_t m_sdf_pipeline_id = -1;
    uint32_t m_vertex_pipeline_id = -1;
    uint32_t m_iterations = 0;

    Method m_method = Method::SDF;
};