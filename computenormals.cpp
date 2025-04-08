#include <iostream>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include "./simplex.h"

// Câmera
struct Camera {
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    
    float yaw;
    float pitch;
    
    float movementSpeed;
    float mouseSensitivity;
    
    Camera(glm::vec3 position = glm::vec3(0.0f, 1.5f, 2.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f, float pitch = -30.0f) 
        : position(position), worldUp(up), yaw(yaw), pitch(pitch),
          movementSpeed(2.5f), mouseSensitivity(0.1f) {
        updateCameraVectors();
    }
    
    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }
    
    void processKeyboard(int direction, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        if (direction == 0)
            position += front * velocity;
        if (direction == 1) 
            position -= front * velocity;
        if (direction == 2)
            position -= right * velocity;
        if (direction == 3)
            position += right * velocity;
    }
    
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;
        
        yaw += xoffset;
        pitch += yoffset;
        
        if (constrainPitch) {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }
        
        updateCameraVectors();
    }
    
private:
    void updateCameraVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);
        
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool firstMouse = true;
float lastX = 400.0f, lastY = 300.0f;

Camera camera;

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window);

glm::mat2 m2 = glm::mat2(  0.80,  0.60,
    -0.60,  0.80 );
glm::mat2 m2i = glm::mat2( 0.80, -0.60,
     0.60,  0.80 );

glm::vec3 fbmd_9( glm::vec2 x )
{
    float f = 1.9;
    float s = 0.55;
    float a = 0.0;
    float b = 0.5;
    glm::vec2  d = glm::vec2(0.0);
    glm::mat2  m = glm::mat2(1.0,0.0,0.0,1.0);
    for( int i=0; i<9; i++ )
    {
        glm::vec3 n = Simplex::dnoise(x);
        a += b*n.x;          // accumulate values		
        d += b*m*glm::vec2(n.y, n.z);       // accumulate derivatives
        b *= s;
        x = f*m2*x;
        m = f*m2i*m;
    }

	return glm::vec3( a, d );
}

// Shadertoy
glm::vec2 smoothstepd( float a, float b, float x)
{
	if( x<a ) return glm::vec2( 0.0, 0.0 );
	if( x>b ) return glm::vec2( 1.0, 0.0 );
    float ir = 1.0/(b-a);
    x = (x-a)*ir;
    return glm::vec2( x*x*(3.0-2.0*x), 6.0*x*(1.0-x)*ir );
}

glm::vec3 terrainMapD( glm::vec2 p )
{
    glm::vec3 e = fbmd_9( p/2000.0f + glm::vec2(1.0,-2.0) );
    e.x  = 600.0*e.x + 600.0;
    e.y = 600.0*e.y;
    e.z = 600.0*e.z;

    // cliff
    glm::vec2 c = smoothstepd( 550.0, 600.0, e.x );
	e.x  = e.x  + 90.0*c.x;
    e.y = (1.0+90.0*c.y)*e.y;
    e.z = (1.0+90.0*c.y)*e.z;
    
    e.y = 2000.0f;
    e.z = 2000.0f;
    return glm::vec3(normalize( glm::vec3(-e.y,1.0,-e.z) ) );
}

void createHeightmapMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, int width, int height) {
    vertices.resize(width * height);
    
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            // Map to [-1, 1]
            float x = (float)i / (width - 1) * 2.0f - 1.0f;  
            float z = (float)j / (height - 1) * 2.0f - 1.0f; 
            
            // Generate procedural heightmap .x and normals .yzw
            // from simplex.h
            glm::vec4 dfbm = Simplex::dfBm(glm::vec3(x,1,z), 16, 1.98f, 0.5f);
            float y = dfbm.x;
            
            int index = i + width * j;
            vertices[index].position = glm::vec3(x, y, z);
            vertices[index].normal = glm::vec3(dfbm.y, dfbm.z, dfbm.w);
        }
    }
    
    indices.clear();
    for (int j = 0; j < height - 1; ++j) {
        for (int i = 0; i < width - 1; ++i) {
            int idx00 = i + width * j;
            int idx10 = (i + 1) + width * j;
            int idx01 = i + width * (j + 1);
            int idx11 = (i + 1) + width * (j + 1);
            
            // First 
            indices.push_back(idx00);
            indices.push_back(idx10);
            indices.push_back(idx01);
            
            // Second
            indices.push_back(idx10);
            indices.push_back(idx11);
            indices.push_back(idx01);
        }
    }
}

const char* vertexShaderSource = R"(
#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 450 core
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

void main() {
    vec3 norm = normalize(Normal);
    vec3 color = 0.5 * norm + 0.5;
    FragColor = vec4(color, 1.0);
}
)";

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// callback
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    
    lastX = xpos;
    lastY = ypos;
    
    camera.processMouseMovement(xoffset, yoffset);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(0, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(1, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(2, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(3, deltaTime);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "Heightmap Normal Visualization", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Configurar callbacks 
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    int width = 128;
    int height = 128;
    createHeightmapMesh(vertices, indices, width, height);

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    
    glEnable(GL_DEPTH_TEST);
    
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(window);
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glm::mat4 model = glm::mat4(1.0f);
        
        glm::mat4 view = camera.getViewMatrix();
        
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),           
            800.0f / 600.0f,               
            0.1f,                          
            100.0f                         
        );
        
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
           
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