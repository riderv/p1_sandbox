#include "raylib.h"

// Функция для отрисовки символа, принудительно вписанного в заданный квадрат
void DrawTextCodepointInBox(Font font, int codepoint, Vector2 position, float boxWidth, float boxHeight, Color color)
{
    // 1. Получаем индекс символа в массиве шрифта
    int index = GetGlyphIndex(font, codepoint);

    // 2. srcRec — область на текстуре атласа, где физически расположен пиксельный рисунок символа
    Rectangle srcRec = font.recs[index];

    // Проверяем, есть ли у символа визуальное отображение (пропускаем пустые глифы)
    if (srcRec.width <= 0 || srcRec.height <= 0) return;

    // 3. Задаем целевой прямоугольник (куда именно на экране мы рисуем)
    // Убираем offsetY и offsetX, чтобы рисунок глифа начинался ровно от переданной позиции position.
    // Принудительно выставляем ширину boxWidth и высоту boxHeight.
    Rectangle destRec = {
        position.x,
        position.y,
        boxWidth,
        boxHeight
    };

    // Опорная точка вращения/смещения (верхний левый угол)
    Vector2 origin = { 0, 0 };

    // 4. Отрисовываем кусок текстуры шрифта, растягивая его ровно под размеры destRec
    DrawTexturePro(font.texture, srcRec, destRec, origin, 0.0f, color);
}

int main(void)
{
    // Инициализация окна
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "raylib 6 - Smooth High-Res Font");

    // Подготовка таблицы символов для загрузки шрифта
    constexpr int codepoints_size = 512;
    int codepoints[codepoints_size] = { 0 };
    for (int i = 0; i < 95; i++)  codepoints[i] = 32 + i;        // Латиница и знаки
    for (int i = 0; i < 255; i++) codepoints[96 + i] = 0x400 + i; // Кириллица (русские буквы)

    // Загружаем кастомный шрифт в высоком разрешении (128 или 256 пикселей)
    // Это дает огромный запас четкости для растягивания в большой квадрат
    Font font = LoadFontEx("assets/fonts/JetBrainsMonoNL-SemiBold.ttf", 128, codepoints, codepoints_size);

    // Включаем классическую билинейную фильтрацию текстуры шрифта
    // (УБРАЛИ GenTextureMipmaps и TRILINEAR, чтобы мелкий текст перестал быть рваным)
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    SetTargetFPS(60);

    // Главный цикл программы
    while (!WindowShouldClose())
    {
        // Отрисовка
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Параметры нашего тестового бокса (250x250)
        Vector2 boxPosition = { 275, 100 };
        float boxSize = 250.0f; // Ширина и высота квадрата
        int character = 'A';    // Символ для отрисовки

        // 1. Рисуем серую рамку квадрата для визуального контроля границ
        DrawRectangleLines(boxPosition.x, boxPosition.y, boxSize, boxSize, LIGHTGRAY);

        // 2. Рисуем символ, растянутый ровно под размеры этого квадрата стандартными средствами
        DrawTextCodepointInBox(font, character, boxPosition, boxSize, boxSize, RED);

        // Дополнительно: выведем текст-подсказку снизу. Теперь он будет идеально сглаженным!
        DrawTextEx(font, "Билинейная фильтрация: текст снова гладкий и аккуратный", (Vector2){115, 380}, 20, 2, DARKGRAY);

        EndDrawing();
    }

    // Очистка ресурсов
    UnloadFont(font);
    CloseWindow();

    return 0;
}
