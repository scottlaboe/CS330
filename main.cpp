#include <iostream>             // cout, cerr
#include <cstdlib>              // EXIT_FAILURE
#include <GL/glew.h>            // GLEW library
#include <GLFW/glfw3.h>         // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <stdlib.h>
#include <common/objloader.hpp>

#include <learnOpengl/camera.h> // Camera class

#include <string>

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Tutorial 4.5"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    const int NUM_LIGHT_SOURCES = 2;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;     // Handles for the vertex buffer objects
        GLuint uvBuffer;
        GLuint normalBuffer;
        GLuint nVertices;
        GLuint lengthOfObjects[5];
    };

    enum {
        PERSPECTIVE_DISPLAY,
        ORTHOGRAPHIC_DISPLAY
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;

    // Shader programs
    GLuint gCubeProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 7.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    int perspective = ORTHOGRAPHIC_DISPLAY;
    bool keypressed;

    // Texture
    GLuint gTextureId[5];
    glm::vec2 gUVScale(5.0f, 5.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Cube and light color
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);

    //light sources
    struct lightSource {
        glm::vec3 gLightPosition;// (1.5f, 0.5f, 3.0f);
        glm::vec3 gLightScale;//(0.3f);
        glm::vec3 gLightColor;// (1.0f, 1.0f, 1.0f);
        GLfloat ambientStrength;
        GLfloat specularIntensity;
        GLfloat diffuseStrength;
    };
    lightSource lightSources[NUM_LIGHT_SOURCES];

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);


/* Cube Vertex Shader Source Code*/
const GLchar* cubeVertexShaderSource = GLSL(440,

layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Cube Fragment Shader Source Code*/
const GLchar* cubeFragmentShaderSource = GLSL(440,

in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

struct lightSource {
    vec3 lightColor;
    vec3 lightPos;
    float ambientStrength;
    float specularIntensity;
    float diffuseStrength;
};

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform lightSource lightSources[2];
uniform vec3 viewPosition;
uniform sampler2D uTexture[5]; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    for (int light = 0; light < 2; ++light) {
        //Phong lighting model calculations to generate ambient, diffuse, and specular components

        //Calculate Ambient lighting
        vec3 ambient = lightSources[light].ambientStrength * lightSources[light].lightColor; // Generate ambient light color

        //Calculate Diffuse lighting
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightSources[light].lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = lightSources[light].diffuseStrength * (impact * lightSources[light].lightColor); // Generate diffuse light color

        //Calculate Specular lighting
        float highlightSize = 16.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = lightSources[light].specularIntensity * specularComponent * lightSources[light].lightColor;

        vec4 textureColor;

        // Texture holds the color to be used for all three components
        for (int i = 0; i < 5; ++i) {
            textureColor = texture(uTexture[i], vertexTextureCoordinate);
        }

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        fragmentColor += vec4(phong, 1.0); // Send lighting results to GPU
    }
}
);


// Lamp Shader Source Code
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

        //Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


// Fragment Shader Source Code
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
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

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader programs
    if (!UCreateShaderProgram(cubeVertexShaderSource, cubeFragmentShaderSource, gCubeProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // tell opengl for each sampler to which texture unit it belongs to
    glUseProgram(gCubeProgramId);

    //set light colors
    lightSources[0].gLightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightSources[0].gLightPosition = glm::vec3(1.5f, 0.5f, 3.0f);
    lightSources[0].gLightScale = glm::vec3(0.3f);
    lightSources[0].ambientStrength = 0.1f;
    lightSources[0].specularIntensity = 0.8f;
    lightSources[0].diffuseStrength = 1.0f;

    //rotate light 180 degrees around object with respect to first source
    glm::vec4 newPosition = glm::rotate(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightSources[0].gLightPosition, 1.0f);
    lightSources[1].gLightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightSources[1].gLightPosition = glm::vec3(newPosition.x, newPosition.y, newPosition.z);
    lightSources[1].gLightScale = glm::vec3(0.3f);
    lightSources[1].ambientStrength = 0.05f;
    lightSources[1].specularIntensity = 0.08f;
    lightSources[1].diffuseStrength = 0.1f;

    // Load texture
    const char* texFilename = "textures/steel.png";
    if (!UCreateTexture(texFilename, gTextureId[0]))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // Load texture
    texFilename = "textures/table.png";
    if (!UCreateTexture(texFilename, gTextureId[1]))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "textures/glass.png";
    if (!UCreateTexture(texFilename, gTextureId[2]))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "textures/pencil.png";
    if (!UCreateTexture(texFilename, gTextureId[3]))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    texFilename = "textures/wood.png";
    if (!UCreateTexture(texFilename, gTextureId[4]))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release shader program
    UDestroyShaderProgram(gCubeProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}

// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        keypressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        if (keypressed) {
            keypressed = false;
            if (perspective == ORTHOGRAPHIC_DISPLAY) {
                perspective = PERSPECTIVE_DISPLAY;
            }
            else {
                perspective = ORTHOGRAPHIC_DISPLAY;
            }
        }
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && gTexWrapMode != GL_REPEAT)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureId[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_REPEAT;

        cout << "Current Texture Wrapping Mode: REPEAT" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && gTexWrapMode != GL_MIRRORED_REPEAT)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureId[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_MIRRORED_REPEAT;

        cout << "Current Texture Wrapping Mode: MIRRORED REPEAT" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && gTexWrapMode != GL_CLAMP_TO_EDGE)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureId[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_CLAMP_TO_EDGE;

        cout << "Current Texture Wrapping Mode: CLAMP TO EDGE" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && gTexWrapMode != GL_CLAMP_TO_BORDER)
    {
        float color[] = { 1.0f, 0.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

        glBindTexture(GL_TEXTURE_2D, gTextureId[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_CLAMP_TO_BORDER;

        cout << "Current Texture Wrapping Mode: CLAMP TO BORDER" << endl;
    }
        
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Function called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();
    glm::mat4 projection;

    // Creates a perspective projection
    switch (perspective) {
        case PERSPECTIVE_DISPLAY:
            projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.01f, 100.0f);
            break;
        case ORTHOGRAPHIC_DISPLAY:
            //calculate current position from perspective
            float halfY = glm::radians(gCamera.Zoom) / 2.0f * 3.1415926f / 180.0f;
            float legOne = sqrt(gCamera.Position.x * gCamera.Position.x + gCamera.Position.y * gCamera.Position.y);
            float focusPlane = sqrt(legOne * legOne + gCamera.Position.z * gCamera.Position.z);
            float top = focusPlane/2; //focus_plane is the distance from the camera
            float right = top * (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT;
            projection = glm::ortho(-right, right, -top, top, 0.01f, 100.0f);
            break;
    }

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gCubeProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gCubeProgramId, "view");
    GLint projLoc = glGetUniformLocation(gCubeProgramId, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    // 2. Rotates shape by 15 degrees in the x axis
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(1.0, 0.0f, 0.0f));

    glm::vec3 location = glm::vec3(0, 0, 0);
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(location);
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    for (int light = 0; light < NUM_LIGHT_SOURCES; ++light) {
        // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
        glUniform3f(glGetUniformLocation(gCubeProgramId, ("lightSources[" + std::to_string(light) + "].lightColor").c_str()), lightSources[light].gLightColor.r, lightSources[light].gLightColor.g, lightSources[light].gLightColor.b);
        glUniform3f(glGetUniformLocation(gCubeProgramId, ("lightSources[" + std::to_string(light) + "].lightPos").c_str()), lightSources[light].gLightPosition.x, lightSources[light].gLightPosition.y, lightSources[light].gLightPosition.z);
        glUniform1f(glGetUniformLocation(gCubeProgramId, ("lightSources[" + std::to_string(light) + "].ambientStrength").c_str()), lightSources[light].ambientStrength);
        glUniform1f(glGetUniformLocation(gCubeProgramId, ("lightSources[" + std::to_string(light) + "].specularIntensity").c_str()), lightSources[light].specularIntensity);
        glUniform1f(glGetUniformLocation(gCubeProgramId, ("lightSources[" + std::to_string(light) + "].diffuseStrength").c_str()), lightSources[light].diffuseStrength);
    }

    GLint objectColorLoc = glGetUniformLocation(gCubeProgramId, "objectColor");
    GLint viewPositionLoc = glGetUniformLocation(gCubeProgramId, "viewPosition");

    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);

    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gCubeProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    glUseProgram(gCubeProgramId);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId[0]);

    glDrawArrays(GL_TRIANGLES, 0, gMesh.lengthOfObjects[0]);

    // bind textures on corresponding texture units
    glBindTexture(GL_TEXTURE_2D, gTextureId[1]);

    for (int object = 1; object < 5; ++object) {
        glBindTexture(GL_TEXTURE_2D, gTextureId[object]);
        // Draws second object
        glDrawArrays(GL_TRIANGLES, gMesh.lengthOfObjects[object-1], gMesh.lengthOfObjects[object]);
    }

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals; 

    loadOBJ("lighter.obj", vertices, uvs, normals);
    mesh.lengthOfObjects[0] = vertices.size();
    loadOBJ("plane.obj", vertices, uvs, normals);
    mesh.lengthOfObjects[1] = vertices.size() - mesh.lengthOfObjects[0];
    loadOBJ("glass.obj", vertices, uvs, normals);
    mesh.lengthOfObjects[2] = vertices.size() - mesh.lengthOfObjects[1];
    loadOBJ("pencil.obj", vertices, uvs, normals);
    mesh.lengthOfObjects[3] = vertices.size() - mesh.lengthOfObjects[2];
    loadOBJ("wood.obj", vertices, uvs, normals);
    mesh.lengthOfObjects[4] = vertices.size() - mesh.lengthOfObjects[3];

    const GLuint normalsPerVertex = 3;
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = vertices.size();

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    //Create UV VBO
    glGenBuffers(1, &mesh.uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.uvBuffer); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

    //Create normal VBO
    glGenBuffers(1, &mesh.normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.normalBuffer); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

    //Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

    // Strides between vertex coordinates
    GLint stride = 0;

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    // Create Normal Attribute Pointers
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.normalBuffer);
    glVertexAttribPointer(1, normalsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);

    // Create UV Attribute Pointers
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.uvBuffer);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, 0);
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.uvBuffer);
    UDestroyTexture(gTextureId[0]);
    UDestroyTexture(gTextureId[1]);
    UDestroyTexture(gTextureId[2]);
    UDestroyTexture(gTextureId[3]);
    UDestroyTexture(gTextureId[4]);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        //flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
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


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
