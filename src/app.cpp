#include "app.hpp"
#include <SGE/input.hpp>
#include <SGE/math/math.hpp>
#include <SGE/time/time.hpp>
#include <SGE/types/cursor_mode.hpp>
#include <SGE/renderer/types.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/types/attributes.hpp>

struct Vertex {
    glm::vec3 position;
    uint8_t ao_id;
    int8_t normal_x;
    int8_t normal_y;
    int8_t normal_z;
};

struct Faces {
    bool front = true;
    bool back = true;
    bool left = true;
    bool right = true;
    bool top = true;
    bool bottom = true;
};

struct AO {
    uint8_t left[4]   = {3, 3, 3, 3};
    uint8_t right[4]  = {3, 3, 3, 3};
    uint8_t top[4]    = {3, 3, 3, 3};
    uint8_t bottom[4] = {3, 3, 3, 3};
    uint8_t front[4]  = {3, 3, 3, 3};
    uint8_t back[4]   = {3, 3, 3, 3};
};

bool App::Init() {
    if (!IEngine::Init())
        return false;
    if (!InitRenderContext(sge::RenderBackend::OpenGL))
        return false;

    m_command_buffer = GetRenderContext()->CreateCommandBuffer();
    m_command_queue = GetRenderContext()->GetCommandQueue();

    sge::WindowSettings window_settings;
    window_settings.width = 1280;
    window_settings.height = 720;
    window_settings.vsync = true;
    window_settings.hidden = true;
    window_settings.cursor_mode = sge::CursorMode::Disabled;

    auto result = CreateWindow(window_settings);
    if (!result) {
        SGE_LOG_ERROR("Couldn't create a window: {}", result.error());
        return false;
    }
    
    m_primary_window = result.value();
    m_primary_window->SetRawMouseInput(true);

    if (!InitSDFPipeline())
        return false;

    if (!InitVertexPipeline())
        return false;

    m_projection_matrix = glm::perspectiveLH_ZO(glm::radians(45.0f), 1280.0f / 720.0f, 0.001f, 100.0f);
    m_inv_projection_matrix = glm::inverse(m_projection_matrix);

    m_primary_window->ShowWindow();

    return true;
}

static void GenerateCube(std::vector<Vertex>& vertices, glm::vec3 pos, glm::vec3 size, Faces faces = Faces(), const AO& ao = AO()) {
    if (faces.front) {
        // Front Face
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 0, 0) * size,
            .ao_id = ao.front[0],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = -1
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 0, 0) * size,
            .ao_id = ao.front[3],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = -1
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 1, 0) * size,
            .ao_id = ao.front[2],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = -1
        });

        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 1, 0) * size,
            .ao_id = ao.front[2],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = -1
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 1, 0) * size,
            .ao_id = ao.front[1],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = -1
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 0, 0) * size,
            .ao_id = ao.front[0],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = -1
        });
    }

    if (faces.right) {
        // Right Face
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 0, 0) * size,
            .ao_id = ao.right[0],
            .normal_x = 1,
            .normal_y = 0,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 0, 1) * size,
            .ao_id = ao.right[3],
            .normal_x = 1,
            .normal_y = 0,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 1, 1) * size,
            .ao_id = ao.right[2],
            .normal_x = 1,
            .normal_y = 0,
            .normal_z = 0
        });

        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 1, 1) * size,
            .ao_id = ao.right[2],
            .normal_x = 1,
            .normal_y = 0,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 1, 0) * size,
            .ao_id = ao.right[1],
            .normal_x = 1,
            .normal_y = 0,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 0, 0) * size,
            .ao_id = ao.right[0],
            .normal_x = 1,
            .normal_y = 0,
            .normal_z = 0
        });
    }

    if (faces.back) {
        // Back face
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 0, 1) * size,
            .ao_id = ao.back[3],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = 1
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 0, 1) * size,
            .ao_id = ao.back[0],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = 1
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 1, 1) * size,
            .ao_id = ao.back[1],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = 1
        });
        

        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 1, 1) * size,
            .ao_id = ao.back[1],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = 1
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 1, 1) * size,
            .ao_id = ao.back[2],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = 1
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 0, 1) * size,
            .ao_id = ao.back[3],
            .normal_x = 0,
            .normal_y = 0,
            .normal_z = 1
        });
        
    }

    if (faces.left) {
        // Left Face
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 0, 1) * size,
            .ao_id = ao.left[3],
            .normal_x = -1,
            .normal_y = 0,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 0, 0) * size,
            .ao_id = ao.left[0],
            .normal_x = -1,
            .normal_y = 0,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 1, 0) * size,
            .ao_id = ao.left[1],
            .normal_x = -1,
            .normal_y = 0,
            .normal_z = 0
        });

        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 1, 0) * size,
            .ao_id = ao.left[1],
            .normal_x = -1,
            .normal_y = 0,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 1, 1) * size,
            .ao_id = ao.left[2],
            .normal_x = -1,
            .normal_y = 0,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 0, 1) * size,
            .ao_id = ao.left[3],
            .normal_x = -1,
            .normal_y = 0,
            .normal_z = 0
        });

    }

    if (faces.top) {
        // Top Face
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 1, 0) * size,
            .ao_id = ao.top[0],
            .normal_x = 0,
            .normal_y = 1,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 1, 0) * size,
            .ao_id = ao.top[1],
            .normal_x = 0,
            .normal_y = 1,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 1, 1) * size,
            .ao_id = ao.top[2],
            .normal_x = 0,
            .normal_y = 1,
            .normal_z = 0
        });

        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 1, 1) * size,
            .ao_id = ao.top[2],
            .normal_x = 0,
            .normal_y = 1,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 1, 1) * size,
            .ao_id = ao.top[3],
            .normal_x = 0,
            .normal_y = 1,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 1, 0) * size,
            .ao_id = ao.top[0],
            .normal_x = 0,
            .normal_y = 1,
            .normal_z = 0
        });
    }
    
    if (faces.bottom) {
        // Bottom Face
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 0, 1) * size,
            .ao_id = ao.bottom[3],
            .normal_x = 0,
            .normal_y = -1,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 0, 1) * size,
            .ao_id = ao.bottom[2],
            .normal_x = 0,
            .normal_y = -1,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 0, 0) * size,
            .ao_id = ao.bottom[1],
            .normal_x = 0,
            .normal_y = -1,
            .normal_z = 0
        });

        vertices.push_back(Vertex{
            .position = pos + glm::vec3(1, 0, 0) * size,
            .ao_id = ao.bottom[1],
            .normal_x = 0,
            .normal_y = -1,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 0, 0) * size,
            .ao_id = ao.bottom[0],
            .normal_x = 0,
            .normal_y = -1,
            .normal_z = 0
        });
        vertices.push_back(Vertex{
            .position = pos + glm::vec3(0, 0, 1) * size,
            .ao_id = ao.bottom[3],
            .normal_x = 0,
            .normal_y = -1,
            .normal_z = 0
        });
    }
}

static int powi(int base, uint32_t power) {
    int result = 1;
    for (uint32_t i = 0; i < power; ++i) {
        result *= base;
    }
    return result;
}

static bool CubeIsValid(int x, int y, int z) {
    while ((x + y + z) > 0) {
        int cx = x % 3;
        int cy = y % 3;
        int cz = z % 3;

        if (
            (cx % 3 == 1 and cy % 3 == 1) ||
            (cx % 3 == 1 and cz % 3 == 1) ||
            (cy % 3 == 1 and cz % 3 == 1)
        ) {
            return false;
        }
        
        x /= 3;
        y /= 3;
        z /= 3;
    }
    return true;
}

static size_t Flatten3DIndex(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height) {
    return (z * width * height) + (y * width) + x;
}

static bool NeighborExists(int x, int y, int z, int max_x, int max_y, int max_z) {
    if (x < 0 || y < 0 || z < 0)
        return false;
    if (x >= max_x || y >= max_y || z >= max_z)
        return false;
    
    return CubeIsValid(x, y, z);
}


static void CalculateAO(uint8_t (&ao)[4], int x, int y, int z, uint8_t axis, int max_x, int max_y, int max_z) {
    uint8_t a = 0;
    uint8_t b = 0;
    uint8_t c = 0;
    uint8_t d = 0;
    uint8_t e = 0;
    uint8_t f = 0;
    uint8_t g = 0;
    uint8_t h = 0;

    // X
    if (axis == 0) {
        a = !NeighborExists(x, y,     z - 1, max_x, max_y, max_z);
        b = !NeighborExists(x, y - 1, z - 1, max_x, max_y, max_z);
        c = !NeighborExists(x, y - 1, z,     max_x, max_y, max_z);
        d = !NeighborExists(x, y - 1, z + 1, max_x, max_y, max_z);
        e = !NeighborExists(x, y,     z + 1, max_x, max_y, max_z);
        f = !NeighborExists(x, y + 1, z + 1, max_x, max_y, max_z);
        g = !NeighborExists(x, y + 1, z,     max_x, max_y, max_z);
        h = !NeighborExists(x, y + 1, z - 1, max_x, max_y, max_z);
    }
    // Y
    else if (axis == 1) {
        a = !NeighborExists(x,     y, z - 1, max_x, max_y, max_z);
        b = !NeighborExists(x - 1, y, z - 1, max_x, max_y, max_z);
        c = !NeighborExists(x - 1, y, z,     max_x, max_y, max_z);
        d = !NeighborExists(x - 1, y, z + 1, max_x, max_y, max_z);
        e = !NeighborExists(x,     y, z + 1, max_x, max_y, max_z);
        f = !NeighborExists(x + 1, y, z + 1, max_x, max_y, max_z);
        g = !NeighborExists(x + 1, y, z,     max_x, max_y, max_z);
        h = !NeighborExists(x + 1, y, z - 1, max_x, max_y, max_z);
    }
    // Z
    else if (axis == 2) {
        a = !NeighborExists(x - 1, y,     z, max_x, max_y, max_z);
        b = !NeighborExists(x - 1, y - 1, z, max_x, max_y, max_z);
        c = !NeighborExists(x,     y - 1, z, max_x, max_y, max_z);
        d = !NeighborExists(x + 1, y - 1, z, max_x, max_y, max_z);
        e = !NeighborExists(x + 1, y,     z, max_x, max_y, max_z);
        f = !NeighborExists(x + 1, y + 1, z, max_x, max_y, max_z);
        g = !NeighborExists(x    , y + 1, z, max_x, max_y, max_z);
        h = !NeighborExists(x - 1, y + 1, z, max_x, max_y, max_z);
    }

    ao[0] = a + b + c;
    ao[1] = g + h + a;
    ao[2] = e + f + g;
    ao[3] = c + d + e;
}

static void GenerateMengerSponge(std::vector<Vertex>& vertices, uint32_t iterations, glm::vec3 position, glm::vec3 size) {
    if (iterations == 0) {
        GenerateCube(vertices, position, size);
        return;
    }

    int iters = static_cast<int>(iterations);

    glm::vec3 pos = position;
    glm::vec3 s = size / std::powf(3.0f, iterations);

    int mx = powi(3, iters);
    int my = powi(3, iters);
    int mz = powi(3, iters);

    for (int x = 0; x < mx; ++x) {
        for (int y = 0; y < my; ++y) {
            for (int z = 0; z < mz; ++z) {
                if (CubeIsValid(x, y, z)) {
                    glm::vec3 p = pos + glm::vec3(x, y, z) * s;

                    Faces faces = {
                        .front  = !NeighborExists(x, y, z - 1, mx, my, mz),
                        .back   = !NeighborExists(x, y, z + 1, mx, my, mz),
                        .left   = !NeighborExists(x - 1, y, z, mx, my, mz),
                        .right  = !NeighborExists(x + 1, y, z, mx, my, mz),
                        .top    = !NeighborExists(x, y + 1, z, mx, my, mz),
                        .bottom = !NeighborExists(x, y - 1, z, mx, my, mz),
                    };

                    AO ao = AO();

                    if (faces.front) {
                        CalculateAO(ao.front, x, y, z - 1, 2, mx, my, mz);
                    }
                    if (faces.back) {
                        CalculateAO(ao.back, x, y, z + 1, 2, mx, my, mz);
                    }
                    if (faces.left) {
                        CalculateAO(ao.left, x - 1, y, z, 0, mx, my, mz);
                    }
                    if (faces.right) {
                        CalculateAO(ao.right, x + 1, y, z, 0, mx, my, mz);
                    }
                    if (faces.top) {
                        CalculateAO(ao.top, x, y + 1, z, 1, mx, my, mz);
                    }
                    if (faces.bottom) {
                        CalculateAO(ao.bottom, x, y - 1, z, 1, mx, my, mz);
                    }
                    
                    GenerateCube(vertices, p, s, faces, ao);
                }
            }
        }
    }
}

bool App::InitSDFPipeline() {
    glm::vec2 vertices[] = {
        {-1.0f, -1.0f}, {0.0f, 0.0f},
        {3.0f, -1.0f}, {2.0f, 0.0f},
        {-1.0f, 3.0f}, {0.0f, 2.0f}
    };

    LLGL::VertexFormat vertexFormat = sge::Attributes(GetRenderContext()->Backend(), {
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_position", "Position"),
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_uv", "UV"),
    });

    m_sdf_vertex_buffer = GetRenderContext()->CreateVertexBuffer(vertices, vertexFormat);
    if (m_sdf_vertex_buffer == nullptr)
        return false;

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.uniforms = {
        LLGL::UniformDescriptor("uCameraToWorld", LLGL::UniformType::Float4x4),
        LLGL::UniformDescriptor("uCameraInverseProjection", LLGL::UniformType::Float4x4),
        LLGL::UniformDescriptor("uIterations", LLGL::UniformType::Int1),
    };

    LLGL::Shader* vertexShader = GetRenderContext()->LoadShaderFromFile(sge::ShaderPath(sge::ShaderType::Vertex, "menger_sponge"), {}, vertexFormat.attributes);
    if (vertexShader == nullptr)
        return false;

    LLGL::Shader* pixelShader = GetRenderContext()->LoadShaderFromFile(sge::ShaderPath(sge::ShaderType::Fragment, "menger_sponge"));
    if (pixelShader == nullptr)
        return false;

    sge::GraphicsPipelineConfig pipelineConfig;
    pipelineConfig.layout = GetRenderContext()->GetLLGLContext()->CreatePipelineLayout(pipelineLayoutDesc);
    pipelineConfig.vertexShader = vertexShader;
    pipelineConfig.pixelShader = pixelShader;

    m_sdf_pipeline_id = GetRenderContext()->AddPipelineConfig(pipelineConfig);

    return true;
}

bool App::InitVertexPipeline() {
    m_vertex_format = sge::Attributes(GetRenderContext()->Backend(), {
        sge::Attribute::Vertex(LLGL::Format::RGB32Float, "a_position", "Position"),
        sge::Attribute::Vertex(LLGL::Format::R8UInt, "a_ao_id", "AoID"),
        sge::Attribute::Vertex(LLGL::Format::R8SInt, "a_normal_x", "NormalX"),
        sge::Attribute::Vertex(LLGL::Format::R8SInt, "a_normal_y", "NormalY"),
        sge::Attribute::Vertex(LLGL::Format::R8SInt, "a_normal_z", "NormalZ"),
    });

    std::vector<Vertex> vertices;
    GenerateCube(vertices, glm::vec3(0.0f), glm::vec3(2.0f));

    LLGL::Buffer* buffer = GetRenderContext()->CreateVertexBuffer(vertices, m_vertex_format);
    if (buffer == nullptr)
        return false;

    m_menger_sponge_meshes.push_back(Mesh{ .vertex_buffer = buffer, .vertex_count = vertices.size() });

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.uniforms = {
        LLGL::UniformDescriptor("uViewMatrix", LLGL::UniformType::Float4x4),
        LLGL::UniformDescriptor("uProjectionMatrix", LLGL::UniformType::Float4x4),
    };

    LLGL::Shader* vertexShader = GetRenderContext()->LoadShaderFromFile(sge::ShaderPath(sge::ShaderType::Vertex, "basic"), {}, m_vertex_format.attributes);
    if (vertexShader == nullptr)
        return false;

    LLGL::Shader* pixelShader = GetRenderContext()->LoadShaderFromFile(sge::ShaderPath(sge::ShaderType::Fragment, "basic"));
    if (pixelShader == nullptr)
        return false;

    sge::GraphicsPipelineConfig pipelineConfig;
    pipelineConfig.layout = GetRenderContext()->GetLLGLContext()->CreatePipelineLayout(pipelineLayoutDesc);
    pipelineConfig.vertexShader = vertexShader;
    pipelineConfig.pixelShader = pixelShader;
    pipelineConfig.cullMode = LLGL::CullMode::Back;
    pipelineConfig.frontCCW = true;
    pipelineConfig.depth.testEnabled = true;
    pipelineConfig.depth.writeEnabled = true;

    m_vertex_pipeline_id = GetRenderContext()->AddPipelineConfig(pipelineConfig);

    return true;
}

void App::OnUpdate() {
    namespace Input = sge::Input;
    namespace Time = sge::Time;

    if (Input::JustPressed(sge::Key::ArrowUp)) {
        m_iterations++;
    }

    if (Input::JustPressed(sge::Key::ArrowDown) && m_iterations > 0) {
        m_iterations--;
    }

    if (Input::JustPressed(sge::Key::Escape) && m_primary_window->IsFocused()) {
        bool isNormalCursor = m_primary_window->GetCursorMode()  == sge::CursorMode::Normal;
        m_primary_window->SetCursorMode(isNormalCursor ? sge::CursorMode::Disabled : sge::CursorMode::Normal);
    }

    if (m_primary_window->GetCursorMode() != sge::CursorMode::Disabled)
        return;

    const float dt = Time::DeltaSeconds();
    const bool is_right_handed = GetRenderContext()->Backend().IsRightHanded();

    bool changed = false;

    float prev_yaw = m_yaw;
    float prev_pitch = m_pitch;
    
    m_yaw -= Input::MouseDelta().x * 0.05f * m_config.sensitivity;
    m_pitch -= Input::MouseDelta().y * 0.05f * m_config.sensitivity;

    if (!sge::approx_equals(prev_yaw, m_yaw))
        changed = true;
    if (!sge::approx_equals(prev_pitch, m_pitch))
        changed = true;

    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    if (changed) {
        m_yaw = std::fmod(m_yaw, 360.0f);
        m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

        glm::vec3 cameraDir;
        cameraDir.x = glm::cos(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
        cameraDir.y = glm::sin(glm::radians(m_pitch));
        cameraDir.z = glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
        m_camera_forward = glm::normalize(cameraDir);
    }

    float speed = 1.0f;
    if (Input::Pressed(sge::Key::LeftCtrl)) {
        speed *= 0.01f;
    }

    if (Input::Pressed(sge::Key::W)) {
        m_camera_pos += m_camera_forward * speed * dt;
        changed = true;
    }
    if (Input::Pressed(sge::Key::S)) {
        m_camera_pos -= m_camera_forward * speed * dt;
        changed = true;
    }
    if (Input::Pressed(sge::Key::D)) {
        glm::vec3 right = glm::normalize(glm::cross(up, m_camera_forward));
        m_camera_pos += right * speed * dt;
        changed = true;
    }
    if (Input::Pressed(sge::Key::A)) {
        glm::vec3 right = glm::normalize(glm::cross(up, m_camera_forward));
        m_camera_pos -= right * speed * dt;
        changed = true;
    }
    if (Input::Pressed(sge::Key::Space)) {
        m_camera_pos += up * speed * dt;
        changed = true;
    }
    if (Input::Pressed(sge::Key::LeftShift)) {
        m_camera_pos -= up * speed * dt;
        changed = true;
    }

    if (changed) {
        SGE_LOG_DEBUG("[{}, {}, {}]", m_camera_pos.x, m_camera_pos.y, m_camera_pos.z);
        m_view_matrix = glm::lookAtLH(m_camera_pos, m_camera_pos + m_camera_forward, up);
        m_inv_view_matrix = glm::inverse(m_view_matrix);
    }
}

void App::OnPostUpdate() {
    if (m_iterations >= m_menger_sponge_meshes.size()) {
        std::vector<Vertex> vertices;
        GenerateMengerSponge(vertices, m_iterations, glm::vec3(0.0f), glm::vec3(2.0f));
        SGE_LOG_INFO("Iteration: {}, Vertex count: {}, Bytes: {}", m_iterations, vertices.size(), vertices.size() * sizeof(Vertex));

        m_menger_sponge_meshes.push_back(Mesh {
            .vertex_buffer = GetRenderContext()->CreateVertexBuffer(vertices, m_vertex_format),
            .vertex_count = vertices.size()
        });
    }
}

void App::OnRender(const std::shared_ptr<sge::GlfwWindow> &window) {
    LLGL::SwapChain& swapChain = GetRenderContext()->GetOrCreateSwapChain(window);
    GetRenderContext()->SetCurrentRenderTarget(&swapChain);

    const LLGL::Extent2D windowSize = window->GetSize();

#if 0
    uint32_t iterations = m_iterations + 1;

    m_command_buffer->Begin();
        m_command_buffer->BeginRenderPass(swapChain);
            m_command_buffer->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
            m_command_buffer->SetViewport(windowSize);

            m_command_buffer->SetPipelineState(GetRenderContext()->GetOrCreatePipeline(m_sdf_pipeline_id));
            m_command_buffer->SetVertexBuffer(*m_sdf_vertex_buffer);

            m_command_buffer->SetUniforms(0, &m_inv_view_matrix, sizeof(m_inv_view_matrix));
            m_command_buffer->SetUniforms(1, &m_inv_projection_matrix, sizeof(m_inv_projection_matrix));
            m_command_buffer->SetUniforms(2, &iterations, sizeof(iterations));
            m_command_buffer->Draw(3, 0);
        m_command_buffer->EndRenderPass();
    m_command_buffer->End();
#else
    const Mesh& mesh = m_menger_sponge_meshes[m_iterations];

    m_command_buffer->Begin();
        m_command_buffer->BeginRenderPass(swapChain);
            m_command_buffer->Clear(LLGL::ClearFlags::ColorDepth, LLGL::ClearValue(0.4f, 0.4f, 0.4f, 1.0f, 1.0f));
            m_command_buffer->SetViewport(windowSize);

            m_command_buffer->SetPipelineState(GetRenderContext()->GetOrCreatePipeline(m_vertex_pipeline_id));
            m_command_buffer->SetVertexBuffer(*mesh.vertex_buffer);

            m_command_buffer->SetUniforms(0, &m_view_matrix, sizeof(m_view_matrix));
            m_command_buffer->SetUniforms(1, &m_projection_matrix, sizeof(m_projection_matrix));
            m_command_buffer->Draw(mesh.vertex_count, 0);
        m_command_buffer->EndRenderPass();
    m_command_buffer->End();
#endif

    m_command_queue->Submit(*m_command_buffer);
    GetRenderContext()->Present(window);
}