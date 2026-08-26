#pragma once

struct Game;

struct GameState
{
    void (*OnUpdate)(Game& g, float dt);
    void (*OnDraw)(const Game& g, float dt);
};

struct MainMenu
{

};

struct Game
{
    GameState *state;
    MainMenu mMainMenu;

};

inline void MainMenu_Init(MainMenu& m)
{

}

inline void MainMenu_OnUpdate(Game& g, float dt)
{

}

inline void MainMenu_OnDraw(const Game& g, float dt)
{

}

GameState gMainMenuState = {
    .OnUpdate = MainMenu_OnUpdate,
    .OnDraw = MainMenu_OnDraw,
};

inline void Game_Init(Game& g)
{
    MainMenu_Init(g.mMainMenu);
    g.state = &gMainMenuState;

}

inline void Game_Update(Game& g)
{
    g.state->OnUpdate(g, GetFrameTime());

}

inline void Game_Draw(const Game& g)
{
    g.state->OnDraw(g, GetFrameTime());
}

inline void Game_Shutdown(Game& g)
{

}
