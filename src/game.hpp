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
 };

struct Game
{
    IGameState state;
    Font font32;
    Font font16;
    Font font8;

    MainMenu mMainMenu;
    bool running = true;
};

inline void MainMenu_OnEnter(MainMenu *self, Game& g)
{

}

inline void MainMenu_OnUpdate(MainMenu *self, Game& g, float dt)
{
    auto &m = *self;
    if (IsKeyPressed(KEY_ESCAPE)) {
        g.running = false;
    }
}

inline void MainMenu_OnDraw(MainMenu* self, const Game& g, float dt)
{
    auto &m = *self;
    ClearBackground(DARKBROWN);

    auto t = "Sandbox engine Проверка шрифта";
    int spacing = 0;
    Vector2 pos{ 0, 20 };

    auto draw_text = [&](const char* text, Font font) {
        // for(int i = 1; i <= 3; i++)
        // {
            DrawTextEx(font, t, pos, font.baseSize, spacing, GREEN);
            pos.y += font.baseSize;

        // }
    };
    draw_text(t, g.font8);
    draw_text(t, g.font16);
    draw_text(t, g.font32);

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
    int codepoints[512] = { codepoints_size };
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
    g.font8 = load_font("assets/fonts/PressStart2P.ttf", 8);
    SetTextureFilter(g.font8.texture, TEXTURE_FILTER_POINT);

    g.font16 = load_font("assets/fonts/FSEX302.ttf", 16);
    SetTextureFilter(g.font16.texture, TEXTURE_FILTER_POINT);

    g.font32 = load_font("assets/fonts/JetBrainsMonoNL-SemiBold.ttf", 36);
    GenTextureMipmaps(&g.font32.texture);
    SetTextureFilter(g.font32.texture, TEXTURE_FILTER_ANISOTROPIC_16X);

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
    UnloadFont(g.font8);
    UnloadFont(g.font16);
    UnloadFont(g.font32);
}
