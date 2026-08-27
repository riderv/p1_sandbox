#pragma once

// Подключаем маленькую функцию MessageBoxA из Windows.h, не таща весь файл
#if defined(_WIN32)
extern "C" __declspec(dllimport) int __stdcall MessageBoxA(void* hWnd, const char* lpText, const char* lpCaption, unsigned int uType);
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
        void (*OnUpdate)(void* obj, Game& g, float dt) = 0;
        void (*OnDraw)(void* obj, const Game& g, float dt) = 0;
    };
    Vtbl *v = 0;
    void OnUpdate(Game& g, float dt) { v->OnUpdate(obj, g, dt); }
    void OnDraw(const Game& g, float dt) { v->OnDraw(obj, g, dt); }
};

// inline void GameState_OnUpdate(IGameState *s, Game& g, float dt)
// {
//     s->v->OnUpdate(s->obj, g, dt);
// }

struct MainMenu
{
    IGameState myState;
    Game *g;
    Font font;
};

struct Game
{
    IGameState *state = 0;
    Font font32;
    MainMenu mMainMenu;
    bool running = true;
};

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

    m.myState.obj = self;
    static IGameState::Vtbl v {
        .OnUpdate = make_callback(MainMenu_OnUpdate),
        .OnDraw =   make_callback(MainMenu_OnDraw)
    };
    m.myState.v = &v;

    m.font = g.font32;

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
        if(!IsFontValid(g.font32)) {
            MessageBoxA(0,
                "Critical Error: " font_file_name " not found!\n"
                "Please check that the 'assets' directory is inside the project root folder.",
                "P1 Sandbox - Fatal Error",
                0x00000010L); // 0x10 — это иконка ошибки (MB_ICONERROR) + кнопка ОК

            abort();
        }
        // Опционально: включаем билинейную фильтрацию, чтобы края были идеально гладкими
        SetTextureFilter(g.font32.texture, TEXTURE_FILTER_BILINEAR);
        #undef  font_file_name
    }

    MainMenu_Init(&g.mMainMenu, g);
    g.state = &g.mMainMenu.myState;
}

inline void Game_Update(Game& g)
{
    g.state->OnUpdate(g, GetFrameTime() );
}

inline void Game_Draw(const Game& g)
{
    g.state->OnDraw(g, GetFrameTime());
}

inline void Game_Shutdown(Game& g)
{
    UnloadFont(g.font32);
}
