// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>
#include <iostream>
#include <vector>

// Include GLFW
#include <GLFW/glfw3.h>

GLFWwindow *window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace glm;
#include <shader.hpp>
#include <texture.hpp>
#include <controls.hpp>

int angles[300 * 300 * 300 * 2]; // angles for obstacles

glm::vec3 centerPositionsObstacles[300][300][300];

glm::vec3 camUp = glm::vec3(0.0, 1.0, 0.0);

float PawnX;
float PawnY;
float PawnZ;

float PawnDirX;
float PawnDirY;
float PawnDirZ;

float radiusPawn;

int h;
int w;
int N;

int bean;
bool finish;

float finishTime;
float startTime;

float scaleF;
float scaleAngle;
int partCount = 20;
int fragmentCount = 20;

float prevMouseX;
float prevMouseY;
bool leftClick;
bool rightClick;

#define space 10
#define vertexcount partCount *fragmentCount * 3 * 3

const float PI = 3.1415926535897;

glm::vec3 sphere[40 * 20 * 3 * 3];
GLfloat ball_vert[40 * 20 * 3 * 3];
GLfloat moving_vert[40 * 20 * 3 * 3];

glm::vec3 obs_vert[30];
int numberOfObs = 12;
float radiusTimeBonus = 1.0 / (10 * N);

vec3 movingObsPosition;
vec3 movingObsDirection;
float radiusMovingObs;

void createsphere(float radius, GLfloat sphere_vert[])
{
    int idx = 0;
    float x, y, z, g;
    float fragmentStep = 2 * PI / fragmentCount;
    float partStep = PI / partCount;
    float sectorAngle, partAngle;
    for (int i = 0; i <= partCount; ++i)
    {
        partAngle = PI / 2 - i * partStep;
        g = radius * cosf(partAngle);
        z = radius * sinf(partAngle);
        for (int j = 0; j <= fragmentCount; ++j)
        {
            sectorAngle = j * fragmentStep;
            x = g * cosf(sectorAngle);
            y = g * sinf(sectorAngle);
            sphere[idx] = glm::vec3(x, y, z);
            idx += 1;
        }
    }
    int frag, part;
    idx = 0;
    for (int i = 0; i < partCount; ++i)
    {
        frag = i * (fragmentCount + 1);
        part = frag + fragmentCount + 1;
        for (int j = 0; j < fragmentCount; ++j, ++frag, ++part)
        {
            if (i != 0)
            {
                sphere_vert[idx] = sphere[frag].x;
                sphere_vert[idx + 1] = sphere[frag].y;
                sphere_vert[idx + 2] = sphere[frag].z;
                sphere_vert[idx + 3] = sphere[part].x;
                sphere_vert[idx + 4] = sphere[part].y;
                sphere_vert[idx + 5] = sphere[part].z;
                sphere_vert[idx + 6] = sphere[frag + 1].x;
                sphere_vert[idx + 7] = sphere[frag + 1].y;
                sphere_vert[idx + 8] = sphere[frag + 1].z;
                idx += 9;
            }
            if (i != (partCount - 1))
            {
                sphere_vert[idx] = sphere[frag + 1].x;
                sphere_vert[idx + 1] = sphere[frag + 1].y;
                sphere_vert[idx + 2] = sphere[frag + 1].z;
                sphere_vert[idx + 3] = sphere[part].x;
                sphere_vert[idx + 4] = sphere[part].y;
                sphere_vert[idx + 5] = sphere[part].z;
                sphere_vert[idx + 6] = sphere[part + 1].x;
                sphere_vert[idx + 7] = sphere[part + 1].y;
                sphere_vert[idx + 8] = sphere[part + 1].z;
                idx += 9;
            }
        }
    }
}

class Ball
{
public:
    Ball(float r)
    {
        radius = r;
        programID = LoadShaders("PawnVS.vertexshader", "PawnFS.fragmentshader");
        bufferId = setBuffers();
        MatrixID = glGetUniformLocation(programID, "MVP");
    }

    GLuint setBuffers()
    {
        GLuint spherebuffer;
        glGenBuffers(1, &spherebuffer);
        glBindBuffer(GL_ARRAY_BUFFER, spherebuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ball_vert), ball_vert, GL_STATIC_DRAW);
        glGenVertexArrays(1, &VertexArrayID);
        return spherebuffer;
    }

    void draw(float near, glm::vec3 cP, glm::vec3 cD, bool ifView)
    {
        glBindVertexArray(VertexArrayID);
        glUseProgram(programID);
        glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (GLfloat)w / (GLfloat)(2.0 * h), near, 150.0f);
        glm::mat4 View = glm::lookAt(cP, cD, camUp);
        glm::mat4 Model = glm::mat4(1.0f);
        Model = glm::translate(Model, vec3(PawnX, PawnY, PawnZ));
        glm::mat4 MVP = Projection * View * Model;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, bufferId);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void *)0);
        if (ifView)
            glUniform1f(4, 1.0);
        else
            glUniform1f(4, 0.0);
        for (int i = 0; i <= 6837; i += 3)
        {
            glDrawArrays(GL_TRIANGLES, i, 3);
        }
    }

private:
    float radius;
    GLuint programID;
    GLuint bufferId;
    GLuint MatrixID;
    GLuint VertexArrayID;
};

class Cube
{
public:
    Cube()
    {
        std::vector<GLuint> result = setBuffers();
        bufferId = result[0];
        texId = result[1];
        programID = LoadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");
        Texture = loadDDS("texture.DDS");
        TextureID = glGetUniformLocation(programID, "myTexture");
        MatrixID = glGetUniformLocation(programID, "MVP");
    }

    std::vector<GLuint> setBuffers()
    {
        std::vector<GLuint> result;
        GLfloat g_vertex_buffer_data[] = {
            // UP
            0.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            // UP
            0.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f,
            // DOWN
            0.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f,
            // DOWN
            0.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 0.0f,
            // LRR
            1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            // LRR
            1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 0.0f,
            // LRL
            0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            // LRL
            0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f,
            // RLR
            0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            // RLR
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            // RLL
            1.0f, 1.0f, 1.0f,
            1.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,
            // RLL
            0.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f};

        static const GLfloat g_uv_buffer_data[] = {
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,

            0.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,

            0.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,

            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,

            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,

            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,

            1.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,

            0.0f,
            1.0f,
            1.0f,
            0.0f,
            0.0f,
            0.0f,

            1.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,

            1.0f,
            1.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,

            1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,

            1.0f,
            1.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
        };
        GLuint vertexbuffer;
        glGenBuffers(1, &vertexbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
        result.push_back(vertexbuffer);
        GLuint uvbuffer;
        glGenBuffers(1, &uvbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);
        result.push_back(uvbuffer);
        glGenVertexArrays(1, &VertexArrayID);
        return result;
    }

    void draw(bool isCrush, bool isCrushWithWall, float near, glm::vec3 cP, glm::vec3 cD)
    {
        glBindVertexArray(VertexArrayID);
        glUseProgram(programID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glUniform1i(TextureID, 0);
        glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (GLfloat)w / (GLfloat)(2.0 * h), near, 150.0f);
        // Camera matrix
        glm::mat4 View = glm::lookAt(cP, cD, camUp);
        glm::mat4 Model = glm::mat4(1.0f);
        glm::mat4 MVP = Projection * View * Model;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, bufferId);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void *)0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, texId);
        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void *)0);

        if (isCrush)
            glUniform1f(4, 1.0);
        else
            glUniform1f(4, 0.0);
        glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }

private:
    GLuint programID;
    GLuint bufferId;
    GLuint texId;
    GLuint TextureID;
    GLuint Texture;
    GLuint VertexArrayID;
    GLuint MatrixID;
};

class StaticObstacles
{
public:
    StaticObstacles(float r)
    {
        radius = r;
        bufferId = setBuffers();
        programID = LoadShaders("ObstaclesVS.vertexshader", "ObstaclesFS.fragmentshader");
        MatrixID = glGetUniformLocation(programID, "MVP");
        MatrixID1 = glGetUniformLocation(programID, "matrix");
    }
    bool IsIntersecting(float x, float y, float z, glm::vec3 v1, glm::vec3 v4, glm::vec3 v3, glm::vec3 v2)
    {
        // generate random points on four triangles and check if one of them is inside the Pawn
        float rPx;
        float rPy;
        float rPz;
        for (int i = 0; i < 20; i++)
        {
            for (int j = 0; j < 20 - i; j++)
            {
                rPx = i / 20.0 * v1.x + j / 20.0 * v3.x + (20.0 - i - j) / 20.0 * v4.x;
                rPy = i / 20.0 * v1.y + j / 20.0 * v3.y + (20.0 - i - j) / 20.0 * v4.y;
                rPz = i / 20.0 * v1.z + j / 20.0 * v3.z + (20.0 - i - j) / 20.0 * v4.z;

                if ((rPx - x) * (rPx - x) + (rPy - y) * (rPy - y) + (rPz - z) * (rPz - z) <= radius * radius)
                    return true;
            }
        }
        for (int i = 0; i < 20; i++)
        {
            for (int j = 0; j < 20 - i; j++)
            {
                rPx = i / 20.0 * v4.x + j / 20.0 * v3.x + (20.0 - i - j) / 20.0 * v2.x;
                rPy = i / 20.0 * v4.y + j / 20.0 * v3.y + (20.0 - i - j) / 20.0 * v2.y;
                rPz = i / 20.0 * v4.z + j / 20.0 * v3.z + (20.0 - i - j) / 20.0 * v2.z;

                if ((rPx - x) * (rPx - x) + (rPy - y) * (rPy - y) + (rPz - z) * (rPz - z) <= radius * radius)
                    return true;
            }
        }
        for (int i = 0; i < 20; i++)
        {
            for (int j = 0; j < 20 - i; j++)
            {
                rPx = i / 20.0 * v1.x + j / 20.0 * v2.x + (20.0 - i - j) / 20.0 * v4.x;
                rPy = i / 20.0 * v1.y + j / 20.0 * v2.y + (20.0 - i - j) / 20.0 * v4.y;
                rPz = i / 20.0 * v1.z + j / 20.0 * v2.z + (20.0 - i - j) / 20.0 * v4.z;

                if ((rPx - x) * (rPx - x) + (rPy - y) * (rPy - y) + (rPz - z) * (rPz - z) <= radius * radius)
                    return true;
            }
        }
        for (int i = 0; i < 20; i++)
        {
            for (int j = 0; j < 20 - i; j++)
            {
                rPx = i / 20.0 * v1.x + j / 20.0 * v2.x + (20.0 - i - j) / 20.0 * v3.x;
                rPy = i / 20.0 * v1.y + j / 20.0 * v2.y + (20.0 - i - j) / 20.0 * v3.y;
                rPz = i / 20.0 * v1.z + j / 20.0 * v2.z + (20.0 - i - j) / 20.0 * v3.z;

                if ((rPx - x) * (rPx - x) + (rPy - y) * (rPy - y) + (rPz - z) * (rPz - z) <= radius * radius)
                    return true;
            }
        }
        return false;
    }
    int check(int forwardOrBack, float newX, float newY, float newZ) // returns 0 if there is crush with an obstacle, 1 if the crush with a wall, 2 otherwise
    {
        // this function will check only fragment of pyramids
        // particular pyramids will be divided into triangles and use isIntersecting function
        float H = 1.0 / N;
        float h = 0.04 * sqrt(3) / 2.0;
        glm::mat4 Model;

        int minXCoordLine = floor((newX - (1.0 / (2.0 * N))) / (1.0 / N));
        int maxXCoordLine = floor((newX + (1.0 / (2.0 * N))) / (1.0 / N));
        int minYCoordLine = floor((newY - (1.0 / (2.0 * N))) / (1.0 / N));
        int maxYCoordLine = floor((newY + (1.0 / (2.0 * N))) / (1.0 / N));
        int minZCoordLine = floor((newZ - (1.0 / (2.0 * N))) / (1.0 / N));
        int maxZCoordLine = floor((newZ + (1.0 / (2.0 * N))) / (1.0 / N));
        for (int i = minXCoordLine; i <= maxXCoordLine; i++)
        {
            for (int j = minYCoordLine; j <= maxYCoordLine; j++)
            {
                for (int k = minZCoordLine; k <= maxZCoordLine; k++)
                {
                    if ((i != 0 || j != 0 || k != 0) && (i > -1 && i < N && j > -1 && j < N && k > -1 && k < N))
                    {
                        glm::vec4 vert1 = vec4(0.0f, (GLfloat)0.0f - static_cast<float>(H / 2.0), static_cast<float>(2.0 / 3.0 * h), 1.0);
                        glm::vec4 vert4 = vec4(0.0f, static_cast<float>(H) - static_cast<float>(H / 2.0), 0.0f, 1.0);
                        glm::vec4 vert3 = vec4(-0.02f, (GLfloat)0.0f - static_cast<float>(H / 2.0), static_cast<float>(h / 3.0), 1.0);
                        glm::vec4 vert2 = vec4(0.02f, (GLfloat)0.0f - static_cast<float>(H / 2.0), -static_cast<float>(h / 3.0), 1.0);
                        Model = glm::mat4(1.0f);
                        Model = glm::translate(Model, centerPositionsObstacles[i][j][k]);
                        Model = glm::rotate(Model, glm::radians((float)angles[2 * (k - 1 + N * j + N * N * i)]), glm::vec3(1, 0, 0));
                        Model = glm::rotate(Model, glm::radians((float)angles[2 * (k - 1 + N * j + N * N * i) + 1]), glm::vec3(0, 0, 1));
                        vert1 = Model * vert1;
                        vert4 = Model * vert4;
                        vert3 = Model * vert3;
                        vert2 = Model * vert2;
                        if (IsIntersecting(newX, newY, newZ, vert1, vert4, vert3, vert2))
                        {
                            if (i == N - 1 && j == N - 1 && k == N - 1)
                            {
                                finish = true;
                            }
                            return 0;
                        }
                    }
                }
            }
        }
        // checks whether our pawn has not left the cube
        if ((newX > (1.0 - radiusPawn)) || (newX < radiusPawn) || (newY > (1.0 - radiusPawn)) || (newY < radiusPawn) || (newZ > (1.0 - radiusPawn)) || (newZ < radiusPawn))
        {
            return 1;
        }
        return 2;
    }

    GLuint setBuffers()
    {
        float H = 1.0 / N;
        float h = 0.04 * sqrt(3) / 2.0;
        float g_vertex_buffer_data[] = {
            0.0f, (GLfloat)0.0f - static_cast<float>(H / 2.0), static_cast<float>(2.0 / 3.0 * h),
            0.0f, static_cast<float>(H) - static_cast<float>(H / 2.0), 0.0f,
            -0.02f, (GLfloat)0.0f - static_cast<float>(H / 2.0), static_cast<float>(h / 3.0),
            0.02f, (GLfloat)0.0f - static_cast<float>(H / 2.0), -static_cast<float>(h / 3.0),
            0.0f, (GLfloat)0.0f - static_cast<float>(H / 2.0), static_cast<float>(2.0 / 3.0 * h),
            0.0f, (GLfloat) static_cast<float>(H / 2.0), 0.0f};
        GLuint obstaclesbuffer;
        glGenBuffers(1, &obstaclesbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, obstaclesbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
        glGenVertexArrays(1, &VertexArrayID);
        return obstaclesbuffer;
    }

    void draw(bool isCrush, bool isCrushWithWall, float near, glm::vec3 cP, glm::vec3 cD)
    {
        glm::mat4 Model;
        glm::mat4 MVP;
        glBindVertexArray(VertexArrayID);
        glUseProgram(programID);
        glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (GLfloat)w / (GLfloat)(2.0 * h), near, 150.0f);
        glm::mat4 View = glm::lookAt(cP, cD, camUp);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, bufferId);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void *)0);
        int angleIdx = 0;
        if (isCrush)
            glUniform1f(7, 1.0);
        else
            glUniform1f(7, 0.0);
        if (isCrushWithWall)
            glUniform1f(8, 1.0);
        else
            glUniform1f(8, 0.0);
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                for (int k = 0; k < N; k++)
                {
                    if (i != 0 || j != 0 || k != 0)
                    {
                        Model = glm::translate(glm::mat4(1.0f), centerPositionsObstacles[i][j][k]);
                        Model = glm::rotate(Model, glm::radians((float)angles[angleIdx]), glm::vec3(1, 0, 0));
                        Model = glm::rotate(Model, glm::radians((float)angles[angleIdx + 1]), glm::vec3(0, 0, 1));
                        MVP = Projection * View * Model;
                        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
                        glUniformMatrix4fv(MatrixID1, 1, GL_FALSE, &Model[0][0]);
                        glUniform3f(4, PawnX, PawnDirY, PawnDirZ);

                        if (i == N - 1 && j == N - 1 && k == N - 1)
                            glUniform1f(5, 1.0);
                        else
                            glUniform1f(5, 0.0);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
                        angleIdx += 2;
                    }
                }
            }
        }
    }

private:
    float radius;
    GLuint programID;
    GLuint bufferId;
    GLuint MatrixID;
    GLuint MatrixID1;
    GLuint VertexArrayID;
};

class TimeBonus
{
public:
    TimeBonus()
    {
        bufferId = setBuffers();
        programID = LoadShaders("TimeBonusVS.vertexshader", "TimeBonusFS.fragmentshader");
        MatrixID = glGetUniformLocation(programID, "MVP");
    }

    void catchBonus() // is checking if we can get a time bonus
    {
        float dist;
        for (unsigned int i = 0; i < numberOfObs; i++)
        {
            if (obs_vert[i].x < 2.0)
            {
                dist = sqrt((PawnX - obs_vert[i].x) * (PawnX - obs_vert[i].x) + (PawnY - obs_vert[i].y) * (PawnY - obs_vert[i].y) + (PawnZ - obs_vert[i].z) * (PawnZ - obs_vert[i].z));
                if (dist <= radiusTimeBonus + radiusPawn)
                {
                    obs_vert[i] = vec3(2.0, 2.0, 2.0);
                    if (startTime > 5.0)
                        startTime -= 5.0; // 5s bonus time
                }
            }
        }
    }

    GLuint setBuffers()
    {
        GLuint spherebuffer;
        glGenBuffers(1, &spherebuffer);
        glBindBuffer(GL_ARRAY_BUFFER, spherebuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ball_vert), ball_vert, GL_STATIC_DRAW);
        glGenVertexArrays(1, &VertexArrayID);
        return spherebuffer;
    }

    void draw(bool isCrush, bool isCrushWithWall, float near, glm::vec3 cP, glm::vec3 cD)
    {
        glm::mat4 MVP;
        glm::mat4 Model;
        glBindVertexArray(VertexArrayID);
        glUseProgram(programID);
        glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (GLfloat)w / (GLfloat)(2.0 * h), near, 150.0f);
        glm::mat4 View = glm::lookAt(cP, cD, camUp);
        if (isCrush)
            glUniform1f(1, 1.0);
        else
            glUniform1f(1, 0.0);
        if (isCrushWithWall)
            glUniform1f(2, 1.0);
        else
            glUniform1f(2, 0.0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, bufferId);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void *)0);
        for (unsigned int i = 0; i < numberOfObs; i++)
        {
            if (obs_vert[i].x < 2.0)
            {
                Model = glm::translate(glm::mat4(1.0f), obs_vert[i]);
                MVP = Projection * View * Model;
                glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

                for (int i = 0; i <= 6837; i += 3)
                {
                    glDrawArrays(GL_TRIANGLES, i, 3);
                }
            }
        }
    }

private:
    Ball ball = Ball(radiusTimeBonus);
    GLuint programID;
    GLuint bufferId;
    GLuint MatrixID;
    GLuint VertexArrayID;
};

class MovingObstacles
{
public:
    MovingObstacles()
    {
        programID = LoadShaders("MovingObsVS.vertexshader", "MovingObsFS.fragmentshader");
        bufferId = setBuffers();
        MatrixID = glGetUniformLocation(programID, "MVP");
    }

    bool crushWithPawn()
    {
        float dist = sqrt((PawnX - movingObsPosition.x) * (PawnX - movingObsPosition.x) + (PawnY - movingObsPosition.y) * (PawnY - movingObsPosition.y) + (PawnZ - movingObsPosition.z) * (PawnZ - movingObsPosition.z));
        if (dist <= radiusMovingObs + radiusPawn)
            return true;
        else
            return false;
    }

    void position() // Moving obstalce is making a move forward
    {
        glm::vec3 direction = glm::vec3(movingObsDirection.x - movingObsPosition.x, movingObsDirection.y - movingObsPosition.y, movingObsDirection.z - movingObsPosition.z);
        float sine = (movingObsDirection.y - movingObsPosition.y) / sqrt((movingObsDirection.x - movingObsPosition.x) * (movingObsDirection.x - movingObsPosition.x) + (movingObsDirection.y - movingObsPosition.y) * (movingObsDirection.y - movingObsPosition.y) + (movingObsDirection.z - movingObsPosition.z) * (movingObsDirection.z - movingObsPosition.z));
        float newY = movingObsPosition.y + 0.01 * sine;
        float newX = movingObsPosition.x + direction.x * (newY - movingObsPosition.y) / direction.y;
        float newZ = movingObsPosition.z + direction.z * (newY - movingObsPosition.y) / direction.y;
        movingObsPosition = vec3(newX, newY, newZ);
        if (newY < 0.0 || newX < 0.0 || newZ > 1.0)
        {
            movingObsPosition = vec3(0.0, 0.0, 1.0);
            movingObsDirection = vec3(1.0, 1.0, 0.0);
        }
        else if (newY > 1.0 || newX > 1.0 || newZ < 0.0)
        {
            movingObsDirection = vec3(0.0, 0.0, 1.0);
            movingObsPosition = vec3(1.0, 1.0, 0.0);
        }
    }

    GLuint setBuffers()
    {
        GLuint spherebuffer;
        glGenBuffers(1, &spherebuffer);
        glBindBuffer(GL_ARRAY_BUFFER, spherebuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(moving_vert), moving_vert, GL_STATIC_DRAW);
        glGenVertexArrays(1, &VertexArrayID);
        return spherebuffer;
    }

    void draw(bool isCrush, bool isCrushWithWall, float prevTime, float currTime, float near, glm::vec3 cP, glm::vec3 cD)
    {
        glBindVertexArray(VertexArrayID);
        glUseProgram(programID);
        glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (GLfloat)w / (GLfloat)(2.0 * h), near, 150.0f);
        glm::mat4 View = glm::lookAt(cP, cD, camUp);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, bufferId);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void *)0);
        if (currTime - prevTime >= 0.2)
            position();
        glm::mat4 Model = glm::mat4(1.0f);
        Model = glm::translate(Model, movingObsPosition);
        glm::mat4 MVP = Projection * View * Model;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        if (isCrush)
            glUniform1f(1, 1.0);
        else
            glUniform1f(1, 0.0);
        if (isCrushWithWall)
            glUniform1f(2, 1.0);
        else
            glUniform1f(2, 0.0);
        for (int i = 0; i <= 6837; i += 3)
        {
            glDrawArrays(GL_TRIANGLES, i, 3);
        }
    }

private:
    Ball ball = Ball(radiusMovingObs);
    GLuint programID;
    GLuint bufferId;
    GLuint MatrixID;
    GLuint VertexArrayID;
};

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        prevMouseX = (float)xpos;
        prevMouseY = (float)ypos;
        leftClick = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        prevMouseX = (float)xpos;
        prevMouseY = (float)ypos;
        rightClick = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        leftClick = false;
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        rightClick = false;
}

void init()
{
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
    }
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    h = 600;
    w = 800;

    PawnX = 1.0 / (2 * N);
    PawnY = 1.0 / (2 * N);
    PawnZ = 1.0 / (2 * N);

    PawnDirX = 0.5;
    PawnDirY = 0.5;
    PawnDirZ = 0.5;

    window = glfwCreateWindow(1600, 600, "Ball in a 3D maze", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        getchar();
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
    }
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glClearColor(0.51f, 0.87f, 0.9f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    // setting central points of pyramids
    int id1 = -1;
    int id2 = 1;
    int id3 = 1;
    for (int i = 0; i < N; i++)
    {
        id1 += 2;
        id2 = 1;
        for (int j = 0; j < N; j++)
        {
            id3 = 1;
            for (int k = 0; k < N; k++)
            {
                centerPositionsObstacles[i][j][k] = vec3(id1 * 1.0 / (2 * N), id2 * 1.0 / (2 * N), id3 * 1.0 / (2 * N));
                id3 += 2;
            }
            id2 += 2;
        }
    }
    startTime = glfwGetTime();
    // setting random angles for pyramids
    srand(bean);
    for (unsigned int i = 0; i < N * N * N * 2; i++)
    {
        angles[i] = rand() % 360;
    }

    for (unsigned int i = 0; i < numberOfObs; i++)
    {
        obs_vert[i] = vec3((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
    }

    radiusTimeBonus = 1.0 / (10 * N);
    radiusPawn = 1.0 / (10 * N);
    movingObsPosition = vec3(0.0, 0.0, 1.0);
    movingObsDirection = vec3(1.0, 1.0, 0.0);
    radiusMovingObs = 0.07;
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    prevMouseY = -1.0;
    prevMouseY = -1.0;
    createsphere(radiusPawn, ball_vert);
    createsphere(radiusMovingObs, moving_vert);
}

void UPMove(float angle)
{
    glm::vec3 direction = glm::vec3(PawnDirX - PawnX, PawnDirY - PawnY, PawnDirZ - PawnZ);
    direction = normalize(direction);
    if (direction.y < 0.99)
    {
        direction = glm::rotate(direction, angle, glm::cross(direction, vec3(0.0, 1.0, 0.0)));
        PawnDirX = PawnX + direction.x;
        PawnDirY = PawnY + direction.y;
        PawnDirZ = PawnZ + direction.z;
    }
}

void DOWNMove(float angle)
{
    glm::vec3 direction = glm::vec3(PawnDirX - PawnX, PawnDirY - PawnY, PawnDirZ - PawnZ);
    direction = normalize(direction);
    if (direction.y > -0.99)
    {
        direction = glm::rotate(direction, angle, cross(direction, vec3(0.0, 1.0, 0.0)));
        PawnDirX = PawnX + direction.x;
        PawnDirY = PawnY + direction.y;
        PawnDirZ = PawnZ + direction.z;
    }
}

void LEFTMove(float angle)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
    glm::vec4 vect = vec4(PawnDirX, PawnDirY, PawnDirZ, 1.0);
    vect = model * vect;
    PawnDirX = vect.x;
    PawnDirY = vect.y;
    PawnDirZ = vect.z;
}

void RIGHTMove(float angle)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 1, 0));
    glm::vec4 vect = vec4(PawnDirX, PawnDirY, PawnDirZ, 1.0);
    vect = model * vect;
    PawnDirX = vect.x;
    PawnDirY = vect.y;
    PawnDirZ = vect.z;
}

int main(int argc, char *argv[])
{
    bean = 0;
    N = 10;
    scaleF = 1.0;
    scaleAngle = 0.0;
    finish = false;
    std::size_t pos1;
    std::string arg1;
    float near;
    vec3 lookAt;
    vec3 cameraPosition;
    if (argc == 3)
    { // two parameters
        std::string arg2 = argv[2];
        std::size_t pos2;
        N = std::stoi(arg2, &pos2);
        if (N > 15)
            N = 15;
        arg1 = argv[1];
        bean = std::stoi(arg1, &pos1);
    }
    if (argc == 2)
    { // one parameter
        arg1 = argv[1];
        bean = std::stoi(arg1, &pos1);
    }
    init();
    Ball pawn = Ball(radiusPawn);
    Cube cube = Cube();
    StaticObstacles obs = StaticObstacles(radiusPawn);
    TimeBonus timeBonus = TimeBonus();
    MovingObstacles movingObstacles = MovingObstacles();
    bool isCrush = false;
    bool isCrushWithWall = false;

    int height;
    int width;

    float currMouseX;
    float currMouseY;
    float prevTime = glfwGetTime();
    float currTime;
    float currMoveTime;
    float prevMoveTime = glfwGetTime();
    int check;
    glm::vec3 direction;
    glm::vec3 position;
    float newX;
    float newY;
    float newZ;
    do
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwGetWindowSize(window, &width, &height);
        w = width;
        h = height;

        // rendering left side of a window
        glViewport(0, 0, ((float)w) / 2, h);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        lookAt = glm::vec3(PawnDirX, PawnDirY, PawnDirZ);
        cameraPosition = glm::vec3(PawnX, PawnY, PawnZ);
        near = radiusPawn;
        pawn.draw(near, cameraPosition, lookAt, false);
        cube.draw(isCrush, isCrushWithWall, near, cameraPosition, lookAt);
        obs.draw(isCrush, isCrushWithWall, near, cameraPosition, lookAt);
        timeBonus.draw(isCrush, isCrushWithWall, near, cameraPosition, lookAt);
        currTime = glfwGetTime();
        movingObstacles.draw(isCrush, isCrushWithWall, prevTime, currTime, near, cameraPosition, lookAt);

        // rendering right side of a window
        glViewport(floor(w / 2), 0, ((float)w / 2), h);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        lookAt = vec3(0.5, 0.5, 0.5);
        cameraPosition = vec3(-1.16, 0.5, 0.5);

        // near = abs(cameraPosition.x) + PawnX - radiusPawn; //camera is not catching what is happening from the pawn to the camera
        near = 1.17;

        pawn.draw(near, cameraPosition, lookAt, true);
        obs.draw(isCrush, isCrushWithWall, near, cameraPosition, lookAt);
        timeBonus.draw(isCrush, isCrushWithWall, near, cameraPosition, lookAt);
        movingObstacles.draw(isCrush, isCrushWithWall, prevTime, currTime, near, cameraPosition, lookAt);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        cube.draw(isCrush, isCrushWithWall, near, cameraPosition, lookAt);

        isCrush = false;
        isCrushWithWall = false;
        if (finish)
        {
            // end of the game
            finishTime = glfwGetTime();
            std::cout << "Your time for the game: " << finishTime - startTime << " s"
                      << "\n";
            break;
        }

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            DOWNMove(-0.015f);
        else if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            UPMove(0.015f);
        else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            direction = glm::vec3(PawnDirX - PawnX, PawnDirY - PawnY, PawnDirZ - PawnZ);
            direction = normalize(direction);
            currMoveTime = glfwGetTime();
            position = direction * 0.01f;
            newX = PawnX + position.x;
            newY = PawnY + position.y;
            newZ = PawnZ + position.z;
            check = obs.check(0, newX, newY, newZ);
            if (check == 2)
            {
                if ((currMoveTime - prevMoveTime) > 0.001)
                {
                    prevMoveTime = currMoveTime;

                    PawnX = newX;
                    PawnY = newY;
                    PawnZ = newZ;

                    PawnDirX += position.x;
                    PawnDirY += position.y;
                    PawnDirZ += position.z;

                    timeBonus.catchBonus();
                }
            }
            else if (check == 0)
                isCrush = true;
            else if (check == 1)
                isCrushWithWall = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        {
            direction = glm::vec3(PawnDirX - PawnX, PawnDirY - PawnY, PawnDirZ - PawnZ);
            direction = normalize(direction);

            position = direction * 0.01f;
            newX = PawnX - position.x;
            newY = PawnY - position.y;
            newZ = PawnZ - position.z;

            currMoveTime = glfwGetTime();
            check = obs.check(1, newX, newY, newZ);
            if (check == 2)
            {
                if ((currMoveTime - prevMoveTime) > 0.001)
                {
                    prevMoveTime = currMoveTime;

                    PawnX = newX;
                    PawnY = newY;
                    PawnZ = newZ;

                    PawnDirX -= position.x;
                    PawnDirY -= position.y;
                    PawnDirZ -= position.z;

                    timeBonus.catchBonus();
                }
            }
            else if (check == 0)
                isCrush = true;
            else if (check == 1)
                isCrushWithWall = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            LEFTMove(2.5f);
        else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            RIGHTMove(-2.5f);

        if (leftClick) // camera view is going up or down - mouse
        {
            double xpos, ypos;
            float angle;
            glfwGetCursorPos(window, &xpos, &ypos);
            currMouseX = (float)xpos;
            currMouseY = (float)ypos;
            if (prevMouseY >= 0.0 && ((prevMouseY - currMouseY) != 0))
            {
                if (prevMouseY - currMouseY > 0)
                {
                    // UP
                    angle = 0.08f * (prevMouseY - currMouseY) / h;
                    UPMove(angle);
                }
                else
                {
                    // DOWN
                    angle = -0.08f * abs(prevMouseY - currMouseY) / h;
                    DOWNMove(angle);
                }
            }
        }
        else if (rightClick) // rotate along Y axis - mouse
        {
            double xpos, ypos;
            float angle;
            glfwGetCursorPos(window, &xpos, &ypos);
            currMouseX = (float)xpos;
            currMouseY = (float)ypos;
            if (prevMouseX >= 0.0 && ((prevMouseX - currMouseX) != 0))
            {
                if (prevMouseX - currMouseX > 0)
                {
                    // LEFT
                    angle = 5.0f * (prevMouseX - currMouseX) / (w / 2.0);
                    LEFTMove(angle);
                }
                else
                {
                    // RIGHT
                    angle = -5.0f * abs(prevMouseX - currMouseX) / (w / 2.0);
                    RIGHTMove(angle);
                }
            }
        }

        if (movingObstacles.crushWithPawn())
        {
            break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(window) == 0);
    glfwTerminate();
    return 0;
}
