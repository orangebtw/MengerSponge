#include "app.hpp"
#include "SGE/input.hpp"
#include "SGE/time/time.hpp"
#include "SGE/types/cursor_mode.hpp"
#include <SGE/renderer/types.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/types/attributes.hpp>
#include <cstdlib>

App::App() {
    InitRenderContext(sge::RenderBackend::OpenGL);

    const auto& context = GetRenderContext()->Context();

    LLGL::CommandBufferDescriptor commandBufferDesc;
    m_command_buffer = context->CreateCommandBuffer(commandBufferDesc);

    m_command_queue = context->GetCommandQueue();

    sge::WindowSettings window_settings;
    window_settings.width = 1280;
    window_settings.height = 720;
    window_settings.vsync = true;
    window_settings.hidden = true;
    window_settings.cursor_mode = sge::CursorMode::Disabled;

    auto result = CreateWindow(window_settings);
    if (!result) {
        SGE_LOG_ERROR("Couldn't create a window: {}", result.error());
        std::abort();
    }
    
    m_primary_window = result.value();
    m_primary_window->SetRawMouseInput(true);

    InitPipeline();

    m_inv_projection_matrix = glm::inverse(glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f));

    m_primary_window->ShowWindow();
}

App::~App() {
    const auto& context = GetRenderContext()->Context();
    SGE_RESOURCE_RELEASE(m_command_buffer);
    SGE_RESOURCE_RELEASE(m_vertex_buffer);
}

void App::InitPipeline() {
    const auto& context = GetRenderContext()->Context();

    glm::vec2 vertices[] = {
        {-1.0f, -1.0f}, {0.0f, 0.0f},
        {3.0f, -1.0f}, {2.0f, 0.0f},
        {-1.0f, 3.0f}, {0.0f, 2.0f}
    };

    LLGL::VertexFormat vertexFormat = sge::Attributes(GetRenderContext()->Backend(), {
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_position", "Position"),
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_uv", "UV"),
    });

    LLGL::BufferDescriptor bufferDesc;
    bufferDesc.size = sizeof(vertices);
    bufferDesc.bindFlags = LLGL::BindFlags::VertexBuffer;
    bufferDesc.vertexAttribs = vertexFormat.attributes;
    m_vertex_buffer = context->CreateBuffer(bufferDesc, (void*)vertices);

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.uniforms = {
        LLGL::UniformDescriptor("uCameraToWorld", LLGL::UniformType::Float4x4),
        LLGL::UniformDescriptor("uCameraInverseProjection", LLGL::UniformType::Float4x4),
        LLGL::UniformDescriptor("uIterations", LLGL::UniformType::Int1),
    };

    LLGL::Shader* vertexShader = GetRenderContext()->LoadShader(sge::ShaderPath(sge::ShaderType::Vertex, "fullscreen_triangle"), {}, vertexFormat.attributes);
    assert(vertexShader != nullptr);
    LLGL::Shader* pixelShader = GetRenderContext()->LoadShader(sge::ShaderPath(sge::ShaderType::Fragment, "menger_sponge"));
    assert(pixelShader != nullptr);

    sge::GraphicsPipelineConfig pipelineConfig;
    pipelineConfig.layout = context->CreatePipelineLayout(pipelineLayoutDesc);
    pipelineConfig.vertexShader = vertexShader;
    pipelineConfig.pixelShader = pixelShader;

    m_pipeline_id = GetRenderContext()->AddPipelineConfig(pipelineConfig);
}

void App::OnFixedUpdate() {
    namespace Input = sge::Input;
    namespace Time = sge::Time;

    if (m_primary_window->GetCursorMode() != sge::CursorMode::Disabled)
        return;

    const float dt = Time::FixedDeltaSeconds();

    m_yaw += Input::MouseDelta().x * 25.0f * dt;
    m_pitch -= Input::MouseDelta().y * 25.0f * dt;

    m_yaw = std::fmod(m_yaw, 360.0f);
    m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

    glm::vec3 cameraDir;
    cameraDir.x = glm::cos(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
    cameraDir.y = glm::sin(glm::radians(m_pitch));
    cameraDir.z = glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
    glm::vec3 forward = glm::normalize(cameraDir);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    constexpr float SPEED = 1.0f; 

    if (Input::Pressed(sge::Key::W)) {
        m_camera_pos += cameraDir * SPEED * dt;
    }
    if (Input::Pressed(sge::Key::S)) {
        m_camera_pos -= cameraDir * SPEED * dt;
    }
    if (Input::Pressed(sge::Key::D)) {
        glm::vec3 right = glm::cross(forward, up);
        m_camera_pos += right * SPEED * dt;
    }
    if (Input::Pressed(sge::Key::A)) {
        glm::vec3 right = glm::cross(forward, up);
        m_camera_pos -= right * SPEED * dt;
    }
    if (Input::Pressed(sge::Key::Space)) {
        m_camera_pos += up * SPEED * dt;
    }
    if (Input::Pressed(sge::Key::LeftShift)) {
        m_camera_pos -= up * SPEED * dt;
    }

    m_inv_view_matrix = glm::inverse(glm::lookAt(m_camera_pos, m_camera_pos + forward, up));
}

void App::OnUpdate() {
    namespace Input = sge::Input;

    if (Input::JustPressed(sge::Key::ArrowUp)) {
        m_iterations++;
    }

    if (Input::JustPressed(sge::Key::ArrowDown) && m_iterations > 1) {
        m_iterations--;
    }

    if (Input::JustPressed(sge::Key::Escape) && m_primary_window->IsFocused()) {
        bool isNormalCursor = m_primary_window->GetCursorMode()  == sge::CursorMode::Normal;
        m_primary_window->SetCursorMode(isNormalCursor ? sge::CursorMode::Disabled : sge::CursorMode::Normal);
    }
}

void App::OnRender(const std::shared_ptr<sge::GlfwWindow> &window, double) {
    LLGL::SwapChain& swapChain = GetRenderContext()->GetOrCreateSwapChain(window);
    GetRenderContext()->SetCurrentRenderTarget(&swapChain);

    LLGL::Extent2D windowSize = window->GetSize();
    
    m_command_buffer->Begin();
        m_command_buffer->BeginRenderPass(swapChain);
            m_command_buffer->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            m_command_buffer->SetViewport(windowSize);

            m_command_buffer->SetPipelineState(GetRenderContext()->GetOrCreatePipeline(m_pipeline_id));

            m_command_buffer->SetUniforms(0, &m_inv_view_matrix, sizeof(m_inv_view_matrix));
            m_command_buffer->SetUniforms(1, &m_inv_projection_matrix, sizeof(m_inv_projection_matrix));
            m_command_buffer->SetUniforms(2, &m_iterations, sizeof(m_iterations));
            m_command_buffer->SetVertexBuffer(*m_vertex_buffer);
            m_command_buffer->Draw(3, 0);
        m_command_buffer->EndRenderPass();
    m_command_buffer->End();

    m_command_queue->Submit(*m_command_buffer);
    GetRenderContext()->Present(window);
}