#include<iostream>
#include<vector>
#include<algorithm>
#include<Windows.h>
#include<chrono>
using namespace std;

const int nScreenWidth = 120;
const int nScreenHeight = 40;

float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerA = 0.0f;

const int nMapHeight = 16;
const int nMapWidth = 16;

float fFOV = 3.14159 / 4.0;
float fDepth = 16.0f;

int main()
{
    // Create Screen Buffer
    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    wstring map;
    map += L"################";
    map += L"#..........#...#";
    map += L"#..........#...#";
    map += L"#..###.........#";
    map += L"#...........#..#";
    map += L"#.........#....#";
    map += L"#....##........#";
    map += L"#...........##.#";
    map += L"#....##........#";
    map += L"#..............#";
    map += L"#.........#....#";
    map += L"#..............#";
    map += L"#.###......#####";
    map += L"#.....##.......#";
    map += L"#..............#";
    map += L"################";

    // Game Loop
    while (1)
    {
        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        // Controls
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
        {
            fPlayerA -= (0.8f) * fElapsedTime;
        }
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
        {
            fPlayerA += (0.8f) * fElapsedTime;
        }
        if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
        {
            float fNewPlayerX = fPlayerX + sinf(fPlayerA) * 5.0f * fElapsedTime;
            float fNewPlayerY = fPlayerY + cosf(fPlayerA) * 5.0f * fElapsedTime;
            if (map[(int)fNewPlayerY * nMapWidth + (int)fNewPlayerX] != '#')
            {
                fPlayerX = fNewPlayerX;
                fPlayerY = fNewPlayerY;
            }
        }
        if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
        {
            float fNewPlayerX = fPlayerX - sinf(fPlayerA) * 5.0f * fElapsedTime;
            float fNewPlayerY = fPlayerY - cosf(fPlayerA) * 5.0f * fElapsedTime;
            if (map[(int)fNewPlayerY * nMapWidth + (int)fNewPlayerX] != '#')
            {
                fPlayerX = fNewPlayerX;
                fPlayerY = fNewPlayerY;
            }
        }

        for (int x = 0; x < nScreenWidth; x++)
        {
            // Calculate the projected ray angle into world space
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

            float fDistanceToWall = 0;
            bool bHitWall = false;
            bool bBoundary = false;

            float fEyeX = sinf(fRayAngle); // Unit vector for ray in player space
            float fEyeY = cosf(fRayAngle);

            while (!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += 0.1f;
                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);
                // Test if ray is out of bounds
                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    bHitWall = true;
                    fDistanceToWall = fDepth;
                }
                else
                {
                    // Ray is inbound so test to see if the ray cell is a wall block
                    if (map[nTestY * nMapWidth + nTestX] == '#')
                    {
                        bHitWall = true;
                        vector<pair<float, float>> p; // Distance, Dot
                        for (int tx = 0; tx < 2; tx++)
                        {
                            for (int ty = 0; ty < 2; ty++)
                            {
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;
                                float d = sqrt(vx * vx + vy * vy);
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
                                p.push_back(make_pair(d, dot));
                            }
                        }
                        // Sort pairs from closest to farthest
                        sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first; });

                        float fBound = 0.005;
                        if (acos(p.at(0).second) < fBound) bBoundary = true;
                        if (acos(p.at(1).second) < fBound) bBoundary = true;
                        // if (acos(p.at(2).second) < fBound) bBoundary = true;
                    }
                }
            }
            // Calculate distance to ceiling and floor
            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            int nFloor = nScreenHeight - nCeiling;
            // Choose a character to render based on the distance to the wall
            wchar_t wcShade;
            if (fDistanceToWall <= fDepth / 4.0f)
            {
                wcShade = 0x2588; // Very close
            }
            else if (fDistanceToWall < fDepth / 3.0f)
            {
                wcShade = 0x2593;
            }
            else if (fDistanceToWall < fDepth / 2.0f)
            {
                wcShade = 0x2592;
            }
            else if (fDistanceToWall < fDepth)
            {
                wcShade = 0x2591;
            }
            else
            {
                wcShade = ' '; // Too far away
            }
            if (bBoundary) wcShade = ' '; //Black it out

            // Render the wall slice and ceiling/floor
            for (int y = 0; y < nScreenHeight; y++)
            {
                if (y < nCeiling)
                {
                    screen[y * nScreenWidth + x] = ' '; // Ceiling
                }
                else if (y > nCeiling && y <= nFloor)
                {
                    screen[y * nScreenWidth + x] = wcShade; // Wall
                }
                else
                {
                    screen[y * nScreenWidth + x] = '.'; // Floor
                }
            }
        }
        // Display stats
        swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

        //Display the map
        for (int nx = 0; nx < nMapWidth; nx++)
        {
            for (int ny = 0; ny < nMapWidth; ny++)
            {
                screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
            }
        }

        screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

        // Display the frame
        screen[nScreenWidth * nScreenHeight - 1] = '\0';
        WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);
    }
    delete[] screen;
    CloseHandle(hConsole);
    return 0;
}
