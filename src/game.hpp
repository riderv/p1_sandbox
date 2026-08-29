#pragma once


#if defined(_WIN32)
// Этот блок останется для Windows, если ты решишь запустить проект там
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
    Font font;
};

struct Game
{
    IGameState state;
    Font font32;
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
    DrawTextEx(m.font, "P1 SANDBOX ENGINE", Vector2{ 80, 150 }, 44, 4, MAROON);

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


    m.font = g.font32;
}

inline void MainMenu_ChangeState(Game& g, IGameState d)
{
    g.state.OnLeave(g);
    g.state = g.mMainMenu.state;
    g.state.OnEnter(g);
}
inline void Game_Init(Game& g)
{

    // LOAD FONTs
    {
        // 1. Создаем массив кодовых точек (ASCII + Кириллица)
        constexpr int codepoints_size = 512;
        int codepoints[512] = { codepoints_size };
        for (int i = 0; i < 95; i++)  codepoints[i] = 32 + i;        // Латиница и знаки
        for (int i = 0; i < 255; i++) codepoints[96 + i] = 0x400 + i; // Кириллица (русские буквы)
        #define font_file_name "assets/fonts/JetBrainsMonoNL-SemiBold.ttf"
        // 2. Загружаем шрифт в размере 32 пикселя со списком символов
        g.font32 = LoadFontEx(font_file_name, 32, codepoints, codepoints_size);
       if (!IsFontValid(g.font32)) {
           TraceLog(LOG_ERROR, "CRITICAL: Failed to load main font 'assets/fonts/%s'", font_file_name);
           
#if defined(_WIN32)
           // Для Windows оставляем нативное окошко
           MessageBoxA(0, 
               "Critical Error: '" font_file_name "' not found!\n"
               "Please check that the 'assets' directory is inside the project root folder.", 
               "P1 Sandbox - Fatal Error", 
               WIN_MB_OK | WIN_MB_ICONERROR);
#else
           // Для Linux выводим красивую графическую ошибку через zenity
           // Если zenity нет в системе, команда просто тихо пропустится, но TraceLog в консоль сработает
           (void)system("zenity --error --title='P1 Sandbox - Fatal Error' "
                  "--text='Critical Error: " font_file_name " not found!\nPlease check that the assets directory is inside the project root folder.' 2>/dev/null");
#endif
           abort();
       }

        // Опционально: включаем билинейную фильтрацию, чтобы края были идеально гладкими
        SetTextureFilter(g.font32.texture, TEXTURE_FILTER_BILINEAR);
        #undef  font_file_name
    }


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
    UnloadFont(g.font32);
}
