
#include "MeshDefs.h"
#include "Mesh.h"
#include "BuildingMesh.h"


// Include STB Image library and define implementation
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

// Structure to hold a mesh for ambient and key lighting
struct GLightMesh
{
	GLuint vao;         // Vertex array object handle
	GLuint vbo;         // Vertex buffer object handle
	GLuint nVertices;   // Number of vertices in the mesh
};

GLightMesh ambLightMesh;
GLightMesh keyLightMesh;
Shapes builder;

// Main GLFW window
GLFWwindow* gWindow = nullptr;

// Shader program IDs
GLuint gShaderProgram;
GLuint gLightProgramId;

// Vector to store scene shapes
vector<GLMesh> scene;

// Variable to toggle between perspective and orthographic projection
bool perspective = false;

// Camera setup
Camera gCamera(glm::vec3(-5.0f, 4.0f, -0.3f), glm::vec3(0.0f, 1.0f, 0.0f), 10.0f, -30.0f);
float gLastX = WINDOW_WIDTH / 2.0f;
float gLastY = WINDOW_HEIGHT / 2.0f;
bool gFirstMouse = true;

// Timing variables
float gDeltaTime = 0.0f; // Time between current frame and last frame
float gLastFrame = 0.0f;

// Light properties (color, position, scale) for the filler light
glm::vec3 gFillerLightColor(1.0f, 1.0f, 1.0f);
glm::vec3 gFillerLightPosition(2.5f, 5.0f, -0.8f);
glm::vec3 gFillerLightScale(0.7f);

//Light properties (color, position, scale) for the key light
glm::vec3 gKeyLightColor(0.0f, 0.0f, 1.0f);
glm::vec3 gKeyLightPosition(-1.5f, 4.0f, -3.6);
glm::vec3 gKeyLightScale(0.1f);

// Function prototypes
bool Initialize(int, char* [], GLFWwindow** window);
void ResizeWindow(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window);
void Render(vector<GLMesh> scene);
bool CreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void DestroyMesh(GLMesh& mesh);
void DestroyShaderProgram(GLuint programId);
void DestroyTexture(GLuint textureId);
void Plane(GLMesh& mesh, vector<float> properties);
void MousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
bool CreateTexture(const char* filename, GLuint& textureId);
void CreateLightMesh(GLightMesh& lightMesh);

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
in vec2 vertexTextureCoordinate; // Texture coordinates

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
	layout(location = 0) in vec3 position; // Vertex attribute position

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transform vertices into clip coordinates
}
);

// Light Fragment Shader Source Code
const GLchar* lampFragmentShaderSource = GLSL(440,
	out vec4 fragmentColor; // Outgoing light color to the GPU

void main()
{
	fragmentColor = vec4(1.0f); // Set color to white (1.0f, 1.0f, 1.0f) with alpha 1.0
}
);

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

// Main function
int main(int argc, char* argv[])
{
	// Check if initialized correctly
	if (!Initialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Build the environment scene
	BuildingMesh::Environment(scene);

	// Build shaders and load textures for each mesh in the scene
	for (auto& m : scene)
	{
		if (!CreateTexture(m.texFilename, m.textureId))
		{
			cout << "Failed to load texture " << m.texFilename << endl;
			return EXIT_FAILURE;
		}

		if (!CreateShaderProgram(vertex_shader_source, fragment_shader_source, gShaderProgram))
			return EXIT_FAILURE;
	}

	// Create shader program for the light object
	if (!CreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLightProgramId))
		return EXIT_FAILURE;

	// Create Light Objects
	CreateLightMesh(ambLightMesh);
	CreateLightMesh(keyLightMesh);

	// Tell OpenGL for each sampler to which texture unit it belongs (only has to be done once)
	glUseProgram(gShaderProgram);
	// Set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gShaderProgram, "uTexture"), 0);

	// Rendering loop
	while (!glfwWindowShouldClose(gWindow))
	{
		// Set the background color of the window
		glClearColor(1.0f, 0.5f, 0.0f, 1.0f);

		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// Process user input
		ProcessInput(gWindow);

		// Render the scene
		Render(scene);

		glfwPollEvents();
	}

	// Clean up
	for (auto& m : scene)
	{
		DestroyMesh(m);
	}

	scene.clear();

	DestroyShaderProgram(gShaderProgram);
	DestroyShaderProgram(gLightProgramId);

	// Exit with success
	exit(EXIT_SUCCESS);
}

// Function Definitions
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

bool CreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
	// Error reporting variables
	int success = 0;
	char infoLog[512];

	// Create shader program object
	programId = glCreateProgram();

	// Create vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Set shader source code
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader
	glCompileShader(vertexShaderId);

	// Check for vertex shader compilation errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		return false;
	}

	// Compile the fragment shader
	glCompileShader(fragmentShaderId);

	// Check for fragment shader compilation errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		return false;
	}

	// Shaders compiled, attach to shader program object from above
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	// Link the program object
	glLinkProgram(programId);

	// Check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);

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

	// Modify light position
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		gFillerLightPosition.x -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		gFillerLightPosition.x += 0.05f;
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
		gFillerLightPosition.z -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		gFillerLightPosition.z += 0.05f;
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
		gFillerLightPosition.y -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		gFillerLightPosition.y += 0.05f;

	// Modify light color 
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		gKeyLightColor.r += 0.001f;
		if (gKeyLightColor.r > 1.0f)
		{
			gKeyLightColor.r = 0.0f;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
	{
		gKeyLightColor.g += 0.001f;
		if (gKeyLightColor.g > 1.0f)
		{
			gKeyLightColor.g = 0.0f;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
	{
		gKeyLightColor.b += 0.001f;
		if (gKeyLightColor.b > 1.0f)
		{
			gKeyLightColor.b = 0.0f;
		}
	}

	 //Turn off key light [ ]
	if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
	{
		gKeyLightColor.r = 0.0f;
		gKeyLightColor.g = 0.0f;
		gKeyLightColor.b = 0.0f;

	}

	 //Turn off key light
	if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
	{
		gKeyLightColor.r = 0.0f;
		gKeyLightColor.g = 1.0f;
		gKeyLightColor.b = 0.0f;

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

void Render(vector<GLMesh> scene)
{

	// Enable z-depth testing
	glEnable(GL_DEPTH_TEST);

	// Clear the background (black)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Transform the camera (x, y, z)
	glm::mat4 view = gCamera.GetViewMatrix();

	// Create perspective or orthographic projection matrix
	glm::mat4 projection;
	if (!perspective)
	{
		// Perspective projection (default)
		projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else
	{
		// Orthographic projection
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	}

	// Loop to draw each shape individually
	for (auto i = 0; i < scene.size(); ++i)
	{
		auto mesh = scene[i];

		// Activate vertex buffer objects within the mesh's vertex array object
		glBindVertexArray(mesh.vao);

		// Use the shape shader program
		glUseProgram(gShaderProgram);

		// Get and pass transform matrices to the shader program for shapes
		GLint modelLocation = glGetUniformLocation(gShaderProgram, "model");
		GLint viewLocation = glGetUniformLocation(gShaderProgram, "view");
		GLint projLocation = glGetUniformLocation(gShaderProgram, "projection");

		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(mesh.model));
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(projection));

		GLint objectColorLoc = glGetUniformLocation(gShaderProgram, "objectColor");

		// Filler Light
		GLint lightColorLoc = glGetUniformLocation(gShaderProgram, "lightColor");
		GLint lightPositionLoc = glGetUniformLocation(gShaderProgram, "lightPos");

		// Key light
		GLint keyLightColorLoc = glGetUniformLocation(gShaderProgram, "keyLightColor");
		GLint keyLightPositionLoc = glGetUniformLocation(gShaderProgram, "keyLightPos");

		// Camera view
		GLint viewPositionLoc = glGetUniformLocation(gShaderProgram, "viewPosition");

		// Pass color, light, and camera data to the shape shader
		glUniform3f(objectColorLoc, mesh.p[0], mesh.p[1], mesh.p[2]);

		// Key Light
		glUniform3f(keyLightColorLoc, gKeyLightColor.r, gKeyLightColor.g, gKeyLightColor.b);
		glUniform3f(keyLightPositionLoc, gKeyLightPosition.x, gKeyLightPosition.y, gKeyLightPosition.z);

		// Filler Light
		glUniform3f(lightColorLoc, gFillerLightColor.r, gFillerLightColor.g, gFillerLightColor.b);
		glUniform3f(lightPositionLoc, gFillerLightPosition.x, gFillerLightPosition.y, gFillerLightPosition.z);

		const glm::vec3 cameraPosition = gCamera.Position;
		glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

		GLint UVScaleLoc = glGetUniformLocation(gShaderProgram, "uvScale");
		glUniform2fv(UVScaleLoc, 1, glm::value_ptr(mesh.gUVScale));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mesh.textureId);

		// Draw the shape
		glDrawArrays(GL_TRIANGLES, 0, mesh.nIndices);
	}

	// Draw the Filler Light
	glUseProgram(gLightProgramId);
	glBindVertexArray(ambLightMesh.vao);

	// Light location and scale
	glm::mat4 model = glm::translate(gFillerLightPosition) * glm::scale(gFillerLightScale);

	// Matrix uniforms from the Light Shader program
	GLint modelLoc = glGetUniformLocation(gLightProgramId, "model");
	GLint viewLoc = glGetUniformLocation(gLightProgramId, "view");
	GLint projLoc = glGetUniformLocation(gLightProgramId, "projection");

	// Matrix data
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Draw the light
	glDrawArrays(GL_TRIANGLES, 0, ambLightMesh.nVertices);

	// Draw the Key Light
	glUseProgram(gLightProgramId);
	glBindVertexArray(keyLightMesh.vao);

	// Light location and Scale
	model = glm::translate(gKeyLightPosition) * glm::scale(gKeyLightScale);

	// Matrix uniforms from the Light Shader program
	modelLoc = glGetUniformLocation(gLightProgramId, "model");
	viewLoc = glGetUniformLocation(gLightProgramId, "view");
	projLoc = glGetUniformLocation(gLightProgramId, "projection");

	// Matrix data
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Draw the light
	glDrawArrays(GL_TRIANGLES, 0, keyLightMesh.nVertices);

	// Deactivate vertex array objects
	glBindVertexArray(0);
	glUseProgram(0);

	// Swap front and back buffers
	glfwSwapBuffers(gWindow);
}

// Clean-up methods
void DestroyMesh(GLMesh& mesh)
{
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(1, &mesh.vbo);
}

void DestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}

bool CreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// Set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// Set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle an image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}

void DestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}

// Template for creating a cube light
void CreateLightMesh(GLightMesh& lightMesh)
{
	// Position and Colors/Textures
	GLfloat verts[] = {
		//Positions          
		//BF          //Negative Z				  //Textures
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

	   //FF          //Positive Z		          //Textures
	   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
	   -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
	   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

	   //LF         //Negative X		           //Textures
	   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
	   -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
	   -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	   -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	   -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

	   //RF         //Positive X		          //Textures
	   0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
	   0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
	   0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	   0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	   0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	   0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

	   //BF        //Negative Y		          //Textures
	   -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	   -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

	   //TF          //Positive Y		           //Textures
	   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
	   -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	lightMesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &lightMesh.vao);
	glBindVertexArray(lightMesh.vao);

	// Creates two buffers
	glGenBuffers(1, &lightMesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, lightMesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	// Strides between vertex coordinates 
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	// Create vertexs
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}