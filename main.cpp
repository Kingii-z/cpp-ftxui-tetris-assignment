#include <thread>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <vector>
#include <random>
#include <string>
#include <chrono>
#include <windows.h>

using namespace ftxui;

const int GAME_WIDTH  = 10;
const int GAME_HEIGHT = 20;

int gameBoard[GAME_HEIGHT][GAME_WIDTH] = {0};
int score = 0;
bool gameRunning = true;

using BlockShape = std::vector<std::vector<int>>;
const std::vector<BlockShape> TETRIS_SHAPES = {
    {{1,1,1,1}},
    {{1,1},{1,1}},
    {{0,1,1},{1,1,0}},
    {{1,1,0},{0,1,1}},
    {{1,1,1},{0,1,0}},
    {{1,0,0},{1,1,1}},
    {{0,0,1},{1,1,1}}
};

struct ActiveBlock {
    BlockShape shape;
    int x;
    int y;
};
ActiveBlock currentBlock;

std::mt19937 randomEngine(std::random_device{}());

// 碰撞检测
bool checkCollision(int offsetX, int offsetY, const BlockShape& shape)
{
    for (int i = 0; i < shape.size(); i++)
    {
        for (int j = 0; j < shape[i].size(); j++)
        {
            if (shape[i][j] == 0) continue;
            int newX = currentBlock.x + j + offsetX;
            int newY = currentBlock.y + i + offsetY;

            if (newX < 0 || newX >= GAME_WIDTH)  return true;
            if (newY >= GAME_HEIGHT)              return true;
            if (newY >= 0 && gameBoard[newY][newX] == 1) return true;
        }
    }
    return false;
}

// 方块旋转
BlockShape rotateShape(const BlockShape& s)
{
    int rows = s.size();
    int cols = s[0].size();
    BlockShape res(cols, std::vector<int>(rows));
    for(int i=0;i<rows;i++)
        for(int j=0;j<cols;j++)
            res[j][rows-1 - i] = s[i][j];
    return res;
}

// 落地锁定+消行
void lockBlockAndClearLine()
{
    for (int i = 0; i < currentBlock.shape.size(); i++)
    {
        for (int j = 0; j < currentBlock.shape[i].size(); j++)
        {
            if (currentBlock.shape[i][j])
                gameBoard[currentBlock.y + i][currentBlock.x + j] = 1;
        }
    }

    for (int row = GAME_HEIGHT - 1; row >= 0; row--)
    {
        bool full = true;
        for (int col = 0; col < GAME_WIDTH; col++)
            if (!gameBoard[row][col]) { full = false; break; }

        if (full)
        {
            score += 150;
            for (int k = row; k > 0; k--)
                memcpy(gameBoard[k], gameBoard[k-1], sizeof(gameBoard[k]));
            row++;
        }
    }

    int id = std::uniform_int_distribution<int>(0,6)(randomEngine);
    currentBlock.shape = TETRIS_SHAPES[id];
    currentBlock.x = GAME_WIDTH / 2 - (int)currentBlock.shape[0].size() / 2;
    currentBlock.y = 0;

    if(checkCollision(0,0,currentBlock.shape)) gameRunning = false;
}

// 键盘监听
void keyboardLoop()
{
    while (gameRunning)
    {
        Sleep(50);
        if(GetAsyncKeyState(VK_LEFT) && !checkCollision(-1,0,currentBlock.shape))
        { currentBlock.x--; Sleep(120); }

        if(GetAsyncKeyState(VK_RIGHT) && !checkCollision(1,0,currentBlock.shape))
        { currentBlock.x++; Sleep(120); }

        if(GetAsyncKeyState(VK_DOWN) && !checkCollision(0,1,currentBlock.shape))
        { currentBlock.y++; Sleep(120); }

        // 新增：上键旋转
        if(GetAsyncKeyState(VK_UP))
        {
            BlockShape rotated = rotateShape(currentBlock.shape);
            if(!checkCollision(0,0,rotated))
                currentBlock.shape = rotated;
            Sleep(180);
        }

        if(GetAsyncKeyState(VK_SPACE))
        {
            while(!checkCollision(0,1,currentBlock.shape))
                currentBlock.y++;
            lockBlockAndClearLine();
            Sleep(200);
        }
    }
}

int main()
{
    int startId = std::uniform_int_distribution<int>(0,6)(randomEngine);
    currentBlock.shape = TETRIS_SHAPES[startId];
    currentBlock.x = GAME_WIDTH / 2 - 2;
    currentBlock.y = 0;

    // 官方稳定交互屏幕，彻底解决重影
     auto screen = ScreenInteractive::TerminalOutput();
     std::thread inputThread(keyboardLoop);
     inputThread.detach();
     auto lastFall = std::chrono::steady_clock::now();
     const std::chrono::milliseconds fallTime(550);
     while(gameRunning)
     {
         auto now = std::chrono::steady_clock::now();
         auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFall);
         if(delta >= fallTime)
         {
             if(!checkCollision(0,1,currentBlock.shape))
                 currentBlock.y++;
             else
                 lockBlockAndClearLine();
             lastFall = now;
         }
         Elements rows;
         for(int i=0;i<GAME_HEIGHT;i++)
         {
             Elements cells;
             for(int j=0;j<GAME_WIDTH;j++)
             {
                 bool fill = gameBoard[i][j];
                 if(i >= currentBlock.y && i < currentBlock.y + (int)currentBlock.shape.size())
                 {
                     int offY = i - currentBlock.y;
                     if(j >= currentBlock.x && j < currentBlock.x + (int)currentBlock.shape[offY].size())
                         if(currentBlock.shape[offY][j - currentBlock.x]) fill = true;
                 }
                 if(fill)
                     cells.push_back(text("■ ") | bgcolor(Color::Cyan) | color(Color::White));
                 else
                     cells.push_back(text("  "));
             }
             rows.push_back(hbox(std::move(cells)));
         }
         Element ui = hbox({
             vbox(std::move(rows)) | border,
             separator(),
             vbox({
                 text("俄罗斯方块") | bold | center,
                 text("得分: " + std::to_string(score)) | center,
                 separator(),
                 text("↑ 旋转"),
                 text("← → 移动"),
                 text("↓ 加速下落"),
                 text("空格 一键落底")
             }) | border | size(WIDTH, EQUAL, 16)
         }) | center;
         // 稳定渲染 无残影
         Render(screen, ui);
         screen.Print();
         screen.Clear();
         Sleep(40);
     }
     Render(screen, vbox({
         text("游戏结束") | bold | center,
         text("最终得分: " + std::to_string(score)) | center
     }) | border | center);
     screen.Print();
     inputThread.join();
     return 0;
 }
