#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Define STB Image implementation
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> 

// Macro for GLSL shader version
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Standard C++ namespace
using namespace std;

// Window title, width, and height constants
const char* const WINDOW_TITLE = "Module 6 Assignment: Lighting a 3D Scene";
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;

// Forward declaration for Camera class (assumed to be in external file)
class Camera;
extern Camera gCamera;

// Global variables for window and timing
GLFWwindow* gWindow = nullptr;
float gDeltaTime = 0.0f; // Time between current frame and last frame
float gLastFrame = 0.0f;
float gLastX = WINDOW_WIDTH / 2.0f;
float gLastY = WINDOW_HEIGHT / 2.0f;
bool gFirstMouse = true;

// Variable to toggle between perspective and orthographic projection
bool perspective = false;

// Camera setup
Camera gCamera(glm::vec3(-5.0f, 4.0f, -0.3f), glm::vec3(0.0f, 1.0f, 0.0f), 10.0f, -30.0f);

// Function to flip image vertically for texture loading
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

// ************** ENHANCEMENT: Shape Base Class **************
// Base class for all 3D shapes
class Shape {
protected:
    // Protected variables to store shape data
    vector<float> vertices;
    vector<unsigned int> indices;
    
    // Shape properties
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 color;
    string texturePath;
    glm::vec2 uvScale;
    
    // OpenGL objects
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint textureId = 0;
    
public:
    // Constructor with default values
    Shape(
        const glm::vec3& pos = glm::vec3(0.0f),
        const glm::vec3& scl = glm::vec3(1.0f),
        const glm::vec3& col = glm::vec3(1.0f),
        const string& texPath = "",
        const glm::vec2& uvScl = glm::vec2(1.0f)
    ) : position(pos), scale(scl), color(col), texturePath(texPath), uvScale(uvScl) {
        // Constructor initializes member variables
    }
    
    // Virtual destructor for proper cleanup in derived classes
    virtual ~Shape() {
        // Cleanup OpenGL resources
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
        }
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
        }
        if (ebo != 0) {
            glDeleteBuffers(1, &ebo);
        }
        if (textureId != 0) {
            glDeleteTextures(1, &textureId);
        }
    }
    
    // Pure virtual functions to be implemented by derived classes
    virtual void generateVertices() = 0;
    virtual void generateIndices() = 0;
    
    // Setup buffers for OpenGL rendering
    void setupBuffers() {
        // Create and bind Vertex Array Object (VAO)
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Create and bind Vertex Buffer Object (VBO)
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Create and bind Element Buffer Object (EBO) if indices are used
        if (!indices.empty()) {
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        }

        // Set vertex attribute pointers
        // Position attribute (3 floats)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal attribute (3 floats)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Texture coordinates attribute (2 floats)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // Unbind VAO
        glBindVertexArray(0);
    }
    
    // Load texture from file
    bool loadTexture() {
        if (texturePath.empty()) {
            return false;
        }
        
        int width, height, channels;
        unsigned char* image = stbi_load(texturePath.c_str(), &width, &height, &channels, 0);
        
        if (!image) {
            cerr << "Failed to load texture: " << texturePath << endl;
            return false;
        }
        
        // Flip the image vertically for OpenGL coordinate system
        flipImageVertically(image, width, height, channels);
        
        // Generate and bind texture
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        // Set texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Upload texture data
        if (channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        } else if (channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        } else {
            cerr << "Not implemented to handle an image with " << channels << " channels" << endl;
            stbi_image_free(image);
            return false;
        }
        
        // Generate mipmaps
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // Free image data
        stbi_image_free(image);
        
        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
        
        return true;
    }
    
    // Draw the shape with specified shader program
    void draw(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(shaderProgram);
        
        // Create model matrix (position and scale)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, scale);
        
        // Set shader uniforms
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Set color uniform
        GLint colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        glUniform3f(colorLoc, color.r, color.g, color.b);
        
        // Set UV scale uniform
        GLint uvScaleLoc = glGetUniformLocation(shaderProgram, "uvScale");
        glUniform2fv(uvScaleLoc, 1, glm::value_ptr(uvScale));
        
        // Bind texture if available
        if (textureId != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureId);
        }
        
        // Bind VAO and draw
        glBindVertexArray(vao);
        
        if (!indices.empty()) {
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 8); // 8 floats per vertex
        }
        
        // Unbind VAO
        glBindVertexArray(0);
    }
    
    // Getters and setters
    glm::vec3 getPosition() const { return position; }
    void setPosition(const glm::vec3& pos) { position = pos; }
    
    glm::vec3 getScale() const { return scale; }
    void setScale(const glm::vec3& scl) { scale = scl; }
    
    glm::vec3 getColor() const { return color; }
    void setColor(const glm::vec3& col) { color = col; }
    
    string getTexturePath() const { return texturePath; }
    void setTexturePath(const string& path) { texturePath = path; }
    
    glm::vec2 getUVScale() const { return uvScale; }
    void setUVScale(const glm::vec2& uvScl) { uvScale = uvScl; }
    
    GLuint getVAO() const { return vao; }
    GLuint getTextureId() const { return textureId; }
    unsigned int getIndicesCount() const { return indices.size(); }
};

// ************** ENHANCEMENT: Cube Class **************
// Cube shape derived from Shape base class
class Cube : public Shape {
private:
    float size; // Size of the cube

public:
    // Constructor with size parameter and other properties
    Cube(
        float cubeSize = 1.0f,
        const glm::vec3& pos = glm::vec3(0.0f),
        const glm::vec3& scl = glm::vec3(1.0f),
        const glm::vec3& col = glm::vec3(1.0f),
        const string& texPath = "",
        const glm::vec2& uvScl = glm::vec2(1.0f)
    ) : Shape(pos, scl, col, texPath, uvScl), size(cubeSize) {
        // Generate geometry (vertices and indices) for the cube
        generateVertices();
        generateIndices();
        
        // Setup VAO, VBO, and EBO
        setupBuffers();
        
        // Load texture if path is provided
        if (!texturePath.empty()) {
            loadTexture();
        }
    }

    // Generate vertices for the cube
    void generateVertices() override {
        // Clear any existing vertices
        vertices.clear();
        
        // Half size for easier calculations
        float halfSize = size / 2.0f;
        
        // Cube vertices with normals and texture coordinates (8 components per vertex)
        // Format: position (3), normal (3), texture coordinates (2)
        
        // Front face (positive Z)
        vertices.push_back(-halfSize); vertices.push_back(-halfSize); vertices.push_back(halfSize);  // Position
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);               // Normal
        vertices.push_back(0.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(-halfSize); vertices.push_back(halfSize);   // Position
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);               // Normal
        vertices.push_back(1.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(halfSize); vertices.push_back(halfSize);    // Position
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);               // Normal
        vertices.push_back(1.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        vertices.push_back(-halfSize); vertices.push_back(halfSize); vertices.push_back(halfSize);   // Position
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(1.0f);               // Normal
        vertices.push_back(0.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        // Back face (negative Z)
        vertices.push_back(-halfSize); vertices.push_back(-halfSize); vertices.push_back(-halfSize); // Position
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);              // Normal
        vertices.push_back(1.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(-halfSize); vertices.push_back(-halfSize);  // Position
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);              // Normal
        vertices.push_back(0.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(halfSize); vertices.push_back(-halfSize);   // Position
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);              // Normal
        vertices.push_back(0.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        vertices.push_back(-halfSize); vertices.push_back(halfSize); vertices.push_back(-halfSize);  // Position
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(-1.0f);              // Normal
        vertices.push_back(1.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        // Left face (negative X)
        vertices.push_back(-halfSize); vertices.push_back(-halfSize); vertices.push_back(-halfSize); // Position
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);              // Normal
        vertices.push_back(0.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(-halfSize); vertices.push_back(-halfSize); vertices.push_back(halfSize);  // Position
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);              // Normal
        vertices.push_back(1.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(-halfSize); vertices.push_back(halfSize); vertices.push_back(halfSize);   // Position
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);              // Normal
        vertices.push_back(1.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        vertices.push_back(-halfSize); vertices.push_back(halfSize); vertices.push_back(-halfSize);  // Position
        vertices.push_back(-1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);              // Normal
        vertices.push_back(0.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        // Right face (positive X)
        vertices.push_back(halfSize); vertices.push_back(-halfSize); vertices.push_back(-halfSize);  // Position
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);               // Normal
        vertices.push_back(1.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(-halfSize); vertices.push_back(halfSize);   // Position
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);               // Normal
        vertices.push_back(0.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(halfSize); vertices.push_back(halfSize);    // Position
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);               // Normal
        vertices.push_back(0.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(halfSize); vertices.push_back(-halfSize);   // Position
        vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);               // Normal
        vertices.push_back(1.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        // Bottom face (negative Y)
        vertices.push_back(-halfSize); vertices.push_back(-halfSize); vertices.push_back(-halfSize); // Position
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);              // Normal
        vertices.push_back(0.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(-halfSize); vertices.push_back(-halfSize);  // Position
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);              // Normal
        vertices.push_back(1.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(-halfSize); vertices.push_back(halfSize);   // Position
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);              // Normal
        vertices.push_back(1.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(-halfSize); vertices.push_back(-halfSize); vertices.push_back(halfSize);  // Position
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);              // Normal
        vertices.push_back(0.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        // Top face (positive Y)
        vertices.push_back(-halfSize); vertices.push_back(halfSize); vertices.push_back(-halfSize);  // Position
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);               // Normal
        vertices.push_back(0.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(halfSize); vertices.push_back(-halfSize);   // Position
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);               // Normal
        vertices.push_back(1.0f); vertices.push_back(0.0f);                                         // Texture coords
        
        vertices.push_back(halfSize); vertices.push_back(halfSize); vertices.push_back(halfSize);    // Position
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);               // Normal
        vertices.push_back(1.0f); vertices.push_back(1.0f);                                         // Texture coords
        
        vertices.push_back(-halfSize); vertices.push_back(halfSize); vertices.push_back(halfSize);   // Position
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);               // Normal
        vertices.push_back(0.0f); vertices.push_back(1.0f);                                         // Texture coords
    }

    // Generate indices for the cube
    void generateIndices() override {
        // Clear any existing indices
        indices.clear();
        
        // Define indices for each face (6 faces, 2 triangles per face, 3 indices per triangle)
        // Front face
        indices.push_back(0); indices.push_back(1); indices.push_back(2);
        indices.push_back(2); indices.push_back(3); indices.push_back(0);
        
        // Back face
        indices.push_back(4); indices.push_back(5); indices.push_back(6);
        indices.push_back(6); indices.push_back(7); indices.push_back(4);
        
        // Left face
        indices.push_back(8); indices.push_back(9); indices.push_back(10);
        indices.push_back(10); indices.push_back(11); indices.push_back(8);
        
        // Right face
        indices.push_back(12); indices.push_back(13); indices.push_back(14);
        indices.push_back(14); indices.push_back(15); indices.push_back(12);
        
        // Bottom face
        indices.push_back(16); indices.push_back(17); indices.push_back(18);
        indices.push_back(18); indices.push_back(19); indices.push_back(16);
        
        // Top face
        indices.push_back(20); indices.push_back(21); indices.push_back(22);
        indices.push_back(22); indices.push_back(23); indices.push_back(20);
    }
    
    // Getters and setters specific to Cube
    float getSize() const { return size; }
    
    // Update size and regenerate geometry if needed
    void setSize(float newSize) {
        if (size != newSize) {
            size = newSize;
            generateVertices();
            generateIndices();
            
            // Update buffers with new data
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        }
    }
};

// ************** ENHANCEMENT: Light Class **************
// Light class derived from Cube
class Light : public Cube {
private:
    glm::vec3 lightColor;
    float intensity;

public:
    // Constructor
    Light(
        const glm::vec3& pos = glm::vec3(0.0f),
        const glm::vec3& col = glm::vec3(1.0f),
        float size = 0.2f,
        float lightIntensity = 1.0f
    ) : Cube(size, pos, glm::vec3(1.0f), col), lightColor(col), intensity(lightIntensity) {
        // The base Cube constructor handles geometry creation
    }
    
    // Draw method specifically for lights, using a dedicated shader
    void draw(GLuint lightShaderProgram, const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(lightShaderProgram);
        
        // Create model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, scale);
        
        // Set shader uniforms
        GLint modelLoc = glGetUniformLocation(lightShaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(lightShaderProgram, "view");
        GLint projLoc = glGetUniformLocation(lightShaderProgram, "projection");
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Bind VAO and draw
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    // Getters and setters for light properties
    glm::vec3 getLightColor() const { return lightColor; }
    void setLightColor(const glm::vec3& color) { lightColor = color; }
    
    float getIntensity() const { return intensity; }
    void setIntensity(float value) { intensity = value; }
};

// ************** ENHANCEMENT: Scene Class **************
// Scene management class
class Scene {
private:
    // Store shapes and lights using smart pointers
    vector<shared_ptr<Shape>> shapes;
    vector<shared_ptr<Light>> lights;
    
    // Shader program IDs
    GLuint shaderProgram;
    GLuint lightShaderProgram;
    
public:
    Scene() : shaderProgram(0), lightShaderProgram(0) {}
    
    ~Scene() {
        // Cleanup shader programs
        if (shaderProgram != 0) {
            glDeleteProgram(shaderProgram);
        }
        if (lightShaderProgram != 0) {
            glDeleteProgram(lightShaderProgram);
        }
    }
    
    // Initialize the scene and create shaders
    bool initialize(const GLchar* vertexShaderSource, const GLchar* fragmentShaderSource, 
                   const GLchar* lightVertexShaderSource, const GLchar* lightFragmentShaderSource) {
        // Create shader programs
        if (!createShaderProgram(vertexShaderSource, fragmentShaderSource, shaderProgram)) {
            cerr << "Failed to create shader program for shapes" << endl;
            return false;
        }
        
        if (!createShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, lightShaderProgram)) {
            cerr << "Failed to create shader program for lights" << endl;
            return false;
        }
        
        // Set texture unit for fragment shader
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
        
        return true;
    }
    
    // Add shapes and lights to the scene
    void addShape(shared_ptr<Shape> shape) {
        shapes.push_back(shape);
    }
    
    void addLight(shared_ptr<Light> light) {
        lights.push_back(light);
    }
    
    // Render the scene
    void render(const glm::mat4& view, const glm::mat4& projection) {
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        
        // Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Get camera position from camera object
        const glm::vec3 cameraPosition = gCamera.Position;
        
        // Only proceed if we have at least one light
        if (!lights.empty()) {
            // Use shader program for shapes
            glUseProgram(shaderProgram);
            
            // Set light properties
            glm::vec3 fillerLightPos = lights[0]->getPosition();
            glm::vec3 fillerLightColor = lights[0]->getLightColor();
            
            glm::vec3 keyLightPos = (lights.size() > 1) ? lights[1]->getPosition() : glm::vec3(0.0f);
            glm::vec3 keyLightColor = (lights.size() > 1) ? lights[1]->getLightColor() : glm::vec3(0.0f);
            
            GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
            GLint lightPositionLoc = glGetUniformLocation(shaderProgram, "lightPos");
            
            GLint keyLightColorLoc = glGetUniformLocation(shaderProgram, "keyLightColor");
            GLint keyLightPositionLoc = glGetUniformLocation(shaderProgram, "keyLightPos");
            
            GLint viewPositionLoc = glGetUniformLocation(shaderProgram, "viewPosition");
            
            glUniform3f(lightColorLoc, fillerLightColor.r, fillerLightColor.g, fillerLightColor.b);
            glUniform3f(lightPositionLoc, fillerLightPos.x, fillerLightPos.y, fillerLightPos.z);
            
            glUniform3f(keyLightColorLoc, keyLightColor.r, keyLightColor.g, keyLightColor.b);
            glUniform3f(keyLightPositionLoc, keyLightPos.x, keyLightPos.y, keyLightPos.z);
            
            glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
            
            // Draw all shapes
            for (auto& shape : shapes) {
                shape->draw(shaderProgram, view, projection);
            }
            
            // Draw all lights
            for (auto& light : lights) {
                light->draw(lightShaderProgram, view, projection);
            }
        }
    }
    
    // Create a shader program
    bool createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource, GLuint& programId) {
        // Error reporting variables
        int success = 0;
        char infoLog[512];

        // Create shader program object
        programId = glCreateProgram();

        // Create vertex and fragment shader objects
        GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

        // Set shader source code
        glShaderSource(vertexShaderId, 1, &vertexShaderSource, NULL);
        glShaderSource(fragmentShaderId, 1, &fragmentShaderSource, NULL);

        // Compile the vertex shader
        glCompileShader(vertexShaderId);

        // Check for vertex shader compilation errors
        glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
            cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
            return false;
        }

        // Compile the fragment shader
        glCompileShader(fragmentShaderId);

        // Check for fragment shader compilation errors
        glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
            cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
            return false;
        }

        // Shaders compiled, attach to shader program object
        glAttachShader(programId, vertexShaderId);
        glAttachShader(programId, fragmentShaderId);

        // Link the program object
        glLinkProgram(programId);

        // Check for linking errors
        glGetProgramiv(programId, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
            cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
            return false;
        }

        // Detach and delete shader objects after linking
        glDetachShader(programId, vertexShaderId);
        glDetachShader(programId, fragmentShaderId);
        glDeleteShader(vertexShaderId);
        glDeleteShader(fragmentShaderId);

        return true;
    }
    
    // Access lights for modifying properties
    Light* getLight(int index) {
        if (index >= 0 && index < lights.size()) {
            return lights[index].get();
        }
        return nullptr;
    }
    
    int getLightCount() const { return lights.size(); }
};

// Function prototypes
bool Initialize(int, char* [], GLFWwindow** window);
void ResizeWindow(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window);
void MousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void BuildScene(Scene& scene);

// Shader source code
// Vertex shader source code for shape rendering
const GLchar* vertex_shader_source = GLSL(440,
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal;
    layout(location = 2) in vec2 textureCoordinate;

    out vec3 vertexNormal;
    out vec3 vertexFragmentPos;
    out vec2 vertexTextureCoordinate;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f);
        vertexFragmentPos = vec3(model * vec4(position, 1.0f));
        vertexNormal = mat3(transpose(inverse(model))) * normal;
        vertexTextureCoordinate = textureCoordinate;
    }
);

// Fragment shader source code for shape rendering
const GLchar* fragment_shader_source = GLSL(440,
    in vec3 vertexFragmentPos;
    in vec3 vertexNormal;
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor;

    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 keyLightColor;
    uniform vec3 lightPos;
    uniform vec3 keyLightPos;
    uniform vec3 viewPosition;

    uniform sampler2D uTexture;
    uniform vec2 uvScale;

    void main()
    {
        // Calculate Ambient
        float FillerStrength = 0.4f;
        float keyStrength = 0.1f;
        vec3 Filler = FillerStrength * lightColor;
        vec3 key = keyStrength * keyLightColor;

        // Calculate Diffuse lighting
        vec3 norm = normalize(vertexNormal);
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos);
        vec3 keyLightDirection = normalize(keyLightPos - vertexFragmentPos);

        float impact = max(dot(norm, lightDirection), 0.0);
        float keyImpact = max(dot(norm, keyLightDirection), 0.0);

        vec3 diffuse = impact * lightColor;
        vec3 keyDiffuse = keyImpact * keyLightColor;

        // Calculate Specular lighting
        float specularIntensity = 0.4f;
        float highlightSize = 16.0f;
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos);
        vec3 reflectDir = reflect(-lightDirection, norm);

        // Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;
        vec3 keySpecular = specularIntensity * specularComponent * keyLightColor;

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

        // Calculate Phong lighting result
        vec3 phong = (Filler + key + diffuse + keyDiffuse + specular) * textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
);

// Light Shader Source Code
const GLchar* lampVertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f);
    }
);

// Light Fragment Shader Source Code
const GLchar* lampFragmentShaderSource = GLSL(440,
    out vec4 fragmentColor;

    void main()
    {
        fragmentColor = vec4(1.0f);
    }
);

// Create a scene instance
Scene scene;

// Main function
int main(int argc, char* argv[])
{
    // Check if initialized correctly
    if (!Initialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Initialize the scene with shader programs
    if (!scene.initialize(vertex_shader_source, fragment_shader_source, 
                         lampVertexShaderSource, lampFragmentShaderSource)) {
        cerr << "Failed to initialize scene" << endl;
        return EXIT_FAILURE;
    }

    // Build the scene with objects
    BuildScene(scene);

    // Rendering loop
    while (!glfwWindowShouldClose(gWindow))
    {
        // Set the background color of the window
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // Process user input
        ProcessInput(gWindow);

        // Create view matrix from camera
        glm::mat4 view = gCamera.GetViewMatrix();

        // Create perspective or orthographic projection matrix
        glm::mat4 projection;
        if (!perspective)
        {
            // Perspective projection (default)
            projection = glm::perspective(glm::radians(gCamera.Zoom), 
                                         (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 
                                          0.1f, 100.0f);
        }
        else
        {
            // Orthographic projection
            projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
        }

        // Render the scene
        scene.render(view, projection);

        // Swap front and back buffers
        glfwSwapBuffers(gWindow);
        glfwPollEvents();
    }

    // Exit with success
    exit(EXIT_SUCCESS);
}

// Build the scene with objects
void BuildScene(Scene& scene)
{
    // Create a cube with texture
    auto cube1 = make_shared<Cube>(
        1.0f,                           // Size
        glm::vec3(0.0f, 0.0f, 0.0f),    // Position
        glm::vec3(1.0f, 1.0f, 1.0f),    // Scale
        glm::vec3(1.0f, 1.0f, 1.0f),    // Color
        "textures/wood.jpg",            // Texture path (adjust as needed)
        glm::vec2(1.0f, 1.0f)           // UV scale
    );
    
    // Create another cube with different size and position
    auto cube2 = make_shared<Cube>(
        1.5f,                           // Size
        glm::vec3(2.5f, 0.0f, 0.0f),    // Position
        glm::vec3(1.0f, 1.0f, 1.0f),    // Scale
        glm::vec3(1.0f, 1.0f, 1.0f),    // Color
        "textures/brick.jpg",           // Texture path (adjust as needed)
        glm::vec2(1.0f, 1.0f)           // UV scale
    );
    
    // Add cubes to the scene
    scene.addShape(cube1);
    scene.addShape(cube2);
    
    // Create lights
    auto fillerLight = make_shared<Light>(
        glm::vec3(2.5f, 5.0f, -0.8f),   // Position
        glm::vec3(1.0f, 1.0f, 1.0f),    // Color (white)
        0.2f,                           // Size
        0.8f                            // Intensity
    );
    
    auto keyLight = make_shared<Light>(
        glm::vec3(-1.5f, 4.0f, -3.6f),  // Position
        glm::vec3(0.0f, 0.0f, 1.0f),    // Color (blue)
        0.1f,                           // Size
        0.5f                            // Intensity
    );
    
    // Add lights to the scene
    scene.addLight(fillerLight);
    scene.addLight(keyLight);
}

bool Initialize(int argc, char* argv[], GLFWwindow** window)
{
    // Initialize GLFW, GLEW, and create the window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the window using constants for parameters
    *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);

    // Check if window creation failed
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, ResizeWindow);
    glfwSetCursorPosCallback(*window, MousePositionCallback);
    glfwSetScrollCallback(*window, MouseScrollCallback);
    glfwSetMouseButtonCallback(*window, MouseButtonCallback);
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    return true;
}

// Process user input and window changes
void ProcessInput(GLFWwindow* window)
{
    // Exit the program
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Toggle between wireframe and filled shapes
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Move the camera
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);

    // Get filler light (index 0)
    Light* fillerLight = scene.getLight(0);
    if (fillerLight) {
        // Modify filler light position
        glm::vec3 pos = fillerLight->getPosition();
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            pos.x -= 0.05f;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
            pos.x += 0.05f;
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
            pos.z -= 0.05f;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
            pos.z += 0.05f;
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
            pos.y -= 0.05f;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
            pos.y += 0.05f;
        fillerLight->setPosition(pos);
    }

    // Get key light (index 1)
    Light* keyLight = scene.getLight(1);
    if (keyLight) {
        // Modify light color
        glm::vec3 color = keyLight->getLightColor();
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            color.r += 0.001f;
            if (color.r > 1.0f) {
                color.r = 0.0f;
            }
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            color.g += 0.001f;
            if (color.g > 1.0f) {
                color.g = 0.0f;
            }
        }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        {
            color.b += 0.001f;
            if (color.b > 1.0f) {
                color.b = 0.0f;
            }
        }
        keyLight->setLightColor(color);

        // Turn off key light
        if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
        {
            keyLight->setLightColor(glm::vec3(0.0f, 0.0f, 0.0f));
        }

        // Reset key light to green
        if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
        {
            keyLight->setLightColor(glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }

    // Toggle perspective versus orthographic view 
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
        perspective = false;
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
        perspective = true;
}

void ResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void MousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // Reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Adjust the camera speed based on mouse scroll
    gCamera.ProcessMouseScroll(yoffset);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // Handle mouse button events
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left Mouse Button Pressed" << endl;
        else
            cout << "Left Mouse Button Released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle Mouse Button Pressed" << endl;
        else
            cout << "Middle Mouse Button Released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right Mouse Button Pressed" << endl;
        else
            cout << "Right Mouse Button Released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}