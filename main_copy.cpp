#include <math.h>
#include <string>
#include <chrono>
#include <locale.h>
#include <ncurses.h>
#include <vector>
#include <utility>
#include <algorithm>
using namespace std;

float fPlayerX = 8.0;
float fPlayerY = 8.0;
float fPlayerAngle = 0.0;
float fPlayerMoveSpeed = 100;
float fPlayerRotateSpeed = 50;

int nMapHeight = 16;
int nMapWidth = 25;
float fFOV = 3.14159265 / 4.0;
float fDepth = 25.0;

WINDOW *create_newwin(int height, int width, int startx, int starty);

int main()
{
    setlocale(LC_ALL, "");

	initscr();
    curs_set(0);
    if(has_colors() == FALSE)
	{	endwin();
		printf("Your terminal does not support color\n");
		exit(1);
	}
	start_color();			/* Start color 			*/
	init_pair(1, COLOR_RED, COLOR_BLUE);

    noecho();

    int nScreenWidth{COLS / 2};
    int nScreenHeight{nScreenWidth / 3};
    int nScreenStartPosY = (LINES - nScreenHeight) / 2;
    int nScreenStartPosX = (COLS - nScreenWidth) / 2;
    WINDOW *gameWin = create_newwin(nScreenHeight, nScreenWidth, nScreenStartPosY, nScreenStartPosX);
    WINDOW *mini_map = newwin(nMapHeight, nMapWidth, 1, 1);

    nodelay(gameWin, TRUE);

    wstring map;
    map += L"#########################";
    map += L"#.......................#";
    map += L"#.......................#";
    map += L"#######..........########";
    map += L"#######..........########";
    map += L"#................##.....#";
    map += L"#................##.....#";
    map += L"#................##.....#";
    map += L"#................##.....#";
    map += L"#.......................#";
    map += L"#...########............#";
    map += L"#......##...............#";
    map += L"#......##...............#";
    map += L"#......##...............#";
    map += L"#......##...............#";
    map += L"#########################";

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    // wattron(gameWin, COLOR_PAIR(1));
    
    // Game Loop
    while (1)
    {
        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;

        float fElapsedTime = elapsedTime.count();

        // control
        wchar_t ch = wgetch(gameWin);
        switch(ch)
        {
        case 'a': 
            fPlayerAngle -= fPlayerRotateSpeed * fElapsedTime;
            break;
        case 'd': 
            fPlayerAngle += fPlayerRotateSpeed * fElapsedTime;
            break;
        case 'w':
            fPlayerX += sin(fPlayerAngle) * fPlayerMoveSpeed * fElapsedTime;
            fPlayerY += cos(fPlayerAngle) * fPlayerMoveSpeed * fElapsedTime;
            // wall collision
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
            {
                fPlayerX -= sin(fPlayerAngle) * fPlayerMoveSpeed * fElapsedTime;
                fPlayerY -= cos(fPlayerAngle) * fPlayerMoveSpeed * fElapsedTime;
            }
            break;
        case 's':
            fPlayerX -= sin(fPlayerAngle) * fPlayerMoveSpeed * fElapsedTime;
            fPlayerY -= cos(fPlayerAngle) * fPlayerMoveSpeed * fElapsedTime;
            // wall collision
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
            {
                fPlayerX += sin(fPlayerAngle) * fPlayerMoveSpeed * fElapsedTime;
                fPlayerY += cos(fPlayerAngle) * fPlayerMoveSpeed * fElapsedTime;
            }
            break;
        default:
            break;
        }

        for(int x = 0; x <= nScreenWidth; ++x)
        {
            // For each screen column calculate the projected ray angle into world space
            float fRayAngle = 
                (fPlayerAngle - fFOV / 2) + (static_cast<float>(x) / static_cast<float>(nScreenWidth)) * fFOV;
            float fDistanceToWall = 0;
            bool bHitWall = false;
            bool bBoundary = false;

            // unit vector for ray in player space(sin^2 + cos^2 = 1)
            float fEyeX = sin(fRayAngle);
            float fEyeY = cos(fRayAngle);

            while(!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += 0.1;
                int nTestX = fPlayerX + fEyeX * fDistanceToWall;
                int nTestY = fPlayerY + fEyeY * fDistanceToWall;

                // test if ray is out of boundary
                if(nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    bHitWall = true;                // Just set distance to maximum depth
                    fDistanceToWall = fDepth;
                }
                else
                {
                    // Ray is inbounds so test to see if the ray cell is a wall block
                    if(map[nTestY * nMapWidth + nTestX] == '#') 
                    {
                        bHitWall = true;

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
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;
                                float d = sqrt(vx*vx + vy*vy); 
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
                                p.push_back(make_pair(d, dot));
                            }

                        // Sort Pairs from closest to farthest
                        sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right) { return left.first < right.first; });
                        
                        // First two/three are closest (we will never see all four)
                        float fBound = 0.005;
                        if (acos(p.at(0).second) < fBound) bBoundary = true;
                        if (acos(p.at(1).second) < fBound) bBoundary = true;
                        // if (acos(p.at(2).second) < fBound) bBoundary = true;
                    }
                }
            }

            // Calculate distance to celling and floor
            int nCelling = nScreenHeight / 2.0f - nScreenHeight / fDistanceToWall;
            int nFloor = nScreenHeight - nCelling;

            // Shader walls based on distance
			wchar_t nShade = ' ';
			if (fDistanceToWall <= fDepth / 4.0f)			nShade = 0x2588;	// Very close	
			else if (fDistanceToWall < fDepth / 3.0f)		nShade = 0x2593;
			else if (fDistanceToWall < fDepth / 2.0f)		nShade = 0x2592;
			else if (fDistanceToWall < fDepth)				nShade = 0x2591;
			else											nShade = ' ';		// Too far away

            if (bBoundary)		nShade = ' '; // Black it out

            for(int y = 0; y < nScreenHeight; ++y)
            {
                if(y < nCelling)
                    mvwprintw(gameWin, y, x, "%lc", ' ');
                else if(y >= nCelling && y <= nFloor)
                    mvwprintw(gameWin, y, x, "%lc", nShade);
                else
                {
                    // shade floor based on distance
                    float b = 1.0f - (y - nScreenHeight / 2.0f) / (nScreenHeight / 2.0f);
                    if(b < 0.25f)        nShade = '=';
                    else if(b < 0.5f)    nShade = '~';
                    else if(b < 0.75f)   nShade = '.';
                    else if(b < 0.9f)    nShade = '-';
                    else                nShade = ' ';
                    
                    mvwprintw(gameWin, y, x, "%lc", nShade);
                }
            }
        }
        box(gameWin, 0, 0);

        // display status
        mvprintw(0, 0, "X:%f, Y:%f, A:%f, FPS:%f", fPlayerX, fPlayerY, fPlayerAngle, 1.0f / fElapsedTime);
        wstring display_map = map;
        display_map[static_cast<int>(fPlayerY) * nMapWidth + static_cast<int>(fPlayerX)] = 'P';
        wmove(mini_map, 0, 0);
        for(size_t nx = 0; nx < nMapWidth; ++nx)
            for(size_t ny = 0; ny < nMapHeight; ++ny)
                mvwaddch(mini_map, ny, nx, display_map[ny * nMapWidth + nx]);

        refresh();
        wrefresh(mini_map);
        wrefresh(gameWin);
    }
    // wattroff(gameWin, COLOR_PAIR(1));
    endwin();
    return 0;
}

WINDOW *create_newwin(int height, int width, int startx, int starty)
{
    WINDOW *local_win = newwin(height, width, startx, starty);
    box(local_win, 0, 0);
    wrefresh(local_win);
    return local_win;
}
