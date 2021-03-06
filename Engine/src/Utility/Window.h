#pragma once
#include <string>
#include "GL/glew.h"
#include "GLFW\glfw3.h"

class Scene;

class Window
{
public:
    Window(std::string name, unsigned int width, unsigned int height, bool isFullscreen, bool isVSync);
    ~Window();

    void SwapBuffer();
    void PollEvent();

    GLFWwindow* GetWindow();
    std::string Name();
    unsigned int Width();
    unsigned int Height();
    bool IsFullscreen();
    bool IsVSync();
    bool ShouldClose();

    void Name(const std::string& name);
    void Width(unsigned int width);
    void Height(unsigned int height);
    void IsFullScreen(bool isFullscreen);
    void IsVSync(bool isVSync);
    static void Resize_CallBack(GLFWwindow* window, int width, int height);
    static void Keyboard_Callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void Mouse_Callback(GLFWwindow* window, int button, int action, int mods);
	static Scene* scene_;

protected:
    void CreateWindow();

    GLFWwindow* window_;
    GLFWmonitor* monitor_;
    std::string name_;
    unsigned int width_;
    unsigned int height_;
    bool isFullscreen_;
    bool isVSync_;
};

