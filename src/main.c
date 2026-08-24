#include "pch.h" // IWYU pragma: keep


int sum(int a, int b)
{
	int c = a + b;
	return c;
}

struct ISome;

typedef
struct ISomeVTable {
    int(*GetTag)(struct ISome obj);

}ISomeVTable;

typedef
struct ISome{
    void* obj;
    ISomeVTable *v;
}ISome;

inline
int Some_GetTag(ISome s) {
    return s.v->GetTag(s);
}

typedef
struct Point {
    int x;
    int y;
}Point, *PPoint;


int Point_GetTag(ISome s)
{
    Point *p = (Point*)s.obj;
    return p->x + p->y;
}

ISomeVTable static_Point_VTable = {
    .GetTag = Point_GetTag
};

ISome Point_CreateInterface(Point* p){
    ISome s;
    s.obj = p;
    s.v = &static_Point_VTable;
    return s;
}

int main()
{

    InitWindow(1200, 600, "p1 Sandbox - Raylib Static Test");
    SetTargetFPS(60);

    Font jetbrainsMono = LoadFont("JetBrainsMono-SemiBoldItalic.ttf");

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        // Задаем позицию для вывода текста
        Vector2 position = { 100, 200 };
        DrawTextEx(jetbrainsMono, "Hello! This is JetBrainsMono-SemiBoldItalic.ttf font.", position, 32, 2, MAROON);
        EndDrawing();
    }

    // 3. ОБЯЗАТЕЛЬНО выгружаем шрифт из памяти видеокарты перед выходом
    UnloadFont(jetbrainsMono);
    CloseWindow();

    Point p;
    p.y = 10;
    p.x = 3;
    // для клиента интерфейса ISome, так мы можем скормить объект, реализующий этот интерфейс
    ISome SomeInterface = Point_CreateInterface(&p);


    // А так клиент будет использовать подсунутый объект через интерфейс.
    int point_tag = SomeInterface.v->GetTag(SomeInterface);
    // Предпочтительный вариант вызова
    int point_tag2 =Some_GetTag(SomeInterface);


    return 0;
}
