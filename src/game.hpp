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
    int zoom = 4;
    enum { fonts_count = 4 };
    Font *fonts[fonts_count];
    int current_font = 0;

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

inline void MainMenu_OnEnter(MainMenu *self, Game& g)
{

}


struct Player {
    int x, y;

};

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
    }else if(IsKeyPressed(KEY_PERIOD)) {
        MainMenu_PrevFont(m);
    }else if(IsKeyPressed(KEY_COMMA)) {
        MainMenu_NextFont(m);
    }
}

inline void MainMenu_OnDraw(MainMenu* self, const Game& g, float dt)
{
    auto &m = *self;
    ClearBackground(DARKBROWN);
    Player player = { .x = 3, .y = 5 };
    Vector2 pos = {0};

    int spacing = 0;
    Font *font = MainMenu_CurrentFont(m);
    for(int x = 0; x < 10; x++){
        for(int y = 0; y < 10; y++){
            pos.x = x * font->baseSize * m.zoom;
            pos.y = y * font->baseSize * m.zoom;
            if(player.x == x && player.y == y) {
                DrawTextEx(*font, "@", pos, font->baseSize * m.zoom, spacing, GREEN);
            }else
            if(!x || !y || x >=9 || y >= 9) {
                //draw_text("#", g.font8);
                DrawTextEx(*font, "#", pos, font->baseSize * m.zoom, spacing, GREEN);
            }else{
                 DrawTextEx(*font, ".", pos, font->baseSize * m.zoom, spacing, GREEN);
            }

        }
    }

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
    m.current_font = 1;
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
    g.unscii8 = load_font("assets/fonts/unscii-8-thin.ttf", 8);
    SetTextureFilter(g.unscii8.texture, TEXTURE_FILTER_POINT);

    g.unscii8t = load_font("assets/fonts/unscii-8.ttf", 8);
    SetTextureFilter(g.unscii8t.texture, TEXTURE_FILTER_POINT);

    g.unscii16 = load_font("assets/fonts/unscii-16.ttf", 16);
    SetTextureFilter(g.unscii8t.texture, TEXTURE_FILTER_POINT);

    g.JetBrainsMonoNL_SemiBold = load_font("assets/fonts/JetBrainsMonoNL-SemiBold.ttf", 36);
    GenTextureMipmaps(&g.JetBrainsMonoNL_SemiBold.texture);
    SetTextureFilter(g.JetBrainsMonoNL_SemiBold.texture, TEXTURE_FILTER_ANISOTROPIC_16X);

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
