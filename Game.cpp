#include "Game.h"
#include <cstdlib>
#include <locale.h>
#include <cmath>
#include <algorithm>
#include <ctime>

using namespace std;

Game::Game()
{
    m_maze.resize(m_mazeWidth * m_mazeHeight, 0);
    m_stack.push(make_pair(0, 0));
    m_maze[0] = CELL_VISITED;
    m_nVisitedCells = 1;
    
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
    init_pair(3, COLOR_BLACK, COLOR_BLUE);
    init_pair(4, COLOR_WHITE, COLOR_BLACK);

    m_screenWidth = COLS / 2;
    m_screenHeight = m_screenWidth / 2;
    m_screenStartPosY = (LINES - m_screenHeight) / 2;
    m_screenStartPosX = (COLS - m_screenWidth) / 1.1;

    m_gameWindow = newwin(m_screenHeight, m_screenWidth, m_screenStartPosY, m_screenStartPosX);
    // m_miniMapWindow = newwin(m_mazeHeight, m_mazeWidth * 2, 1, 0);
    // m_mapEditorWindow = newwin(m_mazeHeight, m_mazeWidth * 2, 1, 1);
    m_mapWidth = m_mazeWidth * (m_pathWidth + 1) + 1;
    m_mapHeight = m_mazeHeight * (m_pathWidth + 1) + 1;
    m_depth = m_mapHeight;
    m_map.resize(m_mapWidth * m_mapHeight, '#');
    m_miniMapWindow = newwin(m_mapHeight, m_mapWidth * 2, 1, 1);
    m_mapEditorWindow = newwin(m_mapHeight, m_mapWidth * 2, 1, 1);

    nodelay(m_gameWindow, true);

    
    generateMaze();

    
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
    bool isPlayerInMap = m_playerX >= 0 && m_playerX <= m_mapWidth && m_playerY >= 0 && m_playerY <= m_mapHeight;
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
        if(!isPlayerInMap || m_map[(int)m_playerY * m_mapWidth + (int)m_playerX] == '#')
        {
            m_playerX -= sin(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
            m_playerY -= cos(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
        }
        break;
    case 's': case 'S':
        m_playerX -= sin(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
        m_playerY -= cos(m_playerAngle) * m_playerMoveSpeed * elapsedTime;
        // wall collision
        if(!isPlayerInMap ||  m_map[(int)m_playerY * m_mapWidth + (int)m_playerX] == '#')
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
    string display_map = m_map;
    int playerIdx = (int)m_playerY * m_mapWidth + (int)m_playerX;
    display_map[playerIdx] = 'P';
    for(int x = 0; x < m_mapWidth; x++) 
    {
        for(int y = 0; y < m_mapHeight; y++) 
        {
            // int displayX = m_mapWidth - x - 1;
            if (display_map[y * m_mapWidth + x] == '#') 
            {
                wattron(window, COLOR_PAIR(1));
                mvwaddstr(window, y, 2 * x, "  ");
                wattroff(window, COLOR_PAIR(1));
            }
            else if (display_map[y * m_mapWidth + x] == 'P') 
            {
                wattron(window, COLOR_PAIR(2));
                mvwaddstr(window, y, 2 * x, "  ");
                wattroff(window, COLOR_PAIR(2));
            }
            else 
            {
               mvwaddstr(window, y, 2 * x, "  "); 
            }
        }  
    }
    wrefresh(window);
}

void Game::generateMaze()
{
    srand(time(0));

    while (!m_stack.empty())
        m_stack.pop();
    for (auto &state : m_maze) 
        state = 0;

    m_stack.push(make_pair(0, 0));
    m_maze[0] = CELL_VISITED;
    m_nVisitedCells = 1;
    m_playerX = 1.0f;
    m_playerY = 1.0f;
    // do the maze algorithm
    while (m_nVisitedCells < m_mazeWidth * m_mazeHeight)
    {
        int top_x = m_stack.top().first;
        int top_y = m_stack.top().second;
        auto offset = [&](int x, int y) 
        {
            return (top_y + y) * m_mazeWidth + (top_x + x);
        };
        // step 1: create a set of the unvisited neighbours
        vector<int> neighbours;
        // north neighbour
        if (top_y > 0 && (m_maze[offset(0, -1)] & CELL_VISITED) == 0)
        {
            neighbours.push_back(0);
        }
        // east neighbour
        if (top_x < m_mazeWidth - 1 && (m_maze[offset(1, 0)] & CELL_VISITED) == 0)
        {
            neighbours.push_back(1);
        }
        // south neighbour
        if (top_y < m_mazeHeight - 1 && (m_maze[offset(0, 1)] & CELL_VISITED) == 0)
        {
            neighbours.push_back(2);
        }
        // west neighbour
        if (top_x > 0 && (m_maze[offset(-1, 0)] & CELL_VISITED) == 0)
        {
            neighbours.push_back(3);
        }

        // Are there any neighbour available?
        if (!neighbours.empty()) 
        {
            // Choose a neighbour randomly
            int next_cell_dir = neighbours[rand() % neighbours.size()];
            // Create a path between the neighbour and the current cell
            switch (next_cell_dir)
            {
            case 0: // North
                m_maze[offset(0, 0)] |= CELL_PATH_N;
                m_maze[offset(0, -1)] |= CELL_VISITED | CELL_PATH_S;
                m_stack.push(make_pair(top_x, top_y - 1));
                break;
            case 1: // East
                m_maze[offset(0, 0)] |= CELL_PATH_E;
                m_maze[offset(1, 0)] |= CELL_VISITED | CELL_PATH_W;
                m_stack.push(make_pair(top_x + 1, top_y));
                break;
            case 2: // South
                m_maze[offset(0, 0)] |= CELL_PATH_S;
                m_maze[offset(0, 1)] |= CELL_VISITED | CELL_PATH_N;
                m_stack.push(make_pair(top_x, top_y + 1));
                break;
            case 3: // West
                m_maze[offset(0, 0)] |= CELL_PATH_W;
                m_maze[offset(-1, 0)] |= CELL_VISITED | CELL_PATH_E;
                m_stack.push(make_pair(top_x - 1, top_y));
                break;
            }
            m_nVisitedCells++;
        }
        else
        {
            // no neighbour available -> back track
            m_stack.pop(); // backtrack
        }
    }

    // generate the maze
    for (auto &c : m_map)
        c = '#';

    for (int x = 0; x < m_mazeWidth; x++) 
    {
        for (int y = 0; y < m_mazeHeight; y++) 
        {
            for (int px = 0; px < m_pathWidth; px++)
            {
                for (int py = 0; py < m_pathWidth; py++)
                {
                    if (m_maze[y * m_mazeWidth + x] & CELL_VISITED) 
                    {
                        int mapY = y * (m_pathWidth + 1) + py + 1;
                        int mapX = x * (m_pathWidth + 1) + px + 1;
                        m_map[mapY * m_mapWidth + mapX] = ' ';
                    }
                }
            }
            
            for (int p = 0; p < m_pathWidth; p++) 
            {
                if (m_maze[y * m_mazeWidth + x] & CELL_PATH_S)
                {
                    int mapY = y * (m_pathWidth + 1) + m_pathWidth + 1;
                    int mapX = x * (m_pathWidth + 1) + p + 1;
                    m_map[mapY * m_mapWidth + mapX] = ' ';
                }
                if (m_maze[y * m_mazeWidth + x] & CELL_PATH_E)
                {
                    int mapY = y * (m_pathWidth + 1) + p + 1;
                    int mapX = x * (m_pathWidth + 1) + m_pathWidth + 1;
                    m_map[mapY * m_mapWidth + mapX] = ' ';
                }
            }
        }
    }
}

// void Game::mapRender(WINDOW *window)
// {
//     // do the maze algorithm
//     if (m_nVisitedCells < m_mazeWidth * m_mazeHeight)
//     {
//         int top_x = m_stack.top().first;
//         int top_y = m_stack.top().second;
//         auto offset = [&](int x, int y) 
//         {
//             return (top_y + y) * m_mazeWidth + (top_x + x);
//         };
//         // step 1: create a set of the unvisited neighbours
//         vector<int> neighbours;
//         // north neighbour
//         if (top_y > 0 && (m_maze[offset(0, -1)] & CELL_VISITED) == 0)
//         {
//             neighbours.push_back(0);
//         }
//         // east neighbour
//         if (top_x < m_mazeWidth - 1 && (m_maze[offset(1, 0)] & CELL_VISITED) == 0)
//         {
//             neighbours.push_back(1);
//         }
//         // south neighbour
//         if (top_y < m_mazeHeight - 1 && (m_maze[offset(0, 1)] & CELL_VISITED) == 0)
//         {
//             neighbours.push_back(2);
//         }
//         // west neighbour
//         if (top_x > 0 && (m_maze[offset(-1, 0)] & CELL_VISITED) == 0)
//         {
//             neighbours.push_back(3);
//         }

//         // Are there any neighbour available?
//         if (!neighbours.empty()) 
//         {
//             // Choose a neighbour randomly
//             int next_cell_dir = neighbours[rand() % neighbours.size()];
//             // Create a path between the neighbour and the current cell
//             switch (next_cell_dir)
//             {
//             case 0: // North
//                 m_maze[offset(0, 0)] |= CELL_PATH_N;
//                 m_maze[offset(0, -1)] |= CELL_VISITED | CELL_PATH_S;
//                 m_stack.push(make_pair(top_x, top_y - 1));
//                 break;
//             case 1: // East
//                 m_maze[offset(0, 0)] |= CELL_PATH_E;
//                 m_maze[offset(1, 0)] |= CELL_VISITED | CELL_PATH_W;
//                 m_stack.push(make_pair(top_x + 1, top_y));
//                 break;
//             case 2: // South
//                 m_maze[offset(0, 0)] |= CELL_PATH_S;
//                 m_maze[offset(0, 1)] |= CELL_VISITED | CELL_PATH_N;
//                 m_stack.push(make_pair(top_x, top_y + 1));
//                 break;
//             case 3: // West
//                 m_maze[offset(0, 0)] |= CELL_PATH_W;
//                 m_maze[offset(-1, 0)] |= CELL_VISITED | CELL_PATH_E;
//                 m_stack.push(make_pair(top_x - 1, top_y));
//                 break;
//             }
//             m_nVisitedCells++;
//         }
//         else
//         {
//             // no neighbour available -> back track
//             m_stack.pop(); // backtrack
//         }
//     }

//     // draw the maze
//     wbkgd(window, COLOR_PAIR(1));
//     for (int x = 0; x < m_mazeWidth; x++) 
//     {
//         for (int y = 0; y < m_mazeHeight; y++) 
//         {
//             for (int px = 0; px < m_pathWidth; px++)
//             {
//                 for (int py = 0; py < m_pathWidth; py++)
//                 {
//                     if (m_maze[y * m_mazeWidth + x] & CELL_VISITED) 
//                     {
//                         wattron(window, COLOR_PAIR(4));
//                         mvwaddstr(window, y * (m_pathWidth + 1) + py + 1, 2 * (x * (m_pathWidth + 1) + px) + 2, "  ");
//                         wattroff(window, COLOR_PAIR(4));
//                     }
//                     else 
//                     {
//                         wattron(window, COLOR_PAIR(3));
//                         mvwaddstr(window, y * (m_pathWidth + 1) + py + 1, 2 * (x * (m_pathWidth + 1) + px) + 2, "  ");
//                         wattroff(window, COLOR_PAIR(3));
//                     }
//                 }
//             }
            
//             for (int p = 0; p < m_pathWidth; p++) 
//             {
//                 if (m_maze[y * m_mazeWidth + x] & CELL_PATH_S)
//                 {
//                     wattron(window, COLOR_PAIR(4));
//                     mvwaddstr(window, y * (m_pathWidth + 1) + m_pathWidth + 1, 2 * (x * (m_pathWidth + 1) + p) + 2, "  ");
//                     wattroff(window, COLOR_PAIR(4));
//                 }
//                 if (m_maze[y * m_mazeWidth + x] & CELL_PATH_E)
//                 {
//                     wattron(window, COLOR_PAIR(4));
//                     mvwaddstr(window, y * (m_pathWidth + 1) + p + 1, 2 * (x * (m_pathWidth + 1) + m_pathWidth) + 2, "  ");
//                     wattroff(window, COLOR_PAIR(4));
//                 }
//             }
//         }
//     }
//     wrefresh(window);
// }

void Game::editMap()
{
    clearScreen();
    mvprintw(0, 0, "EDIT MAP MODE (PRESS M TO SWITCH BACK TO GAME)");
    refresh();
    mapRender(m_mapEditorWindow);

    m_playerAngle = PI;
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
        case 'g': case 'G':
            generateMaze();
            mapRender(m_mapEditorWindow);
            break;
        case 'c': case 'C':
            for (int x = 0; x < m_mapWidth; x++) 
            {
                for (int y = 0; y < m_mapHeight; y++)
                {
                    if (x == 0 || x == m_mapWidth - 1 || y == 0 || y == m_mapHeight - 1)
                        m_map[y * m_mapWidth + x] = '#';
                    else 
                        m_map[y * m_mapWidth + x] = ' ';
                }    
            }
            mapRender(m_mapEditorWindow);
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
                m_map[idx] = ' ';
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