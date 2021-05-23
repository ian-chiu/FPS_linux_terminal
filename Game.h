#pragma once
#include <ncurses.h>
#include <string>
#include <chrono>

class Game
{
public:
    Game();
    ~Game();
    void run();

private:
    void gameControl(float elapsedTime);
    void gameRender();
    void mapRender(WINDOW *window);
    void clearScreen();
    void editMap();

private:
    float m_playerX = 8.0f;
    float m_playerY= 8.0f;
    float m_playerAngle = 0.0f;
    float m_playerMoveSpeed = 100.0f;
    float m_playerRotateSpeed = 50.0f;
    int m_mapHeight = 16;
    int m_mapWidth = 16;
    float m_FOV = 3.14159265f / 4.0f;
    float m_depth = 25.0;
    int m_screenWidth;
    int m_screenHeight;
    int m_screenStartPosY;
    int m_screenStartPosX;
    WINDOW *m_gameWindow;
    WINDOW *m_miniMapWindow;
    WINDOW *m_mapEditorWindow;
    std::wstring m_map;
    bool m_running = true;
    bool m_mapEditorMode = false;
};