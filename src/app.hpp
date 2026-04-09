#include <SGE/engine.hpp>
#include <SGE/types/backend.hpp>
#include <LLGL/CommandBuffer.h>

struct AppConfig {
    float sensitivity = 1.0f;
};

class App final : public sge::IEngine {
public:
    App(const AppConfig& config);
    ~App();

private:
    void OnUpdate() override;
    void OnRender(const std::shared_ptr<sge::GlfwWindow>& window, double interpolation) override;

    void OnWindowResized(const std::shared_ptr<sge::GlfwWindow>& window, int width, int height) override {
        m_inv_projection_matrix = glm::inverse(
            glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f
        ));
    }

    void OnWindowDestroy(sge::GlfwWindow& window) override {
        if (window.GetID() == m_primary_window->GetID()) {
            Stop();
        }
    }

private:
    void InitPipeline();
    
private:
    glm::mat4 m_inv_view_matrix = glm::mat4(1.0);
    glm::mat4 m_inv_projection_matrix = glm::mat4(1.0);

    glm::vec3 m_camera_pos = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 m_camera_forward = glm::vec3(1.0, 0.0, 0.0);

    std::shared_ptr<sge::GlfwWindow> m_primary_window;
    LLGL::CommandBuffer* m_command_buffer = nullptr;
    LLGL::CommandQueue* m_command_queue = nullptr;
    LLGL::Buffer* m_vertex_buffer = nullptr;

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;

    float m_sensitivity = 1.0f;

    uint32_t m_pipeline_id = 0;
    uint32_t m_iterations = 1;
};