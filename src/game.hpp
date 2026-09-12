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


 enum TileId : uint8_t {
     TILE_AIR,
     TILE_FLOOR,
     TILE_WALL,
     TILE_WATER,
     TILE_DOOR,
     TILE_RAMP_N, // поднимает при движении на север (вверх на экране)
     TILE_RAMP_S, // юг (вниз на экране)
     TILE_RAMP_E, // восток (вправо)
     TILE_RAMP_W, // запад (влево)
     TILE_COUNT
 };

 struct RenderDef {
     int codepoint;
     Color fgColor;
     Color bgColor;
 };

inline const RenderDef TILE_RENDER[TILE_COUNT] = {
//codepoint,    fgColor,    bgColor
    { ' ',      BLANK,      BLANK }, //TILE_AIR
    { '.',      { 150, 150, 150, 255 }, BLANK }, //TILE_FLOOR
    { '#',      LIGHTGRAY,  DARKGRAY }, //TILE_WALL
    { '~',      BLUE,       DARKBLUE }, //TILE_WATER
    { '+',      BROWN,      { 80, 50, 20, 255 } }, //TILE_DOOR
    { 0x25B2,   GOLD,       BLANK }, //TILE_RAMP_N (▲, для палитры/превью — сама клетка рисуется в два прохода)
    { 0x25BC,   GOLD,       BLANK }, //TILE_RAMP_S (▼)
    { 0x25BA,   GOLD,       BLANK }, //TILE_RAMP_E (►)
    { 0x25C4,   GOLD,       BLANK }, //TILE_RAMP_W (◄)
};


 struct PhysicsDef {
     bool isSolid;
 };

 inline const PhysicsDef TILE_PHYSICS[TILE_COUNT] = {
     { .isSolid = false }, //TILE_AIR
     { .isSolid = false }, //TILE_FLOOR
     { .isSolid = true },  //TILE_WALL
     { .isSolid = false }, //TILE_WATER
     { .isSolid = true },  //TILE_DOOR
     { .isSolid = false }, //TILE_RAMP_N
     { .isSolid = false }, //TILE_RAMP_S
     { .isSolid = false }, //TILE_RAMP_E
     { .isSolid = false }, //TILE_RAMP_W
};


 inline const char* TILE_DESC_STRINGS[TILE_COUNT] = {
     "Пустота / Воздух",
     "Пол / Земля",
     "Стена / Камень",
     "Глубокая Вода",
     "Деревянная Дверь",
     "Рампа на север",
     "Рампа на юг",
     "Рампа на восток",
     "Рампа на запад"
 };

 static_assert(sizeof(TILE_RENDER) / sizeof(RenderDef) == TILE_COUNT, "Забыл RenderDef!");
 static_assert(sizeof(TILE_PHYSICS) / sizeof(PhysicsDef) == TILE_COUNT, "Забыл PhysicsDef!");
 static_assert(sizeof(TILE_DESC_STRINGS) / sizeof(char*) == TILE_COUNT, "Забыл описание тайла!");

 inline const char* GetTileDescription(TileId id) {
     return TILE_DESC_STRINGS[id];
 }

 // --- Рампы: направление, обратный тайл, значок лестницы ---
 // Кодпоинт значка ступеней, рисуемого поверх стрелки направления (два
 // прохода отрисовки). ВНИМАНИЕ: этот кодпоинт нужно проверить на
 // компиляции — шрифты по умолчанию грузят только ASCII+кириллицу,
 // 0x1328D добавлен в список загрузки отдельно (см. Game_LoadFonts), но
 // сам символ может отсутствовать в глифах unscii-8/JetBrains Mono. Если
 // отрисуется пусто/'?' — нужно будет подобрать другой кодпоинт.
 constexpr int kRampStairsCodepoint = 0x1328D;

 inline bool IsRampTile(uint8_t tile) {
     return tile == TILE_RAMP_N || tile == TILE_RAMP_S || tile == TILE_RAMP_E || tile == TILE_RAMP_W;
 }

 inline void RampDirectionVector(uint8_t tile, int& dx, int& dy) {
     dx = 0; dy = 0;
     switch (tile) {
         case TILE_RAMP_N: dy = -1; break;
         case TILE_RAMP_S: dy = 1;  break;
         case TILE_RAMP_E: dx = 1;  break;
         case TILE_RAMP_W: dx = -1; break;
         default: break;
     }
 }

 inline uint8_t OppositeRampTile(uint8_t tile) {
     switch (tile) {
         case TILE_RAMP_N: return TILE_RAMP_S;
         case TILE_RAMP_S: return TILE_RAMP_N;
         case TILE_RAMP_E: return TILE_RAMP_W;
         case TILE_RAMP_W: return TILE_RAMP_E;
         default: return tile;
     }
 }

 inline int RampArrowCodepoint(uint8_t tile) {
     return TILE_RENDER[tile].codepoint; // стрелки уже лежат в TILE_RENDER для каждого направления
 }


struct GameMap {
    static constexpr int Width = 80;
    static constexpr int Height = 45;
    // Пока небольшая фиксированная высота для теста Z. Позже это станет
    // размером чанка (16x16x256), когда дойдём до чанкования.
    // NOTE for AI: Чанк 16х16х256 был предложен Gemini, незнаю почему такой. Я изначально думал 16х16х16 под 4кб страницу памяти, хотя можно сделать конфигурируемым.
    static constexpr int Depth = 4;

    array<uint8_t, Width * Height * Depth> tileIds;

    // Новые, Z-осознанные аксессоры.
    uint8_t get(int x, int y, int z) const { return tileIds[(size_t)x + Width * ((size_t)y + Height * (size_t)z)]; }
    void set(int x, int y, int z, uint8_t id) { tileIds[(size_t)x + Width * ((size_t)y + Height * (size_t)z)] = id; }

    // Старые 2D-аксессоры сохранены как есть: работают с нижним слоем
    // (z = 0), чтобы MapEditor и формат map.dat пока не трогать —
    // многослойный редактор и сохранение придут вместе с чанкованием.
    uint8_t get(int x, int y) const { return get(x, y, 0); }
    void set(int x, int y, uint8_t id) { set(x, y, 0, id); }

    bool inBounds(int x, int y, int z) const {
        return x >= 0 && x < Width && y >= 0 && y < Height && z >= 0 && z < Depth;
    }
    bool isWalkable(int x, int y, int z) const {
        if (!inBounds(x, y, z)) return false;
        uint8_t tile = get(x, y, z);
        if (tile == TILE_AIR) return false; // воздух — это "нет пола", а не "пол не мешает"
        return !TILE_PHYSICS[tile].isSolid;
    }

    void initDefault() {
        tileIds.fill(TILE_AIR);

        // Нижний этаж (z = 0) — прежняя раскладка, без изменений.
        for (int y = 0; y < Height; ++y) {
            for (int x = 0; x < Width; ++x) {
                if (x == 0 || y == 0 || x == Width - 1 || y == Height - 1) {
                    set(x, y, 0, TILE_WALL);
                } else if (x == 10 && y == 10) {
                    set(x, y, 0, TILE_WATER);
                } else {
                    set(x, y, 0, TILE_FLOOR);
                }
            }
        }

        // --- Тест-зона №1: приподнятая платформа (auto-step вверх/вниз) ---
        // Пол z=0 под платформой заблокирован стеной ("постамент") —
        // поэтому на том же уровне пройти нельзя, и auto-step вынужден
        // поднять игрока на z=1. Снаружи платформы z=1 — воздух, так что
        // сход с неё точно так же авто-степом опускает обратно на z=0.
        for (int y = 18; y <= 22; ++y) {
            for (int x = 20; x <= 24; ++x) {
                set(x, y, 0, TILE_WALL);
                set(x, y, 1, TILE_FLOOR);
            }
        }

        // --- Тест-зона №2: шахта для явного лазанья (climb) ---
        // Колонка пола в центре на каждом уровне z=0..Depth-1, и вокруг
        // неё на z>=1 — маленькая комната со стенами, чтобы после подъёма
        // было видно, что вы куда-то попали, а не просто стоите в пустоте.
        for (int z = 0; z < Depth; ++z) {
            set(30, 10, z, TILE_FLOOR);
            if (z == 0) continue; // z=0 и так пол снаружи, комнату не рисуем
            for (int y = 8; y <= 12; ++y) {
                for (int x = 28; x <= 32; ++x) {
                    bool border = (x == 28 || x == 32 || y == 8 || y == 12);
                    set(x, y, z, border ? TILE_WALL : TILE_FLOOR);
                }
            }
        }
        // --- Тест-зона №3: направленная рампа + провал ---
        // Комната 7x7 на z=1 (стены по периметру, пол внутри). Вход —
        // рампа '►' (TILE_RAMP_E) на земле у западной стены: подойти к
        // ней можно только с запада (двигаясь на восток), затем ещё раз
        // "восток" со стоя на рампе — подъём в комнату. Спуск обратно —
        // просто идти на запад от входа: там пусто (место под рампой),
        // и fallThroughHole сам уронит игрока обратно на рампу — второй
        // тайл для спуска не нужен, один и тот же работает в обе стороны.
        for (int y = 29; y <= 35; ++y) {
            for (int x = 40; x <= 46; ++x) {
                bool border = (x == 40 || x == 46 || y == 29 || y == 35);
                set(x, y, 1, border ? TILE_WALL : TILE_FLOOR);
            }
        }
        set(40, 32, 1, TILE_FLOOR);   // проём в западной стене — площадка приземления
        set(39, 32, 0, TILE_RAMP_E);  // сама рампа, снаружи комнаты, на земле
        // (39,32,1) намеренно остаётся TILE_AIR — проём над рампой для подъёма/падения

        set(43, 31, 1, TILE_AIR); // отдельная дыра в полу — проверка обычного провала
    }
    bool saveToFile(const char* filename) const {
        // Пока сохраняем только нижний слой (z = 0) — формат map.dat
        // и MapEditor ещё не знают про Z, это отдельный следующий шаг.
        array<uint8_t, Width * Height> groundLayer;
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
                groundLayer[x + Width * y] = get(x, y, 0);
        unsigned int dataSize = static_cast<unsigned int>(groundLayer.size());
        return SaveFileData(filename, groundLayer.data(), dataSize);
    }
    bool loadFromFile(const char* filename) {
        int bytesRead = 0;
        uint8_t* loadedData = LoadFileData(filename, &bytesRead);
        if (!loadedData || bytesRead != Width * Height) {
            if (loadedData) UnloadFileData(loadedData);
            return false;
        }
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
                set(x, y, 0, loadedData[x + Width * y]);
        UnloadFileData(loadedData);
        return true;
    }

};



struct Player {
    int x = 1, y = 1, z = 0;
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

struct GameplayState : IGameState
{
    Player player;

    void OnEnter(Game& g) override;
    void OnUpdate(Game& g, float dt) override;
    void OnDraw(const Game& g, float dt) override;

private:
    void tryMoveHorizontal(Game& g, int dx, int dy);
    void tryClimb(Game& g, int dz);
    bool currentMoveDirection(int& dx, int& dy) const;

    static constexpr float kFastMoveInterval = 1.0f / 33.0f; // подобрано на глаз (было 1/3 — казалось медленным)
    float moveRepeatTimer = 0.0f;
    void tryAutoStep(Game& g, int dx, int dy);
    void tryAscendRamp(Game& g, uint8_t rampTile);
    void fallThroughHole(Game& g);
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
    GameplayState mGameplayState;
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




// Функция для отрисовки символа, принудительно вписанного в заданный квадрат
inline void DrawTextCodepointInBox(Font font, int codepoint, Vector2 pos, float boxSize, Color tint, Color bgTint, bool flipVertical = false)
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
    Rectangle sampleRec = srcRec;
    if (flipVertical) {
        sampleRec.height *= -1.0f; // стандартный приём raylib: отрицательная высота источника = вертикальный флип
    }
    DrawTexturePro(font.texture, sampleRec, destRec, origin, 0.0f, tint);
}

// Отрисовка одной клетки карты с учётом рамп: обычный тайл — один проход
// (как раньше), рампа — два прохода (стрелка направления, затем поверх
// значок лестницы), а пустота, под которой скрыта рампа, — тоже два
// прохода, но обратное направление и перевёрнутый значок лестницы (чисто
// визуальная подсказка про возможность спуститься/провалиться туда).
inline void DrawWorldTile(Font font, const GameMap& map, int x, int y, int z, Vector2 pos, float boxSize)
{
    uint8_t tileId = map.get(x, y, z);

    if (tileId == TILE_AIR) {
        if (z > 0) {
            uint8_t below = map.get(x, y, z - 1);
            if (IsRampTile(below)) {
                uint8_t reverseTile = OppositeRampTile(below);
                DrawTextCodepointInBox(font, RampArrowCodepoint(reverseTile), pos, boxSize, GOLD, BLANK);
                DrawTextCodepointInBox(font, kRampStairsCodepoint, pos, boxSize, GOLD, BLANK, /*flipVertical=*/true);
                return;
            }
        }
        const RenderDef& rdef = TILE_RENDER[TILE_AIR];
        DrawTextCodepointInBox(font, rdef.codepoint, pos, boxSize, rdef.fgColor, rdef.bgColor);
        return;
    }

    if (IsRampTile(tileId)) {
        DrawTextCodepointInBox(font, RampArrowCodepoint(tileId), pos, boxSize, GOLD, TILE_RENDER[tileId].bgColor);
        DrawTextCodepointInBox(font, kRampStairsCodepoint, pos, boxSize, GOLD, BLANK);
        return;
    }

    const RenderDef& rdef = TILE_RENDER[tileId];
    DrawTextCodepointInBox(font, rdef.codepoint, pos, boxSize, rdef.fgColor, rdef.bgColor);
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
            g.worldMap.loadFromFile("map.dat");
        }else{
            selectedItem = 1;
            setHint(press_again_hint);
        }
    }
    else if(IsKeyPressed(KEY_TWO)) {
        if(selectedItem == 2) {
            g.ChangeState(&g.mGameplayState);
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
    // Значки рамп: стрелки направления + символ лестницы (см. kRampStairsCodepoint).
    // ВНИМАНИЕ: сами глифы должны присутствовать в файле шрифта — то, что
    // мы их запросили здесь, не гарантирует, что unscii-8/JetBrains Mono
    // их реально содержат (особенно 0x1328D, иероглифический диапазон).
    codepoints[351] = 0x25B2; // ▲
    codepoints[352] = 0x25BC; // ▼
    codepoints[353] = 0x25BA; // ►
    codepoints[354] = 0x25C4; // ◄
    codepoints[355] = kRampStairsCodepoint; // 0x1328D

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
    g.unscii8t = load_font("assets/fonts/unscii-8-thin.ttf", 8);
    SetTextureFilter(g.unscii8t.texture, TEXTURE_FILTER_POINT);

    g.unscii8 = load_font("assets/fonts/unscii-8.ttf", 8);
    SetTextureFilter(g.unscii8.texture, TEXTURE_FILTER_POINT);

    g.unscii16 = load_font("assets/fonts/unscii-16.ttf", 16);
    SetTextureFilter(g.unscii16.texture, TEXTURE_FILTER_POINT);

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
    // 1. Выход из редактора по Ctrl+Q с сохранением
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_Q)) {
        g.worldMap.saveToFile("map.dat"); // Автосейв при выходе
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
    if (IsKeyPressed(KEY_FOUR)) selectedTileId = 4;

}



inline void MapEditor::OnDraw(const Game& g, float dt)
{
    ClearBackground(BLACK);

    float boxSize = g.current_font_size * g.zoom;

    // Автовыбор шрифта для карты мира (LOD)
    Font fontToUse = g.JetBrainsMonoNL_SemiBold;
    if (boxSize < 24.0f) {
        fontToUse = g.unscii8; // Растровый unscii-8 из вашего кода// for AI: не thin версия останется при малых зумах
    }

    // 1. Отрисовка карты мира
    for (int y = 0; y < GameMap::Height; y++) {
        for (int x = 0; x < GameMap::Width; x++) {
            Vector2 pos = { x * boxSize, y * boxSize };
            if (pos.x < GetScreenWidth() && pos.y < GetScreenHeight()) {
                uint8_t tileId = g.worldMap.get(x, y);
                const RenderDef& rdef = TILE_RENDER[tileId];
                DrawTextCodepointInBox(fontToUse, rdef.codepoint, pos, boxSize, rdef.fgColor, rdef.bgColor);
              }
        }
    }

    // 2. ОТРИСОВКА ФАНТОМНОЙ КИСТИ И КУРСОРA
    Vector2 cursorRemarksPos = { cursorX * boxSize, cursorY * boxSize };
    const RenderDef& currentBrush = TILE_RENDER[selectedTileId];

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
            DrawTextCodepointInBox(g.unscii16, TILE_RENDER[i].codepoint, iconPos, 16.0f, TILE_RENDER[i].fgColor, TILE_RENDER[i].bgColor);

            char itemBuf[256];
            snprintf(itemBuf, sizeof(itemBuf), "[%2c] - %s", 'A' + i, GetTileDescription(static_cast<TileId>(i)));

            DrawTextEx(g.unscii16, itemBuf, { (float)winX + 65, itemY }, 16, 0, itemColor);
        }

        DrawTextEx(g.unscii16, "Нажмите букву дважды для подтверждения", { (float)winX + 20, (float)winY + winH - 25 }, 16, 0, GRAY);
    }

    // 4. СТАТУС-БАР В САМОМ НИЗУ ЭКРАНА (ШРИФТ UNSCII-8)
    int screenH = GetScreenHeight();
    DrawRectangle(0, screenH - 20, GetScreenWidth(), 20, { 20, 20, 20, 255 });

    char statusBuf[256];
    snprintf(statusBuf, sizeof(statusBuf), "Координаты: [%2d, %2d] | Кисть: [%c] | Палитра: [~] | Сохр/Выход: [Ctrl+Q]",
             cursorX, cursorY, currentBrush.codepoint);

    DrawTextEx(g.unscii8, statusBuf, { 10, (float)screenH - 14 }, 8, 0, RAYWHITE);
}


inline void GameplayState::OnEnter(Game& g)
{
    // Стартовая позиция — угол нижнего этажа, рядом со стеной.
    player = Player{ 1, 1, 0 };
}

inline void GameplayState::tryMoveHorizontal(Game& g, int dx, int dy)
{
    // Если стоим на рампе и направление совпадает с направлением рампы —
    // это не обычный шаг, а подъём (см. tryAscendRamp).
    uint8_t currentTile = g.worldMap.get(player.x, player.y, player.z);
    if (IsRampTile(currentTile)) {
        int rdx, rdy;
        RampDirectionVector(currentTile, rdx, rdy);
        if (dx == rdx && dy == rdy) {
            tryAscendRamp(g, currentTile);
            return;
        }
    }

    int nx = player.x + dx;
    int ny = player.y + dy;

    if (g.worldMap.isWalkable(nx, ny, player.z)) {
        uint8_t targetTile = g.worldMap.get(nx, ny, player.z);
        if (IsRampTile(targetTile)) {
            // На рампу можно ступить только с "противоположной" стороны —
            // то есть двигаясь ровно в направлении самой рампы. С любой
            // другой стороны она ведёт себя как обычная стена.
            int rdx, rdy;
            RampDirectionVector(targetTile, rdx, rdy);
            if (dx != rdx || dy != rdy) {
                return;
            }
        }
        player.x = nx;
        player.y = ny;
        return;
    }

    // Пол на этом уровне отсутствует (воздух) — это не стена, а проём/дыра:
    // шагаем внутрь и падаем на первый пол снизу, а не просто блокируемся.
    if (g.worldMap.get(nx, ny, player.z) == TILE_AIR) {
        player.x = nx;
        player.y = ny;
        fallThroughHole(g);
        return;
    }

    // Иначе — стена или другое препятствие: движение не удаётся.
    // Потайные проходы за такими стенами ищутся через Ctrl+направление
    // (см. tryAutoStep) — обычная ходьба туда сама не "запрыгивает".
}

inline void GameplayState::tryAutoStep(Game& g, int dx, int dy)
{
    int nx = player.x + dx;
    int ny = player.y + dy;

    // Явный поиск прохода: пробуем текущий уровень, затем на уровень выше,
    // затем на уровень ниже. Только по Ctrl+направление — намеренное
    // действие игрока, а не побочный эффект обычной ходьбы.
    const int zCandidates[3] = { player.z, player.z + 1, player.z - 1 };
    for (int nz : zCandidates) {
        if (g.worldMap.isWalkable(nx, ny, nz)) {
            player.x = nx;
            player.y = ny;
            player.z = nz;
            return;
        }
    }
}

inline void GameplayState::tryAscendRamp(Game& g, uint8_t rampTile)
{
    int rdx, rdy;
    RampDirectionVector(rampTile, rdx, rdy);

    // Подняться можно только если прямо над рампой пусто — это место
    // намеренно держим свободным под проход/переход на клетку выше.
    if (g.worldMap.get(player.x, player.y, player.z + 1) != TILE_AIR) {
        return;
    }

    int lx = player.x + rdx;
    int ly = player.y + rdy;
    int lz = player.z + 1;
    if (!g.worldMap.isWalkable(lx, ly, lz)) {
        return; // наверху не на что приземлиться
    }

    player.x = lx;
    player.y = ly;
    player.z = lz;
}

inline void GameplayState::fallThroughHole(Game& g)
{
    // Ищем ближайший пол строго ниже текущего уровня; останавливаемся на
    // первом найденном полу, либо на дне карты (z=0), если пола вообще нет.
    int z = player.z - 1;
    while (z > 0 && !g.worldMap.isWalkable(player.x, player.y, z)) {
        --z;
    }
    player.z = z;
}

inline void GameplayState::tryClimb(Game& g, int dz)
{
    // Явное лазанье: перемещение строго по Z на месте, независимо от
    // авто-степа при ходьбе (например, подъём по шахте/лестнице).
    int nz = player.z + dz;
    if (g.worldMap.isWalkable(player.x, player.y, nz)) {
        player.z = nz;
    }
}

inline bool GameplayState::currentMoveDirection(int& dx, int& dy) const
{
    // То же сопоставление клавиш, что и в тап-версии движения — просто
    // отдаём направление, а не сразу двигаем, чтобы этим мог пользоваться
    // и Shift-режим с таймером повторов.
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) { dx = -1; dy = 0;  return true; }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) { dx = 1;  dy = 0;  return true; }
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) { dx = 0;  dy = -1; return true; }
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) { dx = 0;  dy = 1;  return true; }
    if (IsKeyDown(KEY_Q)) { dx = -1; dy = -1; return true; }
    if (IsKeyDown(KEY_E)) { dx = 1;  dy = -1; return true; }
    if (IsKeyDown(KEY_Z)) { dx = -1; dy = 1;  return true; }
    if (IsKeyDown(KEY_X)) { dx = 1;  dy = 1;  return true; }
    return false;
}

inline void GameplayState::OnUpdate(Game& g, float dt)
{
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_Q)) {
        g.ChangeState(&g.mMainMenu);
        return;
    }

    bool ctrlHeld = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool fastMove = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    if (ctrlHeld) {
        // Ctrl+направление: явный поиск прохода на уровень выше/ниже —
        // для потайных комнат за стенами. Обычная ходьба (и бег на Shift)
        // в стену теперь просто упирается — неочевидно, что туда можно
        // "запрыгнуть", так что это отдельное осознанное действие.
        // Примечание: Ctrl+Q зарезервирован под выход в меню, поэтому
        // северо-запад (Q) через Ctrl этим способом недоступен.
        moveRepeatTimer = 0.0f;
        if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) tryAutoStep(g, -1, 0);
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) tryAutoStep(g, 1, 0);
        if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) tryAutoStep(g, 0, -1);
        if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) tryAutoStep(g, 0, 1);
        if (IsKeyPressed(KEY_E)) tryAutoStep(g, 1, -1);
        if (IsKeyPressed(KEY_Z)) tryAutoStep(g, -1, 1);
        if (IsKeyPressed(KEY_X)) tryAutoStep(g, 1, 1);
    } else if (fastMove) {
        // Shift + направление удерживается: движение с фиксированной
        // скоростью 3 тайла/сек (не завязано на FPS), а не по одному тайлу
        // за нажатие. tryMoveHorizontal не меняется — она просто вызывается
        // по таймеру вместо однократного вызова по IsKeyPressed.
        int dx = 0, dy = 0;
        if (currentMoveDirection(dx, dy)) {
            moveRepeatTimer += dt;
            while (moveRepeatTimer >= kFastMoveInterval) {
                moveRepeatTimer -= kFastMoveInterval;
                tryMoveHorizontal(g, dx, dy);
            }
        } else {
            moveRepeatTimer = 0.0f;
        }
    } else {
        moveRepeatTimer = 0.0f;

        if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) tryMoveHorizontal(g, -1, 0);
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) tryMoveHorizontal(g, 1, 0);
        if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) tryMoveHorizontal(g, 0, -1);
        if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) tryMoveHorizontal(g, 0, 1);

        // Диагонали: Q/E/Z/X вокруг WASD (северо-запад/северо-восток/юго-запад/юго-восток).
        if (IsKeyPressed(KEY_Q)) tryMoveHorizontal(g, -1, -1); // северо-запад
        if (IsKeyPressed(KEY_E)) tryMoveHorizontal(g, 1, -1);  // северо-восток
        if (IsKeyPressed(KEY_Z)) tryMoveHorizontal(g, -1, 1);  // юго-запад
        if (IsKeyPressed(KEY_X)) tryMoveHorizontal(g, 1, 1);   // юго-восток
    }

    // Явное лазанье — отдельно от обычной ходьбы (шахты/лестницы).
    if (IsKeyPressed(KEY_PAGE_UP))   tryClimb(g, 1);
    if (IsKeyPressed(KEY_PAGE_DOWN)) tryClimb(g, -1);
}

inline void GameplayState::OnDraw(const Game& g, float dt)
{
    ClearBackground(BLACK);

    float boxSize = g.current_font_size * g.zoom;
    Font fontToUse = g.JetBrainsMonoNL_SemiBold;
    if (boxSize < 24.0f) {
        fontToUse = g.unscii8;
    }

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Камера: центрируем экран на игроке. Раньше карта рисовалась от
    // мирового (0,0) в пикселях экрана без привязки к игроку — всё, что
    // дальше от угла карты, чем размер окна (с учётом zoom), было
    // физически за пределами видимой области.
    float camOffsetX = screenW * 0.5f - (player.x + 0.5f) * boxSize;
    float camOffsetY = screenH * 0.5f - (player.y + 0.5f) * boxSize;

    // Рисуем только тот Z-уровень, на котором сейчас стоит игрок.
    for (int y = 0; y < GameMap::Height; y++) {
        for (int x = 0; x < GameMap::Width; x++) {
            Vector2 pos = { x * boxSize + camOffsetX, y * boxSize + camOffsetY };
            if (pos.x > -boxSize && pos.x < screenW && pos.y > -boxSize && pos.y < screenH) {
                DrawWorldTile(fontToUse, g.worldMap, x, y, player.z, pos, boxSize);
            }
        }
    }

    // Игрок всегда в центре экрана, поверх карты.
    Vector2 playerPos = { player.x * boxSize + camOffsetX, player.y * boxSize + camOffsetY };
    DrawTextCodepointInBox(fontToUse, '@', playerPos, boxSize, YELLOW, BLANK);

    DrawRectangle(0, screenH - 20, screenW, 20, { 20, 20, 20, 255 });
    char statusBuf[256];
    snprintf(statusBuf, sizeof(statusBuf), "Позиция: [%2d, %2d, %2d] | WASD/Стрелки: движение | QEZX: диагонали | Shift: бег | Ctrl+направление: потайной проход | PgUp/PgDn: лазанье | Выход: [Ctrl+Q]",
             player.x, player.y, player.z);
    DrawTextEx(g.unscii8, statusBuf, { 10, (float)screenH - 14 }, 8, 0, RAYWHITE);
}
