#include <iostream>
#include <vector>
#include <array>
#include <chrono>
#include <random>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

//PARAMETERS

//If batching test is enabled, rendering will finish after all points are altered.
#define BATCHING_TEST

const int width = 2560;
const int height = 1440;
const unsigned int pointCount = 67'108'864;
const unsigned int batchSize = 16'384;

GLFWwindow* Initialize(int width, int height);
unsigned int CreateShaderProgram();
unsigned int CreateRandomPointBuffer(unsigned int count);
unsigned int CreateVertexArrayObject(unsigned int vbo);
void AlterBuffer(unsigned int vbo, unsigned int& offset, unsigned int batchSize, unsigned int seed);
void Shutdown(GLFWwindow* window);

int main()
{
    GLFWwindow* window = Initialize(width, height);
    unsigned int program = CreateShaderProgram();

    unsigned int vbo = CreateRandomPointBuffer(pointCount);
    unsigned int vao = CreateVertexArrayObject(vbo);

    glViewport(0, 0, width, height);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glEnable(GL_DEPTH_TEST);

    unsigned int frame = 0u;
    unsigned int offset = 0u;
    float sum = 0.f;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        auto start = std::chrono::high_resolution_clock::now();

#ifdef BATCHING_TEST
        if (!(offset + batchSize > pointCount))
        {
            AlterBuffer(vbo, offset, batchSize, frame);
        }
        else
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
#endif // BATCHING_TEST

        //------Renderpass start------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, pointCount);

        glfwSwapBuffers(window);
        //------Renderpass end------

        std::chrono::duration<float, std::milli> ms = std::chrono::high_resolution_clock::now() - start;

        frame += 1u;
        sum += ms.count();
    }

    std::cout << sum / static_cast<float>(frame) << std::endl;
    std::cout << frame << std::endl;

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    Shutdown(window);
}

GLFWwindow* Initialize(int width, int height)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Graph", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    return window;
}

unsigned int CreateShaderProgram()
{
    const char* vertexShaderSource = R"(
#version 460 core
layout(location = 0) in vec2 aPosition;
void main()
{
    gl_Position = vec4(aPosition.x, aPosition.y, 0.f, 1.f);
}
)";

    const char* fragmentShaderSource = R"(
#version 460 core
out vec4 fragColor;
void main()
{
    fragColor = vec4(1.f, 1.f, 1.f, 1.f);
}
)";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

unsigned int CreateRandomPointBuffer(unsigned int count)
{
    std::vector<float> buffer;
    buffer.reserve(count * 2);
    
    std::default_random_engine engine;
    std::uniform_real_distribution xDistribution(-1.f, 0.f);
    std::uniform_real_distribution yDistribution(-1.f, 1.f);

    for (unsigned int i = 0; i < count; i++)
    {
        buffer.push_back(xDistribution(engine));
        buffer.push_back(yDistribution(engine));
    }

    unsigned int vbo = 0;
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(float), buffer.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return vbo;
}

unsigned int CreateVertexArrayObject(unsigned int vbo)
{
    unsigned int vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

    glBindVertexArray(0);
    return vao;
}

void AlterBuffer(unsigned int vbo, unsigned int& offset, unsigned int batchSize, unsigned int seed)
{
    std::vector<float> buffer;
    buffer.reserve(batchSize * 2);

    std::default_random_engine engine(seed);
    std::uniform_real_distribution xDistribution(0.f, 1.f);
    std::uniform_real_distribution yDistribution(-1.f, 1.f);

    for (unsigned int i = 0; i < batchSize; i++)
    {
        buffer.push_back(xDistribution(engine));
        buffer.push_back(yDistribution(engine));
    }

    //8 value comes from: 2 float per point [i.e. x,y] * sizeof(float) [which is 4] 
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, offset * 8, batchSize * 8, buffer.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    offset += batchSize;
}

void Shutdown(GLFWwindow* window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}
