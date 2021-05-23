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
int nMapWidth = 16;
float fFOV = 3.14159265 / 4.0;
float fDepth = 16.0;

WINDOW *create_newwin(int height, int width, int startx, int starty);

int main()
{
    setlocale(LC_ALL, "");
	initscr();
    noecho();
    cbreak();
    // curs_set(0);

    if(has_colors() == FALSE)
	{	endwin();
		printf("Your terminal does not support color\n");
		exit(1);
	}
	start_color();			/* Start color 			*/
    init_pair(1, COLOR_BLACK, COLOR_CYAN);

    WINDOW *mini_map = newwin(nMapHeight, 2 *nMapWidth, 1, 1);

    string map;
    map += "#########.......";
	map += "#...............";
	map += "#.......########";
	map += "#..............#";
	map += "#......##......#";
	map += "#......##......#";
	map += "#..............#";
	map += "###............#";
	map += "##.............#";
	map += "#......####..###";
	map += "#......#.......#";
	map += "#......#.......#";
	map += "#..............#";
	map += "#......#########";
	map += "#..............#";
	map += "################";

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    tp2 = chrono::system_clock::now();
    chrono::duration<float> elapsedTime = tp2 - tp1;
    tp1 = tp2;

    float fElapsedTime = elapsedTime.count();

    wmove(mini_map, 0, 0);
    for(size_t nx = 0; nx < nMapWidth; nx++) {
        for(size_t ny = 0; ny < nMapHeight; ++ny) {
            if (map[ny * nMapWidth + nx] == '#') {
                wattron(mini_map, COLOR_PAIR(1));
                mvwaddstr(mini_map, ny, 2*nx, "  ");
                wattroff(mini_map, COLOR_PAIR(1));
            }
        }  
    }

    refresh();
    wrefresh(mini_map);
    
    nodelay(mini_map, true);
    int x = 0, y = 0;
    int ch = 0;
    while(1) 
    {
        while((ch = getch()) != 'q')
        {
            switch (ch)
            {
            case 'a': 
                if (x > 0)     
                    wmove(mini_map, y, x-=2);
                break;
            case 'd': 
                if (x < nMapWidth * 2)             
                    wmove(mini_map, y, x+=2);
                break;
            case 'w':
                if (y > 0)             
                    wmove(mini_map, y--, x);
                break;
            case 's':
                if (y < nMapHeight)    
                    wmove(mini_map, y++, x);
                break;
            case 'e':
                wattron(mini_map, COLOR_PAIR(1));
                mvwaddstr(mini_map, y, x, "  ");
                wattroff(mini_map, COLOR_PAIR(1));
                map[y * nMapWidth + x / 2] = '#';
                break;
            default:
                break;
            }
            wmove(mini_map, y, x);
            wrefresh(mini_map);
        }
    }

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
