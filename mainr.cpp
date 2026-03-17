#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// Вершинный шейдер
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out VS_OUT {
    vec3 pos;
    vec3 normal;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vs_out.pos = vec3(model * vec4(aPos, 1.0));
    vs_out.normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Геометрический шейдер (исправлен: uniform'ы объявлены до использования)
const char* geometryShaderSource = R"(
#version 330 core
layout(triangles) in;
layout(line_strip, max_vertices = 5) out;

in VS_OUT {
    vec3 pos;
    vec3 normal;
} gs_in[];

out vec3 gColor;

uniform mat4 view;
uniform mat4 projection;
uniform float furLength = 0.5;   // увеличена длина
uniform float time;

// Псевдослучайная функция
float random(vec3 seed) {
    return fract(sin(dot(seed, vec3(12.9898, 78.233, 45.5432))) * 43758.5453);
}

void generateFur(vec3 basePos, vec3 normal, int vertexID) {
    // Построение касательного пространства
    vec3 tangent = normalize(cross(normal, vec3(0.0, 1.0, 0.0)));
    if (length(tangent) < 0.1)
        tangent = normalize(cross(normal, vec3(1.0, 0.0, 0.0)));
    vec3 bitangent = cross(normal, tangent);

    // Случайные параметры для каждого волоска
    float r1 = random(basePos + vec3(vertexID, 1.0, 0.0));
    float r2 = random(basePos + vec3(vertexID, 2.0, 0.0));
    float angle = r1 * 2.0 * 3.14159;
    float offset = (r2 - 0.5) * 0.2;

    // Направление роста с небольшим отклонением от нормали
    vec3 dir = normalize(normal + (tangent * cos(angle) + bitangent * sin(angle)) * offset);

    // Генерация сегментов волоска
    for (int i = 0; i < 5; i++) {
        float t = float(i) / 4.0;
        vec3 pos = basePos + dir * furLength * t;
        // Изгиб для естественности (с учётом времени)
        vec3 bend = tangent * sin(t * 3.14159 * 2.0 + r1 * 10.0 + time) * 0.05;
        pos += bend;
        gl_Position = projection * view * vec4(pos, 1.0);
        gColor = vec3(0.9, 0.7, 0.3); // яркий золотистый
        EmitVertex();
    }
    EndPrimitive();
}

void main() {
    for (int i = 0; i < 3; i++) {
        generateFur(gs_in[i].pos, normalize(gs_in[i].normal), i);
    }
}
)";

// Фрагментный шейдер
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 gColor;

void main() {
    FragColor = vec4(gColor, 1.0);
}
)";

// Создание сферы
void createSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, int stacks, int slices) {
    vertices.clear();
    indices.clear();

    for (int i = 0; i <= stacks; ++i) {
        float phi = glm::pi<float>() * float(i) / float(stacks);
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);

        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * glm::pi<float>() * float(j) / float(slices);
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            float x = sinPhi * cosTheta;
            float y = cosPhi;
            float z = sinPhi * sinTheta;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
}

void checkShader(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
    }
}

void checkProgram(GLuint program) {
    GLint success;
    GLchar infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << std::endl;
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Fur Rendering", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(2.0f); // чуть толще линии

    // Компиляция шейдеров
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkShader(vertexShader, "VERTEX");

    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometryShader, 1, &geometryShaderSource, NULL);
    glCompileShader(geometryShader);
    checkShader(geometryShader, "GEOMETRY");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkShader(fragmentShader, "FRAGMENT");

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, geometryShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    glDeleteShader(fragmentShader);

    // Создание сферы
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    createSphere(sphereVertices, sphereIndices, 30, 30);

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint timeLoc = glGetUniformLocation(shaderProgram, "time");
    GLuint furLengthLoc = glGetUniformLocation(shaderProgram, "furLength");

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    float timeValue = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        timeValue = (float)glfwGetTime();

        glm::mat4 view = glm::lookAt(glm::vec3(2.5f, 1.8f, 3.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), timeValue * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(timeLoc, timeValue);
        glUniform1f(furLengthLoc, 0.4f); // длина шерсти

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)sphereIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}