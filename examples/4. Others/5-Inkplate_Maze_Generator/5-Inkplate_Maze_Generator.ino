#include "Inkplate.h"

Inkplate display(INKPLATE_1BIT);

const int cellSize = 10;

const int w = 790 / cellSize, h = 590 / cellSize;
char maze[w * h];

int dx[] = {-1, 0, 0, 1};
int dy[] = {0, -1, 1, 0};

void setup()
{
    Serial.begin(115200);
    display.begin();
    // Generate and display the maze
    generateMaze(maze, w, h);
    showMaze(maze, w, h);
}

void loop()
{
}

// Display the maze
void showMaze(const char *maze, int width, int height)
{
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            if (maze[x + y * width] == 1)
                for (int i = 0; i < 4; ++i)
                {
                    int xx = x + dx[i];
                    int yy = y + dy[i];
                    if (0 <= xx && xx < w && 0 <= yy && yy < h && maze[yy * width + xx] == 1)
                        display.drawLine(3 + x * cellSize + cellSize / 2, 3 + y * cellSize + cellSize / 2,
                                         3 + x * cellSize + cellSize / 2 + (dx[i] * cellSize / 2),
                                         3 + y * cellSize + cellSize / 2 + (dy[i] * cellSize / 2), BLACK);
                }

    display.display();
}

// Carve the maze starting at x, y.
void carveMaze(char *maze, int width, int height, int x, int y)
{
    int x1, y1;
    int x2, y2;
    int dx, dy;
    int dir, count;

    dir = random(4);
    count = 0;
    while (count < 4)
    {
        dx = 0;
        dy = 0;
        switch (dir)
        {
        case 0:
            dx = 1;
            break;
        case 1:
            dy = 1;
            break;
        case 2:
            dx = -1;
            break;
        default:
            dy = -1;
            break;
        }
        x1 = x + dx;
        y1 = y + dy;
        x2 = x1 + dx;
        y2 = y1 + dy;
        if (x2 > 0 && x2 < width && y2 > 0 && y2 < height && maze[y1 * width + x1] == 1 && maze[y2 * width + x2] == 1)
        {
            maze[y1 * width + x1] = 0;
            maze[y2 * width + x2] = 0;
            x = x2;
            y = y2;
            dir = random(4);
            count = 0;
        }
        else
        {
            dir = (dir + 1) % 4;
            count += 1;
        }
    }
}

// Generate maze in matrix maze with size width, height.
void generateMaze(char *maze, int width, int height)
{
    int x, y;

    // Initialize the maze.
    for (x = 0; x < width * height; x++)
    {
        maze[x] = 1;
    }
    maze[1 * width + 1] = 0;

    // Seed the random number generator.
    srand(time(0));

    // Carve the maze.
    for (y = 1; y < height; y += 2)
    {
        for (x = 1; x < width; x += 2)
        {
            carveMaze(maze, width, height, x, y);
        }
    }

    // Set up the entry and exit.
    maze[0 * width + 1] = 0;
    maze[(height - 1) * width + (width - 2)] = 0;
}