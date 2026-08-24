#include "pch.h" // IWYU pragma: keep


int sum(int a, int b)
{
	int c = a + b;
	return c;
}

int main()
{

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "p1 Sandbox - Raylib Static Test");
    SetTargetFPS(60);
    MaximizeWindow();


    Font jetbrainsMono = LoadFont("JetBrainsMono-SemiBoldItalic.ttf");

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLUE);
        // Задаем позицию для вывода текста
        Vector2 position = { 100, 200 };
        DrawTextEx(jetbrainsMono, "Hello! This is JetBrainsMono-SemiBoldItalic.ttf font.", position, 32, 2, MAROON);
        EndDrawing();
    }

    // 3. ОБЯЗАТЕЛЬНО выгружаем шрифт из памяти видеокарты перед выходом
    UnloadFont(jetbrainsMono);
    CloseWindow();

    return 0;
}
