#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <functional>
#include <unordered_map>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp>


void GLAPIENTRY
MessageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);
}

float inputRight = 0;
float inputForward = 0;
float inputJump = 0;
float inputSpeed = 0;

static void kbCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //if (key == GLFW_KEY_E && action == GLFW_PRESS)
    //    std::cout << "asd" << std::endl;

    if (key == GLFW_KEY_A)
    {
        if (action == GLFW_PRESS)
        {
            inputRight += -1;
        }
        else if(action == GLFW_RELEASE)
        {
            inputRight -= -1;
        }
    }
    else if (key == GLFW_KEY_D)
    {
        if (action == GLFW_PRESS)
        {
            inputRight += 1;
        }
        else if (action == GLFW_RELEASE)
        {
            inputRight -= 1;
        }
    }
    else if (key == GLFW_KEY_W)
    {
        if (action == GLFW_PRESS)
        {
            inputForward += 1;
        }
        else if (action == GLFW_RELEASE)
        {
            inputForward -= 1;
        }
    }
    else if (key == GLFW_KEY_S)
    {
        if (action == GLFW_PRESS)
        {
            inputForward += -1;
        }
        else if (action == GLFW_RELEASE)
        {
            inputForward -= -1;
        }
    }
    else if (key == GLFW_KEY_SPACE)
    {
        if (action == GLFW_PRESS)
        {
            inputJump += 1;
        }
        else if (action == GLFW_RELEASE)
        {
            inputJump -= 1;
        }
    }
    else if (key == GLFW_KEY_C)
    {
        if (action == GLFW_PRESS)
        {
            inputJump += -1;
        }
        else if (action == GLFW_RELEASE)
        {
            inputJump -= -1;
        }
    }
    else if (key == GLFW_KEY_LEFT_SHIFT)
    {
        if (action == GLFW_PRESS)
        {
            inputSpeed += 1;
        }
        else if (action == GLFW_RELEASE)
        {
            inputSpeed -= 1;
        }
    }
    else if (key == GLFW_KEY_LEFT_CONTROL)
    {
        if (action == GLFW_PRESS)
        {
            inputSpeed += -1;
        }
        else if (action == GLFW_RELEASE)
        {
            inputSpeed -= -1;
        }
    }
}

static std::string readFile(const std::string& filepath)
{
    std::ifstream inFile;
    inFile.open(filepath); //open the input file

    std::stringstream strStream;
    strStream << inFile.rdbuf(); //read the file

    return strStream.str(); //str holds the content of the file
}

static unsigned int compileShader(unsigned int type, const std::string& source)
{
    unsigned id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = new char[length];
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile " <<
            (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        delete[] message;
        return 0;
    }

    return id;
}

static unsigned int createShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

static float x = 0;
static float y = 0;

static double oldX;
static double oldY;

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    double deltaX = oldX - xpos;
    double deltaY = oldY - ypos;

    x -= deltaY / 500;
    y += deltaX / 500;

    x = std::fmin(3.1/2, std::fmax(-3.1/2, x ));
    oldX = xpos;
    oldY = ypos;
}

static float xpos;
static float ypos;
static float zpos;

static float speed = 0.005;
static float shiftMultiplier = 5.5;
static float ctrlMultiplier = 0.2;

static glm::vec3 pos = glm::vec3(0, 0, 3);

glm::mat4 viewMatrix(glm::vec3 eye, glm::vec3 center, glm::vec3 up) {
    glm::vec3 f = glm::normalize(center - eye);
    glm::vec3 s = glm::normalize(cross(f, up));
    glm::vec3 u = glm::cross(s, f);
    return glm::mat4(
        glm::vec4( s,   0.0),
        glm::vec4( u,   0.0),
        glm::vec4(-f,   0.0),
        glm::vec4( 0.0, 0.0, 0.0, 1)
    );
}

glm::mat3 yAxisRotationMatrix(float theta)
{
    return glm::mat3(
         glm::cos(theta), 0, glm::sin(theta),
         0              , 1, 0              ,
        -glm::sin(theta), 0, glm::cos(theta)
    );
}

glm::mat3 xAxisRotationMatrix(float theta)
{
    return glm::mat3(
        1, 0              ,  0              ,
        0, glm::cos(theta), -glm::sin(theta),
        0, glm::sin(theta),  glm::cos(theta)
    );
}

int main(void)
{
    GLFWwindow* window;
    /* Initialize the library */
    if (!glfwInit())
        return -1;
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1920, 1080, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
        return -1;

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    std::cout << glGetString(GL_VERSION) << std::endl;

    // quad that covers whole screen
    float positions[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };

    unsigned int indices[] = {
        0,1,3,
        3,1,2
    };

    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    unsigned int ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    std::string vertexShader = readFile("res/shaders/BasicVertex.shader");
    std::string fragmentShader = readFile("res/shaders/BasicFragment.shader");

    unsigned int shader = createShader(vertexShader, fragmentShader);
    glUseProgram(shader);

    int u_WindowSize = glGetUniformLocation(shader, "windowSize");
    glUniform2f(u_WindowSize, 1920, 1080);
    int u_Time = glGetUniformLocation(shader, "time");
    int u_ViewToWorld = glGetUniformLocation(shader, "viewToWorld");
    int u_Pos = glGetUniformLocation(shader, "pos");

    float t = 0;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, kbCallback);

    /* Loop until the user closes the window */

    while (!glfwWindowShouldClose(window))
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        t += 0.01;

        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        glm::vec3 cameraForward = glm::vec3(0.0, 0.0, 1.0);
        glm::vec3 cameraTarget = yAxisRotationMatrix(y) * (xAxisRotationMatrix(x) * cameraForward);
        glm::vec3 cameraUp = glm::vec3(0.0, 1.0, 0.0);

        //right
        glm::vec3 right = normalize(glm::cross(cameraTarget, cameraUp));

        glm::mat3 rfuMatrix = glm::mat3(right, -cameraTarget, cameraUp);
        glm::vec3 multipliers = glm::vec3(inputRight, inputForward, inputJump);

        pos += rfuMatrix * multipliers * speed * (inputSpeed == 1 ? shiftMultiplier : (inputSpeed == -1 ? ctrlMultiplier : 1));

        glm::mat4 viewToWorld = viewMatrix(pos, pos + cameraTarget, cameraUp);
        //glm::mat4 viewToWorld = viewMatrix(pos, glm::vec3(0, 5, 10), cameraUp);

        glUniformMatrix4fv(u_ViewToWorld, 1, GL_FALSE, glm::value_ptr(viewToWorld));
        glUniform3f(u_Pos, pos.x, pos.y, pos.z);
        glUniform1f(u_Time, t);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}