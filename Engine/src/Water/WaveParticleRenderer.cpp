#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "WaveParticleRenderer.h"
#include "WaveParticleManager.h"
#include "WaveParticle.h"
#include "OpenGL/GLSLProgram.h"
#include "Constant.h"
#include "Manager/GLSLProgramManager.h"


WaveParticleRenderer::WaveParticleRenderer(int heightMapWidth, int heightMapHeight, WaveParticleManager* manager)
    : heightMapWidth_(heightMapWidth)
    , heightMapHeight_(heightMapHeight)
    , manager_(manager)
{
    StartPointDirectionData_ = new float[Constant::maxNumberOfWaveParticleWidth * Constant::maxNumberOfWaveParticleHeight * 4];
    SpeedTimeAmplitudeRadiusData_ = new float[Constant::maxNumberOfWaveParticleWidth * Constant::maxNumberOfWaveParticleHeight * 4];
    
    waveProgram_ = GLSLProgramManager::Instance()->GetProgram("WaveParticle");
    normalComputeProgram_ = GLSLProgramManager::Instance()->GetProgram("ComputeHeightMapNormal");

    uniformStartPointDirectionTexture_ = waveProgram_->GetUniformLocation("startPointDirectionTexture");
    uniformSpeedTimeAmplitudeRadiusTexture_ = waveProgram_->GetUniformLocation("speedTimeAmplitudeRadiusTexture");
    uniformHeightMapSize_ = waveProgram_->GetUniformLocation("heightMapSize");
    
    CreateRenderBuffer();
    CreateVertexArrayObject();
}

WaveParticleRenderer::~WaveParticleRenderer()
{
    delete[] StartPointDirectionData_;
    delete[] SpeedTimeAmplitudeRadiusData_;
}

void WaveParticleRenderer::Update(double deltaT)
{
    auto aliveParticles = manager_->GetAliveParticles();
    if (aliveParticles.size() > 0)
    {
        #pragma omp parallel for
        for (int i = 0; i < (int)aliveParticles.size(); ++i)
        {
            StartPointDirectionData_[i * 4 + 0] = aliveParticles[i]->startPoint_.x;
            StartPointDirectionData_[i * 4 + 1] = aliveParticles[i]->startPoint_.y;
            StartPointDirectionData_[i * 4 + 2] = aliveParticles[i]->direction_.x;
            StartPointDirectionData_[i * 4 + 3] = aliveParticles[i]->direction_.y;

            SpeedTimeAmplitudeRadiusData_[i * 4 + 0] = aliveParticles[i]->speed_;
            SpeedTimeAmplitudeRadiusData_[i * 4 + 1] = aliveParticles[i]->time_;
            SpeedTimeAmplitudeRadiusData_[i * 4 + 2] = aliveParticles[i]->amplitude_;
            SpeedTimeAmplitudeRadiusData_[i * 4 + 3] = aliveParticles[i]->radius_;

            aliveParticles[i]->Update((float)deltaT, NULL, heightMapWidth_, heightMapHeight_);
        }
        int width = Constant::maxNumberOfWaveParticleWidth;
        int height = (aliveParticles.size() / Constant::maxNumberOfWaveParticleWidth) + 1;
        glBindTexture(GL_TEXTURE_2D, waveStartPointDirectionTexture_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, StartPointDirectionData_);
        glBindTexture(GL_TEXTURE_2D, waveSpeedTimeAmplitudeRadiusTexture_);
        glTexSubImage2D(GL_TEXTURE_2D, 0,0,0, width, height, GL_RGBA, GL_FLOAT, SpeedTimeAmplitudeRadiusData_);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, WaveParticleFbo_);
    glViewport(0, 0, heightMapWidth_, heightMapHeight_);
    glClear(GL_COLOR_BUFFER_BIT);
    waveProgram_->Activate();
    glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, waveStartPointDirectionTexture_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, waveSpeedTimeAmplitudeRadiusTexture_);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    
    glUniform1i(uniformStartPointDirectionTexture_, 0);
    glUniform1i(uniformSpeedTimeAmplitudeRadiusTexture_, 1);
    glUniform2iv(uniformHeightMapSize_, 1, glm::value_ptr(glm::ivec2(heightMapWidth_, heightMapHeight_)));
    
    glBindVertexArray(vao_);
    glDrawElements(GL_POINTS, aliveParticles.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    ComputeNormalMap();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, Constant::ViewportWidth, Constant::ViewPortHeight);
}

GLuint WaveParticleRenderer::GetHeightMapTexture()
{
    return heightMapTexture_;
}

GLuint WaveParticleRenderer::GetNormalHeightMapTexture()
{
    return normalHeightMapTexture_;
}

void WaveParticleRenderer::CreateRenderBuffer()
{
    glGenFramebuffers(1, &WaveParticleFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, WaveParticleFbo_);

    glGenTextures(1, &heightMapTexture_);
    glBindTexture(GL_TEXTURE_2D, heightMapTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, heightMapWidth_, heightMapHeight_, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, heightMapTexture_, 0);


    GLenum buffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, buffers);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw new std::exception("Water wave refraction fbo problem");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &normalHeightMapTexture_);
    glBindTexture(GL_TEXTURE_2D, normalHeightMapTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, heightMapWidth_, heightMapHeight_, 0, GL_RED, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenTextures(1, &waveStartPointDirectionTexture_);
    glBindTexture(GL_TEXTURE_2D, waveStartPointDirectionTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Constant::maxNumberOfWaveParticleWidth, Constant::maxNumberOfWaveParticleHeight, 0, GL_RGBA, GL_FLOAT, StartPointDirectionData_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &waveSpeedTimeAmplitudeRadiusTexture_);
    glBindTexture(GL_TEXTURE_2D, waveSpeedTimeAmplitudeRadiusTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Constant::maxNumberOfWaveParticleWidth, Constant::maxNumberOfWaveParticleHeight, 0, GL_RGBA, GL_FLOAT, SpeedTimeAmplitudeRadiusData_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void WaveParticleRenderer::CreateVertexArrayObject()
{
    const int maxNumberOfParticles = Constant::maxNumberOfWaveParticleWidth * Constant::maxNumberOfWaveParticleHeight;
    GLfloat* vertices = new GLfloat[maxNumberOfParticles * 3];
    GLuint* indexes = new GLuint[maxNumberOfParticles];
    GLfloat* uvCoord = new GLfloat[maxNumberOfParticles * 2];
    for (int i = 0; i < maxNumberOfParticles * 3; ++i)
    {
        vertices[i] = 0.f;
    }
    for (int i = 0; i < maxNumberOfParticles; ++i)
    {
        indexes[i] = i;
    }
    for (int i = 0; i < Constant::maxNumberOfWaveParticleHeight; ++i)
    {
        for (int j = 0; j < Constant::maxNumberOfWaveParticleWidth; ++j)
        {
            uvCoord[(i * Constant::maxNumberOfWaveParticleHeight + j) * 2 + 0] = ((float)j / (float)Constant::maxNumberOfWaveParticleWidth);
            uvCoord[(i * Constant::maxNumberOfWaveParticleHeight + j) * 2 + 1] = ((float)i / (float)Constant::maxNumberOfWaveParticleHeight);
        }
    }

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(3, &vbo_[0]);

    //Vertices
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glBufferData(GL_ARRAY_BUFFER, 3 * maxNumberOfParticles * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    //Faces
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxNumberOfParticles * sizeof(GLuint), indexes, GL_STATIC_DRAW);

    //UVCoord
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[2]);
    glBufferData(GL_ARRAY_BUFFER, 2  * maxNumberOfParticles * sizeof(GLfloat), uvCoord, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    delete[] vertices;
    delete[] indexes;
    delete[] uvCoord;
}

void WaveParticleRenderer::ComputeNormalMap()
{
    normalComputeProgram_->Activate();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, heightMapTexture_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, normalHeightMapTexture_);
    glBindImageTexture(0, normalHeightMapTexture_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);;

    glUniform1i(normalComputeProgram_->GetUniformLocation("normalMap"), 0);
    glUniform1i(normalComputeProgram_->GetUniformLocation("heightMap"), 1);
    glUniform1f(normalComputeProgram_->GetUniformLocation("inv_textureWidth"), 1.f / heightMapWidth_);
    glUniform3fv(normalComputeProgram_->GetUniformLocation("scale"), 1, glm::value_ptr(glm::vec3(30.f, 1.f, 30.f)));
    glDispatchCompute(heightMapWidth_, heightMapHeight_, 1);
}
