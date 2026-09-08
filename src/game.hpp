#pragma once


#if defined(_WIN32)
extern "C" __declspec(dllimport) int __stdcall MessageBoxA(void* hWnd, const char* lpText, const char* lpCaption, unsigned int uType);
#define WIN_MB_OK           0x00000000L
#define WIN_MB_ICONERROR    0x00000010L
#endif


template <typename T, typename R, typename ...Args>
inline auto make_callback(R(*func)(T *, Args...))
{
    return reinterpret_cast<R(*)(void*, Args...)>(func);
}



struct Game;

struct IGameState
{
    void *obj = 0;
    struct Vtbl {
        void (*OnEnter)(void* obj, Game& g) = [](void*, Game&){};
        void (*OnLeave)(void* obj, Game& g) = [](void*, Game&){};
        void (*OnUpdate)(void* obj, Game& g, float dt) = [](void*, Game&, float){};
        void (*OnDraw)(void* obj, const Game& g, float dt) = [](void*, const Game&, float){};
    };
    static Vtbl default_vtbl;
    Vtbl *v = &default_vtbl;
    //syntax sugar
    void OnUpdate(Game& g, float dt) { v->OnUpdate(obj, g, dt); }
    void OnDraw(const Game& g, float dt) { v->OnDraw(obj, g, dt); }
    void OnEnter(Game &g) { v->OnEnter(obj, g); }
    void OnLeave(Game& g) { v->OnLeave(obj, g); }
};
inline IGameState::Vtbl IGameState::default_vtbl{};

// inline void GameState_OnUpdate(IGameState *s, Game& g, float dt)
// {
//     s->v->OnUpdate(s->obj, g, dt);
// }

struct MainMenu
{
    IGameState state;
    Game *g;
    int zoom = 2;
    enum { fonts_count = 4 };
    Font *fonts[fonts_count];
    int current_font = 0;
    int current_font_size = 16;

 };

struct Game
{
    IGameState state;
    Font unscii8;
    Font unscii8t;
    Font unscii16;
    Font JetBrainsMonoNL_SemiBold;
    MainMenu mMainMenu;
    bool running = true;
};

struct WorldChunk
{

};

struct World
{

};

inline void MainMenu_OnEnter(MainMenu *self, Game& g)
{

}


struct Player {
    int x, y;

};


// Функция для отрисовки символа, принудительно вписанного в заданный квадрат
inline void DrawTextCodepointInBox(Font font, int codepoint, Vector2 pos, float boxSize, Color tint)
{
    int index = GetGlyphIndex(font, codepoint);

    if (index < 0 || font.recs[index].width <= 0 || font.recs[index].height <= 0) {
        index = GetGlyphIndex(font, '?');
        if (index < 0 || font.recs[index].width <= 0 || font.recs[index].height <= 0) {
            DrawRectangleLines((int)pos.x, (int)pos.y, (int)boxSize, (int)boxSize, RED);
            return;
        }
    }

    Rectangle srcRec = font.recs[index];

    float glyphWidth = (font.glyphs[index].advanceX == 0) ? srcRec.width : (float)font.glyphs[index].advanceX;
    float glyphHeight = (float)font.baseSize;

    // АВТОМАТИЧЕСКИЙ ПАДДИНГ:
    // Если базовый размер шрифта маленький (например, 8 для unscii), значит он растровый -> ставим 1 пиксель.
    // Если шрифт большой (36+ для JetBrains) -> паддинг не нужен (0).
    float padding = (font.baseSize <= 16) ? 1.0f : 0.0f;

    // Рабочая зона внутри ячейки с учетом паддинга
    float usableBoxSizeX = boxSize - (padding * 2.0f);
    float usableBoxSizeY = boxSize - (padding * 2.0f);

    float scaleFactorY = usableBoxSizeY / glyphHeight;

    // Для векторных шрифтов (где padding == 0) оставляем сужение 0.85f.
    // Для растровых (где есть жесткий отступ 1px) можно использовать 1.0f, чтобы не плющить пиксели.
    float widthRatio = (padding > 0.0f) ? 1.0f : 0.85f;
    float scaleFactorX = (usableBoxSizeX * widthRatio) / glyphWidth;

    float valueOffsetX = font.glyphs[index].offsetX;
    float valueOffsetY = font.glyphs[index].offsetY;

    // Центрирование символа
    float remainingSpaceX = usableBoxSizeX - (glyphWidth * scaleFactorX);
    float remainingSpaceY = usableBoxSizeY - (glyphHeight * scaleFactorY);

    float drawX = std::floor(pos.x + padding + (valueOffsetX * scaleFactorX) + (remainingSpaceX * 0.5f));
    float drawY = std::floor(pos.y + padding + (valueOffsetY * scaleFactorY) + (remainingSpaceY * 0.5f));

    Rectangle destRec = {
        drawX,
        drawY,
        std::floor(srcRec.width * scaleFactorX),
        std::floor(srcRec.height * scaleFactorY)
    };

    Vector2 origin = { 0.0f, 0.0f };
    DrawTexturePro(font.texture, srcRec, destRec, origin, 0.0f, tint);
}

inline Font* MainMenu_NextFont(MainMenu &m)
{
    m.current_font = ++m.current_font % m.fonts_count;
    return m.fonts[m.current_font];
}
inline Font* MainMenu_PrevFont(MainMenu &m)
{
    if(--m.current_font < 0 ) m.current_font = m.fonts_count - 1;
        return m.fonts[m.current_font];

}
inline Font* MainMenu_CurrentFont(MainMenu &m)
{
    return m.fonts[m.current_font];
}


inline void MainMenu_OnUpdate(MainMenu *self, Game& g, float dt)
{
    auto &m = *self;
    if (IsKeyPressed(KEY_ESCAPE)) {
        g.running = false;
    }
    else if(IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)){
        m.zoom++;
    }
    else if(IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS)) {
        if(--m.zoom < 1) m.zoom = 1;
    }
    else if(IsKeyPressed(KEY_PERIOD)) {
            MainMenu_PrevFont(m);
        }
    else if(IsKeyPressed(KEY_COMMA)) {
            MainMenu_NextFont(m);
    }

}

 inline void MainMenu_OnDraw(MainMenu* self, const Game& g, float dt)
{
    auto &m = *self;
    ClearBackground(DARKBROWN); //

    Player player = { .x = 3, .y = 4 }; //
    int spacing = 0; //
    Font *font = MainMenu_CurrentFont(m); //
    Color color = { 222, 222, 222, 255 }; //

    // Вычисляем размер одного квадратного тайла в пикселях
    // Константный размер базового шрифта * зум
    float boxSize = m.current_font_size * m.zoom;

    for (int x = 0; x < 10; x++) { //
        for (int y = 0; y < 10; y++) { //

            // Позиция левого верхнего угла нашей плитки (тайла)
            Vector2 pos = {
                x * boxSize,
                y * boxSize
            };

            // Определяем, какой символ (кодопоинт) рисовать в этой ячейке
            int codepoint = '.';
            Color textColor = color;

            if (player.x == x && player.y == y) { //
                codepoint = '@';
                textColor = YELLOW; // подсветим игрока
            }
            else if (!x || !y || x >= 9 || y >= 9) { //
                codepoint = '#';
            }
            else if (x == 3 && y == 3) { //
                codepoint = 'g';
            }
            else if (x == 3 && y == 5) { //
                codepoint = 'T';
            }
            else if(x == 4 && y == 4) {
                codepoint = 'T';
            }
            else if(x==2 && y == 4) {
                codepoint = 'H';
            }

            // Тест фоллбэка: попробуем вывести символ, которого точно нет в ASCII
            // (например, какой-нибудь японский иероглиф или битый код 0)
            if (x == 5 && y == 5) {
                codepoint = 0x4E00; // Этот символ превратится в '?' или в DrawRectangleLines
            }

            // Рисуем символ строго в границах ячейки
            DrawTextCodepointInBox(*font, codepoint, pos, boxSize, textColor);
        }
    }

    // Отрисовка вашего тестового треугольника поверх
    float offsetX = 30, offsetY = 30; //
    int zm = 2; //
    Vector2 v1 = { 0.0f + offsetX, 0.0f + offsetY }; //
    Vector2 v2 = { 0.0f + offsetX, 10.0f*zm + offsetY }; //
    Vector2 v3 = { 10.0f*zm + offsetX, 10.0f*zm + offsetY }; //
    DrawTriangle(v1, v2, v3, GREEN); //
}


inline void MainMenu_Init(MainMenu *self, Game& g)
{
    auto &m = *(MainMenu*)self;

    static IGameState::Vtbl v = {
        .OnEnter = make_callback(MainMenu_OnEnter),
        .OnUpdate = make_callback(MainMenu_OnUpdate),
        .OnDraw =   make_callback(MainMenu_OnDraw)
    };
    m.state.obj = self;
    m.state.v = &v;
    m.fonts[0] = &g.unscii8t;
    m.fonts[1] = &g.unscii8;
    m.fonts[2] = &g.unscii16;
    m.fonts[3] = &g.JetBrainsMonoNL_SemiBold;

}

inline void MainMenu_ChangeState(Game& g, IGameState d)
{
    g.state.OnLeave(g);
    g.state = g.mMainMenu.state;
    g.state.OnEnter(g);
}

inline void Game_LoadFonts(Game &g)
{
    // 1. Создаем массив кодовых точек (ASCII + Кириллица)
    constexpr int codepoints_size = 512;
    int codepoints[codepoints_size] = { 0 };
    for (int i = 0; i < 95; i++)  codepoints[i] = 32 + i;        // Латиница и знаки
    for (int i = 0; i < 255; i++) codepoints[96 + i] = 0x400 + i; // Кириллица (русские буквы)

    constexpr int bufsize = 2048;
    char buf[bufsize];
    auto load_font = [&](const char* file_name, int size) -> Font
    {
        //sprintf_s(buf, bufsize,
        Font font = LoadFontEx(file_name, size, codepoints, codepoints_size);
        if (!IsFontValid(font))
        {
            TraceLog(LOG_ERROR, "CRITICAL: Failed to load main font 'assets/fonts/%s'", file_name);

            #if defined(_WIN32)
                snprintf(buf, bufsize, "Critical Error: '%s' not found!\n"
                    "Please check that the 'assets' directory is inside the project root folder.",
                      buf);
                MessageBoxA(0, buf, "P1 Sandbox - Fatal Error", WIN_MB_OK | WIN_MB_ICONERROR);
            #else
                // Для Linux выводим красивую графическую ошибку через zenity
                // Если zenity нет в системе, команда просто тихо пропустится, но TraceLog в консоль сработает
                snprintf(buf, bufsize, "zenity --error --title='P1 Sandbox - Fatal Error' "
                    "--text='Critical Error: [%s] not found!\nPlease check that the assets directory is inside the project root folder.' 2>/dev/null",
                    file_name);

                (void)system(buf);
            #endif
            abort();
        }
        return font;
    };
    g.unscii8 = load_font("assets/fonts/unscii-8-thin.ttf", 8);
    SetTextureFilter(g.unscii8.texture, TEXTURE_FILTER_POINT);

    g.unscii8t = load_font("assets/fonts/unscii-8.ttf", 8);
    SetTextureFilter(g.unscii8t.texture, TEXTURE_FILTER_POINT);

    g.unscii16 = load_font("assets/fonts/unscii-16.ttf", 16);
    SetTextureFilter(g.unscii8t.texture, TEXTURE_FILTER_POINT);

    g.JetBrainsMonoNL_SemiBold = load_font("assets/fonts/JetBrainsMonoNL-SemiBold.ttf", 64);
    GenTextureMipmaps(&g.JetBrainsMonoNL_SemiBold.texture);
    SetTextureFilter(g.JetBrainsMonoNL_SemiBold.texture, TEXTURE_FILTER_TRILINEAR);

}

inline void Game_Init(Game& g)
{
    Game_LoadFonts(g);

    MainMenu_Init(&g.mMainMenu, g);
    MainMenu_ChangeState(g, g.mMainMenu.state);

}

inline void Game_Update(Game& g)
{
    g.state.OnUpdate(g, GetFrameTime() );
}

inline void Game_Draw(Game& g)
{
    g.state.OnDraw(g, GetFrameTime());
}

inline void Game_Shutdown(Game& g)
{
    UnloadFont(g.unscii8);
    UnloadFont(g.unscii8t);
    UnloadFont(g.unscii16);
}
