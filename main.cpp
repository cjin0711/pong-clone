#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include <cmath>
#include <ctime>

enum AppStatus { RUNNING, TERMINATED };

constexpr float WINDOW_SIZE_MULT = 1.5f;

constexpr int WINDOW_WIDTH = 640 * WINDOW_SIZE_MULT,
WINDOW_HEIGHT = 480 * WINDOW_SIZE_MULT;

constexpr float BG_RED = 0.1f,
BG_BLUE = 0.1f,
BG_GREEN = 0.1f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr GLint NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

// ALL SPRITES
constexpr char PLAYERONE_SPRITE_FILEPATH[] = "playerOne.png";
constexpr char PLAYERTWO_SPRITE_FILEPATH[] = "playerTwo.png";
constexpr char BALL_SPRITE_FILEPATH[] = "ball.png";

// SPRITE MOVEMENT SPEEDS
constexpr float PADDLE_SPEED = 2.5f;
constexpr float BALL_SPEED = 1.7f;

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program;
float g_previous_ticks = 0.0f;

// Matrix initialization
glm::mat4 g_view_matrix, g_projection_matrix;
glm::mat4 g_playerOne_matrix;
glm::mat4 g_playerTwo_matrix;
glm::mat4 g_ball_matrix;

// Initial Positions
constexpr glm::vec3 INIT_POS_PLAYERONE = glm::vec3(-4.5f, 0.0f, 0.0f),
INIT_POS_PLAYERTWO = glm::vec3(4.5f, 0.0f, 0.0f),
INIT_POS_BALL = glm::vec3(0.0f, 0.0f, 0.0f),
INIT_SCALE_PLAYERONE = glm::vec3(1.0f, 1.0f, 0.0f),
INIT_SCALE_PLAYERTWO = glm::vec3(1.0f, 1.0f, 0.0f),
INIT_SCALE_BALL = glm::vec3(1.0f, 1.0f, 0.0f);

// Toggle Mode Variables
bool singleMode = false;
float botDirection = 1.0f;

GLuint g_playerOne_texture_id;
GLuint g_playerTwo_texture_id;
GLuint g_ball_texture_id;

// Initial Position & Movement Matrices
glm::vec3 g_playerOne_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_playerOne_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_playerTwo_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_playerTwo_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(BALL_SPEED, BALL_SPEED, 0.0f);

void initialise();
void process_input();
void update();
void render();
void shutdown();

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Pong Clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr) shutdown();

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_playerOne_position = INIT_POS_PLAYERONE;
    g_playerTwo_position = INIT_POS_PLAYERTWO;
    g_ball_position = INIT_POS_BALL;

    g_playerOne_matrix = glm::mat4(1.0f);
    g_playerTwo_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_playerOne_texture_id = load_texture(PLAYERONE_SPRITE_FILEPATH);
    g_playerTwo_texture_id = load_texture(PLAYERTWO_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

       // ADDED FROM PREV TEMPLATE
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_q: g_app_status = TERMINATED; break;

            // trigger event for single mode
            case SDLK_t: singleMode = !singleMode;

            default: break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);


    // Player One Movement
    if (key_state[SDL_SCANCODE_W])
    {
        g_playerOne_movement.y = PADDLE_SPEED;
    }
    else if (key_state[SDL_SCANCODE_S])
    {
        g_playerOne_movement.y = -PADDLE_SPEED;
    }
    else {
        g_playerOne_movement.y = 0;
    }

    // Player Two Movement (dependent on t toggle)
    if (!singleMode)
    {
        if (key_state[SDL_SCANCODE_UP])
        {
            g_playerTwo_movement.y = PADDLE_SPEED;
        }
        else if (key_state[SDL_SCANCODE_DOWN])
        {
            g_playerTwo_movement.y = -PADDLE_SPEED;
        }
        else
        {
            g_playerTwo_movement.y = 0;
        }
    }
}

void update()
{
    // --- DELTA TIME CALCULATIONS --- //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float g_delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;


    // --- TRANSLATION --- //
   
    // Update ball position
    g_ball_position += g_ball_movement * BALL_SPEED * g_delta_time;

    // Update playerOne position
    if (g_playerOne_position.y > 3.25f) 
    {
        g_playerOne_position.y = 3.25f;
    }
    else if (g_playerOne_position.y < -3.25f) 
    {
        g_playerOne_position.y = -3.25f;
    }
    else
    {
        g_playerOne_position.y += g_playerOne_movement.y * PADDLE_SPEED * g_delta_time;
    }

    // Conditional for updating playerTwo position
    if (singleMode) 
    {
        g_playerTwo_position.y += botDirection * PADDLE_SPEED * g_delta_time;

        if (g_playerTwo_position.y > 3.25f || g_playerTwo_position.y < -3.25f)
        {
            botDirection = -1.0f * botDirection;
        }
    }
    else
    {
        if (g_playerTwo_position.y > 3.25f)
        {
            g_playerTwo_position.y = 3.25f;
        }
        else if (g_playerTwo_position.y < -3.25f)
        {
            g_playerTwo_position.y = -3.25f;
        }
        else
        {
            g_playerTwo_position.y += g_playerTwo_movement.y * PADDLE_SPEED * g_delta_time;
        }
    }

    // Collision Detection for Walls
    if (g_ball_position.y > 3.5f || g_ball_position.y < -3.5f)
    {
        g_ball_movement.y *= -1.0f;
    }

    // Collision Detection for Player One
    float x_distance_one = fabs(g_ball_position.x - g_playerOne_position.x) - ((INIT_SCALE_BALL.x + INIT_SCALE_PLAYERONE.x) / 2.0f);
    float y_distance_one = fabs(g_ball_position.y - g_playerOne_position.y) - ((INIT_SCALE_BALL.y + INIT_SCALE_PLAYERONE.y) / 2.0f);

    // Collision Detection for Player Two
    float x_distance_two = fabs(g_ball_position.x - g_playerTwo_position.x) - ((INIT_SCALE_BALL.x + INIT_SCALE_PLAYERTWO.x) / 2.0f);
    float y_distance_two = fabs(g_ball_position.y - g_playerTwo_position.y) - ((INIT_SCALE_BALL.y + INIT_SCALE_PLAYERTWO.y) / 2.0f);

    if ((x_distance_one < 0.0f && y_distance_one < 0.0f) || (x_distance_two < 0.0f && y_distance_two < 0.0f))
    {
        g_ball_movement.x *= -1.0f;
        g_ball_movement.y += ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
    }

    // Update matrices
    g_playerOne_matrix = glm::mat4(1.0f);
    g_playerOne_matrix = glm::translate(g_playerOne_matrix, g_playerOne_position);

    g_playerTwo_matrix = glm::mat4(1.0f);
    g_playerTwo_matrix = glm::translate(g_playerTwo_matrix, g_playerTwo_position);

    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);

    // Game over condition
    if (g_ball_position.x < -5.0f || g_ball_position.x > 5.0f)
    {
        g_app_status = TERMINATED;
    }
}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Draw Player One
    draw_object(g_playerOne_matrix, g_playerOne_texture_id);

    // Draw Player Two
    draw_object(g_playerTwo_matrix, g_playerTwo_texture_id);

    // Draw Ball
    draw_object(g_ball_matrix, g_ball_texture_id);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}