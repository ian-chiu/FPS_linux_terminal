#include "Game.h"
#include <cstdlib>
#include <locale.h>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

Game::Game()
{
    setlocale(LC_ALL, "");
	initscr();
    noecho();
    if(has_colors() == FALSE)
	{	
        endwin();
		printf("Your terminal does not support color\n");
		exit(1);
	}
    start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_YELLOW);

    m_screenWidth = COLS / 2;
    m_screenHeight = m_screenWidth / 3;
    m_screenStartPosY = (LINES - m_screenHeight) / 2;
    m_screenStartPosX = (COLS - m_screenWidth) / 2;

    m_gameWindow = newwin(m_screenHeight, m_screenWidth, m_screenStartPosY, m_screenStartPosX);
    m_miniMapWindow = newwin(m_mapHeight, m_mapWidth * 2, 1, 0);
    m_mapEditorWindow = newwin(m_mapHeight, 2 * m_mapWidth, 1, 1);

    nodelay(m_gameWindow, true);

    m_map += L"################";
    m_map += L"#..............#";
    m_map += L"#..............#";
    m_map += L"#######........#";
    m_map += L"#######........#";
    m_map += L"#..............#";
    m_map += L"#..............#";
    m_map += L"#..............#";
    m_map += L"#..............#";
    m_map += L"#..............#";
    m_map += L"#...########...#";
    m_map += L"#......##......#";
    m_map += L"#......##......#";
    m_map += L"#......##......#";
    m_map += L"#......##......#";
    m_map += L"################";
}

Game::~Game()
{
    endwin();
}

void Game::run()
{
    auto lastFrameTime = std::chrono::system_clock::now();
    auto currentFrameTime = std::chrono::system_clock::now();
    while (m_running)
    {
        currentFrameTime = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        if (m_mapEditorMode) 
        {
            curs_set(1);
            editMap();
        }
        else 
        {
            curs_set(0);
            gameControl(elapsedTime.count());
            gameRender();
        }
    }
}

void Game::gameControl(float elapsedTime)
{
    wchar_t ch = wgetch(m_gameWindow);
    switch(ch)
    {
    case 'a': case 'A': 
        m_playerAngle -= m_playerRotateSpeed * elapsedTime;
        break;
    case 'd': case 'D':
        m_playerAngle += m_playerRotateSpeed * elapsedTime;
        break;
    case 'w': case 'W':
        m_playerX += sin(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
        m_playerY += cos(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
        // wall collision
        if(m_map[(int)m_playerY * m_mapWidth + (int)m_playerX] == '#')
        {
            m_playerX -= sin(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
            m_playerY -= cos(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
        }
        break;
    case 's': case 'S':
        m_playerX -= sin(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
        m_playerY -= cos(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
        // wall collision
        if(m_map[(int)m_playerY * m_mapWidth + (int)m_playerX] == '#')
        {
            m_playerX += sin(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
            m_playerY += cos(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
        }
        break;
    case 'm': case 'M':
        m_mapEditorMode = true;
        break;
    default:
        break;
    }
}

void Game::gameRender()
{
    for(int x = 0; x <= m_screenWidth; ++x)
    {
        // For each screen column calculate the projected ray angle into world space
        float rayAngle = 
            (m_playerAngle - m_FOV / 2) + ((float)x / (float)m_screenWidth) * m_FOV;
        float distanceToWall = 0;
        bool hittingWall = false;
        bool isBoundary = false;

        // unit vector for ray in player space(sin^2 + cos^2 = 1)
        float eyeX = sin(rayAngle);
        float eyeY = cos(rayAngle);

        while(!hittingWall && distanceToWall < m_depth)
        {
            distanceToWall += 0.1;
            int testX = m_playerX + eyeX * distanceToWall;
            int testY = m_playerY + eyeY * distanceToWall;

            // test if ray is out of boundary
            if(testX < 0 || testX >= m_mapWidth || testY < 0 || testY >= m_mapHeight)
            {
                hittingWall = true;                // Just set distance to maximum depth
                distanceToWall = m_depth;
            }
            else
            {
                // Ray is inbounds so test to see if the ray cell is a wall block
                if(m_map[testY * m_mapWidth + testX] == '#') 
                {
                    hittingWall = true;

                    // To highlight tile boundaries, cast a ray from each corner
                    // of the tile, to the player. The more coincident this ray
                    // is to the rendering ray, the closer we are to a tile 
                    // boundary, which we'll shade to add detail to the walls
                    vector<pair<float, float>> p;

                    // Test each corner of hit tile, storing the distance from
                    // the player, and the calculated dot product of the two rays
                    for (int tx = 0; tx < 2; tx++)
                        for (int ty = 0; ty < 2; ty++)
                        {
                            // Angle of corner to eye
                            float vy = (float)testY + ty - m_playerY;
                            float vx = (float)testX + tx - m_playerX;
                            float d = sqrt(vx*vx + vy*vy); 
                            float dot = (eyeX * vx / d) + (eyeY * vy / d);
                            p.push_back(make_pair(d, dot));
                        }

                    // Sort Pairs from closest to farthest
                    sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right) { return left.first < right.first; });
                    
                    // First two/three are closest (we will never see all four)
                    float fBound = 0.005;
                    if (acos(p.at(0).second) < fBound) isBoundary = true;
                    if (acos(p.at(1).second) < fBound) isBoundary = true;
                    // if (acos(p.at(2).second) < fBound) isBoundary = true;
                }
            }
        }

        // Calculate distance to celling and floor
        int nCelling = m_screenHeight / 2.0f - m_screenHeight / distanceToWall;
        int nFloor = m_screenHeight - nCelling;

        // Shader walls based on distance
        wchar_t nShade = ' ';
        if (distanceToWall <= m_depth / 4.0f)			nShade = 0x2588;	// Very close	
        else if (distanceToWall < m_depth / 3.0f)		nShade = 0x2593;
        else if (distanceToWall < m_depth / 2.0f)		nShade = 0x2592;
        else if (distanceToWall < m_depth)				nShade = 0x2591;
        else											nShade = ' ';		// Too far away

        if (isBoundary)		nShade = ' '; // Black it out

        for(int y = 0; y < m_screenHeight; ++y)
        {
            if(y < nCelling)
                mvwprintw(m_gameWindow, y, x, "%lc", ' ');
            else if(y >= nCelling && y <= nFloor)
                mvwprintw(m_gameWindow, y, x, "%lc", nShade);
            else
            {
                // shade floor based on distance
                float b = 1.0f - (y - m_screenHeight / 2.0f) / (m_screenHeight / 2.0f);
                if(b < 0.25f)        nShade = '=';
                else if(b < 0.5f)    nShade = '~';
                else if(b < 0.75f)   nShade = '.';
                else if(b < 0.9f)    nShade = '-';
                else                 nShade = ' ';
                
                mvwprintw(m_gameWindow, y, x, "%lc", nShade);
            }
        }
    }
    box(m_gameWindow, 0, 0);

    // display status
    mvprintw(0, 0, "X:%f, Y:%f, A:%f", m_playerX, m_playerY, m_playerAngle * 360.0f / 3.14159265);
    
    refresh();
    mapRender(m_miniMapWindow);
    wrefresh(m_gameWindow);
}

void Game::mapRender(WINDOW *window)
{
    wstring display_map = m_map;
    int playerIdx = (int)m_playerY * m_mapWidth + (int)m_playerX;
    display_map[playerIdx] = 'P';
    for(int x = 0; x < m_mapWidth; x++) {
        for(int y = 0; y < m_mapHeight; y++) {
            if (display_map[y * m_mapWidth + x] == '#') {
                wattron(window, COLOR_PAIR(1));
                mvwaddstr(window, y, 2*x, "  ");
                wattroff(window, COLOR_PAIR(1));
            }
            else if (display_map[y * m_mapWidth + x] == 'P') {
                wattron(window, COLOR_PAIR(2));
                mvwaddstr(window, y, 2*x, "  ");
                wattroff(window, COLOR_PAIR(2));
            }
            else {
               mvwaddstr(window, y, 2*x, "  "); 
            }
        }  
    }
    wrefresh(window);
}

void Game::editMap()
{
    clearScreen();
    mvprintw(0, 0, "EDIT MAP MODE (PRESS M TO SWITCH BACK TO GAME)");
    refresh();
    mapRender(m_mapEditorWindow);

    int pixelX = 0, pixelY = 0;
    while (m_mapEditorMode)
    {
        int ch = wgetch(m_mapEditorWindow);
        switch (ch)
        {
        case 'a': case 'A': 
            if (pixelX > 0)     
                wmove(m_mapEditorWindow, pixelY, pixelX-=2);
            break;
        case 'd': case 'D':
            if (pixelX < m_mapWidth * 2)             
                wmove(m_mapEditorWindow, pixelY, pixelX+=2);
            break;
        case 'w': case 'W':
            if (pixelY > 0)             
                wmove(m_mapEditorWindow, pixelY--, pixelX);
            break;
        case 's': case 'S':
            if (pixelY < m_mapHeight)    
                wmove(m_mapEditorWindow, pixelY++, pixelX);
            break;
        case '1':
        {
            int idx = pixelY * m_mapWidth + pixelX / 2;
            int playerIdx = (int)m_playerY * m_mapWidth + (int)m_playerX;
            if (idx != playerIdx) 
            {
                wattron(m_mapEditorWindow, COLOR_PAIR(1));
                mvwaddstr(m_mapEditorWindow, pixelY, pixelX, "  ");
                wattroff(m_mapEditorWindow, COLOR_PAIR(1));
                m_map[idx] = '#';
            }
            break;
        }  
        case '2': 
        {
            int idx = pixelY * m_mapWidth + pixelX / 2;
            int playerIdx = (int)m_playerY * m_mapWidth + (int)m_playerX;
            if (idx != playerIdx) 
            {
                mvwaddstr(m_mapEditorWindow, pixelY, pixelX, "  ");
                m_map[idx] = '.';
            }
            break;
        }  
        case '3':
        {
            int updatedIdx = pixelY * m_mapWidth + pixelX / 2;
            if (m_map[updatedIdx] != '#') {
                wattron(m_mapEditorWindow, COLOR_PAIR(2));
                mvwaddstr(m_mapEditorWindow, pixelY, pixelX, "  ");
                wattroff(m_mapEditorWindow, COLOR_PAIR(2));

                mvwaddstr(m_mapEditorWindow, m_playerY, m_playerX * 2, "  ");
                m_playerX = pixelX / 2;
                m_playerY = pixelY;
            }
            break;
        }
        case 'm':
            m_mapEditorMode = false;
            break;
        default:
            break;
        }
        wmove(m_mapEditorWindow, pixelY, pixelX);
        wrefresh(m_mapEditorWindow);
    }
    clearScreen();
}

void Game::clearScreen()
{
    clear();
    wclear(m_miniMapWindow);
    wclear(m_gameWindow);
    wclear(m_mapEditorWindow);
    refresh();
    wrefresh(m_gameWindow);
    wrefresh(m_miniMapWindow);
    wrefresh(m_mapEditorWindow);
}