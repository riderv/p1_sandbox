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
    virtual ~IGameState() = default;
    virtual void OnUpdate(Game& g, float dt) {} ;
    virtual void OnDraw(const Game& g, float dt) {};
    virtual void OnEnter(Game &g) {}
    virtual void OnLeave(Game& g) {}
};

struct MainMenu: IGameState
{
    Game *g;
    int selectedItem = -1;
    const char* hint = "";
    void OnUpdate(Game& g, float dt) override;
    void OnDraw(const Game& g, float dt) override;
    void setHint(const char* hint) { this->hint = hint; }
 };


struct TileDef {
    int codepoint;
    Color fgColor;
    Color bgColor;
    bool isSolid;
};

enum TileId : uint8_t {
    TILE_AIR = 0,
    TILE_FLOOR = 1,
    TILE_WALL = 2,
    TILE_WATER = 3,
    TILE_COUNT
};

inline const TileDef TILE_DATABASE[TILE_COUNT] = {
    /*[TILE_AIR  ] =*/ { ' ', BLANK, BLANK, false },
    /*[TILE_FLOOR] =*/ { '.', { 150, 150, 150, 255 }, BLANK, false },
    /*[TILE_WALL]  =*/ { '#', LIGHTGRAY, DARKGRAY, true },
    /*[TILE_WATER] =*/ { '~', BLUE, DARKBLUE, false }
};

// Проверка времени компиляции (работает без исключений и RTTI)
static_assert(sizeof(TILE_DATABASE) / sizeof(TileDef) == TILE_COUNT,
              "Забыл добавить тайл в TILE_DATABASE!");

struct GameMap {
    static constexpr int Width = 80;
    static constexpr int Height = 45;

    // Карта весит ВСЕГО 3600 байт (80 * 45 * 1 байт)!
    // Она целиком помещается в кэш L1 процессора
    array<uint8_t, Width * Height> tileIds;

    uint8_t get(int x, int y) const noexcept { return tileIds[x + Width * y]; }
    void set(int x, int y, uint8_t id) noexcept { tileIds[x + Width * y] = id; }

    void initDefault() noexcept {
        for (int y = 0; y < Height; ++y) {
            for (int x = 0; x < Width; ++x) {
                if (x == 0 || y == 0 || x == Width - 1 || y == Height - 1) {
                    set(x, y, TILE_WALL);
                } else if (x == 10 && y == 10) {
                    set(x, y, TILE_WATER);
                } else {
                    set(x, y, TILE_FLOOR);
                }
            }
        }
    }
};



struct MapEditor:  IGameState
{
    int cursorX = 0;
    int cursorY = 0;
    uint8_t selectedTileId = TILE_WALL;

    // Состояния для палитры (клавиша Ё / `)
    bool isPaletteOpen = false;
    int selectedPaletteIdx = 0;
    int lastSelectedPaletteIdx = -1; // Для отслеживания двойного клика Nikon-style

    void OnUpdate(Game& g, float dt) override;
    void OnDraw(const Game& g, float dt) override;
};

struct Game
{
    bool running = true;
    Font unscii8;
    Font unscii8t;
    Font unscii16;
    Font JetBrainsMonoNL_SemiBold;
    int zoom = 2;

    MainMenu mMainMenu;
    MapEditor mMapEditor;
    GameMap worldMap;

    enum { fonts_count = 4 };
    Font *fonts[fonts_count];
    int current_font = 0;
    int current_font_size = 16;


    IGameState defaultState;
    IGameState *state = &defaultState; // Default state for safe call Enter/Leave before initialization and no needed null checks.
    inline void ChangeState(IGameState *newState);
    Font font() const { return *fonts[current_font];}
    void SelectNextFont() { current_font = ++current_font % fonts_count; }
    void SelectPrevFont() { if(--current_font < 0 )current_font = fonts_count - 1; }

    inline void Update(float dt);
    inline void Draw(float dt) const;
};




struct Player {
    int x, y;

};


// Функция для отрисовки символа, принудительно вписанного в заданный квадрат
inline void DrawTextCodepointInBox(Font font, int codepoint, Vector2 pos, float boxSize, Color tint, Color bgTint)
{
    // 1. Отрисовка бэкграунда (если он не прозрачный)
    if (bgTint.a > 0) {
        DrawRectangle((int)pos.x, (int)pos.y, (int)boxSize, (int)boxSize, bgTint);
    }

    // 2. Поиск индекса символа
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

    // Автоматический паддинг в зависимости от типа шрифта
    float padding = (font.baseSize <= 16) ? 1.0f : 0.0f;

    float usableBoxSizeX = boxSize - (padding * 2.0f);
    float usableBoxSizeY = boxSize - (padding * 2.0f);

    float scaleFactorY = usableBoxSizeY / glyphHeight;
    float widthRatio = (padding > 0.0f) ? 1.0f : 0.85f;
    float scaleFactorX = (usableBoxSizeX * widthRatio) / glyphWidth;

    float valueOffsetX = font.glyphs[index].offsetX;
    float valueOffsetY = font.glyphs[index].offsetY;

    // Центрирование
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

inline void Game::ChangeState(IGameState *newState)
{
    state->OnLeave(*this);
    state = newState;
    state->OnEnter(*this);
}



inline void MainMenu::OnUpdate(Game& g, float dt)
{
    auto &m = *this;
    const char *press_again_hint = "Нажмите ещё раз для подтверждения.";
    if (IsKeyPressed(KEY_ESCAPE)) {
        g.running = false;
    }
    else if(IsKeyPressed(KEY_ONE)) {
        if(selectedItem == 1) {
            g.ChangeState(&g.mMapEditor);
        }else{
            selectedItem = 1;
            setHint(press_again_hint);
        }
    }
    else if(IsKeyPressed(KEY_TWO)) {
        if(selectedItem == 2) {
            setHint("Сорян-посорян, пока не реализованно.");
        }else{
            selectedItem = 2;
            setHint(press_again_hint);
        }
    }

}

 inline void MainMenu::OnDraw(const Game& g, float dt)
{
    auto &m = *this;
    ClearBackground(DARKBROWN); //
    Vector2 pos { 100.0f, 10.0f };
    float h1size = 32;
    float regsize = 16;
    DrawTextEx(g.font(), "Your will, my Feudal Lord:", pos, h1size, 0, RED );
    pos.y += h1size * g.zoom;

    Color txtColor = (selectedItem == 1) ? GOLD : WHITE;
    DrawTextEx(g.font(), "1. Create / Edit map.", pos, regsize, 0, txtColor);

    pos.y +=   regsize * g.zoom;
    txtColor = (selectedItem == 2) ? GOLD : WHITE;

    pos.y +=   regsize * g.zoom;
    DrawTextEx(g.font(), "2. Play map.", pos, regsize, 0, txtColor);

    pos.y += 400;
    DrawTextEx(g.font(), this->hint, pos, regsize, 0, WHITE);
}


inline void MainMenu_Init(MainMenu *self, Game& g)
{
    auto &m = *(MainMenu*)self;
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

    g.fonts[0] = &g.unscii8t;
    g.fonts[1] = &g.unscii8;
    g.fonts[2] = &g.unscii16;
    g.fonts[3] = &g.JetBrainsMonoNL_SemiBold;

    g.worldMap.initDefault();
    MainMenu_Init(&g.mMainMenu, g);
    g.ChangeState(&g.mMainMenu);

}


inline void Game_Shutdown(Game& g)
{
    UnloadFont(g.unscii8);
    UnloadFont(g.unscii8t);
    UnloadFont(g.unscii16);
}


inline void Game::Update(float dt)
{
    if(IsKeyPressed(KEY_EQUAL)) {
        zoom++;
    }else if( IsKeyPressed(KEY_MINUS)) {
        zoom--;
    }else if( IsKeyPressed(KEY_PERIOD)) {
        SelectPrevFont();
    }else if( IsKeyPressed(KEY_COMMA)){
        SelectNextFont();
    }
    state->OnUpdate(*this, dt);

}

inline void Game::Draw(float dt) const
{
    state->OnDraw(*this, dt);
}

inline void MapEditor::OnUpdate(Game& g, float dt) {
    // 1. Выход из редактора по Ctrl+Q
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_Q)) {
        g.ChangeState(&g.mMainMenu);
        return;
    }

    // 2. Логика работы с ПАЛИТРОЙ (клавиша Ё / `)
    if (IsKeyPressed(KEY_GRAVE)) {
        isPaletteOpen = !isPaletteOpen;
        if (isPaletteOpen) {
            lastSelectedPaletteIdx = -1;
            selectedPaletteIdx = selectedTileId;
        }
    }

    if (isPaletteOpen) {
        // Каждому тайлу соответствует своя буква: 0->A, 1->B, 2->C, 3->D...
        for (int i = 0; i < TILE_COUNT; ++i) {
            int keyCheck = KEY_A + i;
            if (IsKeyPressed(keyCheck)) {
                if (selectedPaletteIdx == i && lastSelectedPaletteIdx == i) {
                    // НАЖАТО ДВАЖДЫ (Nikon-Style) — подтверждаем и закрываем палитру
                    selectedTileId = static_cast<uint8_t>(i);
                    isPaletteOpen = false;
                } else {
                    // ПЕРВОЕ НАЖАТИЕ — просто выбираем пункт и ждем повтора
                    selectedPaletteIdx = i;
                    lastSelectedPaletteIdx = i;
                }
                break;
            }
        }
        return; // Пока открыта палитра, рисовать на карте нельзя
    }

    // 3. СТАНДАРТНОЕ РИСОВАНИЕ МЫШЬЮ
    float boxSize = g.current_font_size * g.zoom;
    Vector2 mousePos = GetMousePosition();

    if (mousePos.x >= 0 && mousePos.x < GetScreenWidth() &&
        mousePos.y >= 0 && mousePos.y < GetScreenHeight())
    {
        int mouseTileX = (int)(mousePos.x / boxSize);
        int mouseTileY = (int)(mousePos.y / boxSize);

        if (mouseTileX >= 0 && mouseTileX < GameMap::Width &&
            mouseTileY >= 0 && mouseTileY < GameMap::Height)
        {
            cursorX = mouseTileX;
            cursorY = mouseTileY;

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                g.worldMap.set(cursorX, cursorY, selectedTileId);
            }
            else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                g.worldMap.set(cursorX, cursorY, 1); // Стираем в TILE_FLOOR
            }
        }
    }

    // Хоткеи 1, 2, 3 на панели как быстрая альтернатива мыши
    if (IsKeyPressed(KEY_ONE))   selectedTileId = 1;
    if (IsKeyPressed(KEY_TWO))   selectedTileId = 2;
    if (IsKeyPressed(KEY_THREE)) selectedTileId = 3;
}



inline void MapEditor::OnDraw(const Game& g, float dt)
{
    ClearBackground(BLACK);

    float boxSize = g.current_font_size * g.zoom;

    // Автовыбор шрифта для карты мира (LOD)
    Font fontToUse = g.JetBrainsMonoNL_SemiBold;
    if (boxSize < 24.0f) {
        fontToUse = g.unscii8; // Растровый unscii-8-thin из вашего кода
    }

    // 1. Отрисовка карты мира
    for (int y = 0; y < GameMap::Height; y++) {
        for (int x = 0; x < GameMap::Width; x++) {
            Vector2 pos = { x * boxSize, y * boxSize };
            if (pos.x < GetScreenWidth() && pos.y < GetScreenHeight()) {
                uint8_t tileId = g.worldMap.get(x, y);
                const TileDef& def = TILE_DATABASE[tileId];
                DrawTextCodepointInBox(fontToUse, def.codepoint, pos, boxSize, def.fgColor, def.bgColor);
            }
        }
    }

    // 2. ОТРИСОВКА ФАНТОМНОЙ КИСТИ И КУРСОРA
    Vector2 cursorRemarksPos = { cursorX * boxSize, cursorY * boxSize };
    const TileDef& currentBrush = TILE_DATABASE[selectedTileId];

    // Превью символа прямо под курсором мыши перед кликом
    if (!isPaletteOpen) {
        Color previewColor = currentBrush.fgColor;
        previewColor.a = 150;
        Color previewBg = currentBrush.bgColor;
        if (previewBg.a > 0) previewBg.a = 100;

        DrawTextCodepointInBox(fontToUse, currentBrush.codepoint, cursorRemarksPos, boxSize, previewColor, previewBg);
    }

    // Рамка курсора с мягким миганием
    float alpha = (sinf(GetTime() * 8.0f) + 1.0f) * 0.5f;
    DrawRectangleLines((int)cursorRemarksPos.x, (int)cursorRemarksPos.y, (int)boxSize, (int)boxSize, Fade(YELLOW, 0.4f + alpha * 0.4f));

    // 3. ИНТЕРФЕЙС ПАЛИТРЫ (ШРИФТ UNSCII-16)
    if (isPaletteOpen) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

        int winW = 450;
        int winH = 220;
        int winX = (GetScreenWidth() - winW) / 2;
        int winY = (GetScreenHeight() - winH) / 2;

        DrawRectangle(winX, winY, winW, winH, { 30, 30, 30, 240 });
        DrawRectangleLines(winX, winY, winW, winH, GRAY);

        // Красивый заголовок на русском
        DrawTextEx(g.unscii16, "ПАЛИТРА БЛОКОВ (Ё)", { (float)winX + 20, (float)winY + 15 }, 16, 0, YELLOW);

        // Список доступных тайлов
        for (int i = 0; i < TILE_COUNT; ++i) {
            float itemY = winY + 50 + i * 25;
            Color itemColor = WHITE;

            if (i == selectedPaletteIdx) {
                // Если выбрано один раз — оранжевый, если подтверждено/повторно — зеленый
                itemColor = (i == lastSelectedPaletteIdx) ? ORANGE : GREEN;
                DrawRectangle(winX + 15, itemY - 2, winW - 30, 20, Fade(itemColor, 0.2f));
            }

            // Рисуем иконку тайла в палитре
            Vector2 iconPos = { (float)winX + 30, itemY };
            DrawTextCodepointInBox(g.unscii16, TILE_DATABASE[i].codepoint, iconPos, 16.0f, TILE_DATABASE[i].fgColor, TILE_DATABASE[i].bgColor);

            // Текст пункта: [Буква] - Название
            char itemBuf[256];
            const char* tileNames[TILE_COUNT] = { "Пустота / Воздух", "Пол / Земля", "Стена / Камень", "Глубокая Вода" };
            snprintf(itemBuf, sizeof(itemBuf), "[%c] - %s", 'A' + i, tileNames[i]);

            DrawTextEx(g.unscii16, itemBuf, { (float)winX + 65, itemY }, 16, 0, itemColor);
        }

        DrawTextEx(g.unscii16, "Нажмите букву дважды для подтверждения", { (float)winX + 20, (float)winY + winH - 25 }, 16, 0, GRAY);
    }

    // 4. СТАТУС-БАР В САМОМ НИЗУ ЭКРАНА (ШРИФТ UNSCII-8)
    int screenH = GetScreenHeight();
    DrawRectangle(0, screenH - 20, GetScreenWidth(), 20, { 20, 20, 20, 255 });

    char statusBuf[256];
    snprintf(statusBuf, sizeof(statusBuf), "Координаты: [%d, %d] | Кисть: [%c] | Палитра: [~] | Выход: [Ctrl+Q]",
             cursorX, cursorY, currentBrush.codepoint);

    DrawTextEx(g.unscii8, statusBuf, { 10, (float)screenH - 14 }, 8, 0, RAYWHITE);
}