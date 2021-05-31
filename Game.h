#pragma once
#include <ncurses.h>
#include <string>
#include <chrono>

#include <stack>
#include <vector>
#include <utility>

#define PI 3.14159265f

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
    void generateMaze();

private:
    float m_playerX = 1.0f;
    float m_playerY= 1.0f;
    float m_playerAngle = PI / 2.0f;
    float m_playerMoveSpeed = 150.0f;
    float m_playerRotateSpeed = 70.0f;
    int m_mapHeight;
    int m_mapWidth;
    float m_depth;
    int m_mazeHeight = 10;
    int m_mazeWidth = 10;
    float m_FOV = PI / 3.0f;
    int m_screenWidth;
    int m_screenHeight;
    int m_screenStartPosY;
    int m_screenStartPosX;
    WINDOW *m_gameWindow;
    WINDOW *m_miniMapWindow;
    WINDOW *m_mapEditorWindow;
    std::string m_map;
    bool m_running = true;
    bool m_mapEditorMode = false;

    // int m_mazeWidth;
    // int m_mazeHeight;
    std::vector<int> m_maze;
    enum 
    {
        CELL_PATH_N = 0x01,
        CELL_PATH_E = 0X02,
        CELL_PATH_S = 0X04,
        CELL_PATH_W = 0X08,
        CELL_VISITED = 0X10
    };
    int m_nVisitedCells;
    std::stack<std::pair<int, int>> m_stack;
    int m_pathWidth = 2;
};