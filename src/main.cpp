#include "pch.h" // IWYU pragma: keep



#include "game.hpp"


Game game;

int main()
{

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "p1 Sandbox - Raylib Static Test");
    SetTargetFPS(60);
    MaximizeWindow();

    Game_Init(game);
    while (!WindowShouldClose() && game.running)
    {
        float dt = GetFrameTime();
        game.Update(dt);
        BeginDrawing();
        game.Draw(dt);
        EndDrawing();

    }
    Game_Shutdown(game);

    CloseWindow();

    return 0;
}
