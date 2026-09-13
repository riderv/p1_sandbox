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
 // прохода отрисовки). 'H' — обычный ASCII, гарантированно есть в любом
 // загруженном шрифте (было 0x1328D, иероглиф-"лестница", но такого
 // глифа не нашлось в файлах unscii-8/JetBrains Mono — рендерился '?').
 constexpr int kRampStairsCodepoint = 'H'; // было 0x1328D (иероглиф-лестница) — глифа не оказалось в файле шрифта

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
        // Проверяем, что пространство для тела (z) и блок под ногами (z-1) в границах карты
        if (!inBounds(x, y, z) || !inBounds(x, y, z - 1)) return false;

        uint8_t bodyTile = get(x, y, z);     // Блок, где находится тело игрока
        uint8_t groundTile = get(x, y, z - 1); // Блок под ногами игрока

        // 1. Проверка пространства для тела (голова/туловище):
        // Там НЕ должно быть твёрдого блока (стены). Воздух или не-солид тайлы подходят.
        if (TILE_PHYSICS[bodyTile].isSolid) return false;

        // 2. Проверка опоры под ногами:
        // Под ногами НЕ должно быть пустого воздуха, иначе персонаж начнёт падать.
        // Там должен быть любой твёрдый или осязаемый блок (пол, стена нижнего этажа и т.д.).
        if (groundTile == TILE_AIR) return false;

        return true;
    }



    void initDefault() {
        tileIds.fill(TILE_AIR);

        // Расставляем прочные стены по всему периметру карты НА ВСЕХ уровнях Z (от 0 до Depth-1)
        for (int z = 0; z < Depth; ++z) {
            for (int y = 0; y < Height; ++y) {
                for (int x = 0; x < Width; ++x) {
                    if (x == 0 || y == 0 || x == Width - 1 || y == Height - 1) {
                        set(x, y, z, TILE_WALL);
                    } else if (z == 0) {
                        // На самом нижнем этаже (земле) всё остальное пространство заливаем полом
                        if (x == 10 && y == 10) {
                            set(x, y, 0, TILE_WATER);
                        } else {
                            set(x, y, 0, TILE_FLOOR);
                        }
                    }
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
        // Сдвинута ближе к спавну: центр теперь на клетке (5, 5).
        // Стены обнимают проём 3х3 тайла на всех ярусах z=0..3.
        for (int z = 0; z < Depth; ++z) {
            for (int y = 4; y <= 6; ++y) {
                for (int x = 4; x <= 6; ++x) {
                    if (x == 5 && y == 5) {
                        // В самом низу шахты (на земле) — прочный пол,
                        // а на верхних этажах — сквозной воздух колодца
                        set(x, y, z, (z == 0) ? TILE_FLOOR : TILE_AIR);
                    } else {
                        // Окружаем шахту прочными стенами на ВСЕХ ярусах z=0..3
                        set(x, y, z, TILE_WALL);
                    }
                }
            }
        }

        // --- Прорубаем честный воксельный вход в шахту на уровне глаз ---
        set(5, 6, 0, TILE_FLOOR); // Земля под дверью становится прочным полом для ног
        set(5, 6, 1, TILE_AIR);   // А на уровне z=1 убираем стену, заменяя её на воздух (проход)

        // --- Тест-зона №3: направленная рампа + провал ---
        // Сдвинута ближе к спавну: комната 5x5 на z=1 сдвинута в координаты (10-14, 3-7).
        // Вход — рампа '►' (TILE_RAMP_E) на земле у западной стены: подойти к
        // ней можно только с запада (двигаясь на восток).
        for (int y = 3; y <= 7; ++y) {
            for (int x = 10; x <= 14; ++x) {
                bool border = (x == 10 || x == 14 || y == 3 || y == 7);
                set(x, y, 1, border ? TILE_WALL : TILE_FLOOR);
            }
        }
        set(10, 5, 1, TILE_FLOOR);   // проём в западной стене комнаты на z=1 — площадка приземления
        set(9, 5, 0, TILE_RAMP_E);   // сама рампа, снаружи комнаты, на земле (z=0)
        // (9,5,1) намеренно остаётся TILE_AIR — проём над рампой для подъёма/падения

        // Северный вход: рампа смотрит на юг — заходишь и поднимаешься, двигаясь вниз по экрану.
        set(12, 3, 1, TILE_FLOOR);   // проём в северной стене комнаты на z=1
        set(12, 2, 0, TILE_RAMP_S);  // рампа снаружи на земле (z=0)

        // Южный вход: рампа смотрит на север — заходишь и поднимаешься, двигаясь вверх по экрану.
        set(12, 7, 1, TILE_FLOOR);   // проём в южной стене комнаты на z=1
        set(12, 8, 0, TILE_RAMP_N);  // рампа снаружи на земле (z=0)

        set(12, 5, 1, TILE_AIR);     // отдельная дыра в полу комнаты — проверка обычного провала
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
    int cameraZ = 0; // GEMINI: добавляем функционал управления камерой колесом мыши.


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
inline void DrawWorldTile(Font font, const GameMap& map, int x, int y, int z, Vector2 pos, float boxSize, int currentCameraZ)
{
    uint8_t tileId = map.get(x, y, z);

    // СИТУАЦИЯ А: Мы рендерим пустую клетку воздуха
    if (tileId == TILE_AIR) {
        // Если под этим воздухом лежит рампа, И наша камера находится ВЫШЕ этой пустой клетки
        // (То есть ракурс камеры позволяет заглянуть ВНУТРЬ проёма на спуск)
        if (z > 0 && currentCameraZ > z) {
            uint8_t below = map.get(x, y, z - 1);
            if (IsRampTile(below)) {
                uint8_t reverseTile = OppositeRampTile(below);
                // Рисуем ТОЛЬКО стрелку спуска (движение вниз по рампам)
                DrawTextCodepointInBox(font, RampArrowCodepoint(reverseTile), pos, boxSize, GOLD, BLANK);
                return;
            }
        }
        // Обычный воздух
        const RenderDef& rdef = TILE_RENDER[TILE_AIR];
        DrawTextCodepointInBox(font, rdef.codepoint, pos, boxSize, rdef.fgColor, rdef.bgColor);
        return;
    }

    // СИТУАЦИЯ Б: Мы рендерим саму рампу
    if (IsRampTile(tileId)) {
        // Условие Одиночной Стрелки:
        // Мы рисуем стрелку подъёма ТОЛЬКО если ракурс камеры находится на уровне самой рампы
        // или на уровне её проёма (currentCameraZ <= z + 1).
        // Если камера поднялась ЕЩЁ выше, то этот слой z будет полностью визуально заменён
        // стрелкой спуска, которую отрисует слой воздуха TILE_AIR над ней (см. Ситуацию А).
        if (currentCameraZ <= z + 1) {
            DrawTextCodepointInBox(font, RampArrowCodepoint(tileId), pos, boxSize, GOLD, TILE_RENDER[tileId].bgColor);
        } else {
            // Если мы смотрим совсем сверху, нижняя стрелка просто пропускается, чтобы не было наложения
            DrawTextCodepointInBox(font, ' ', pos, boxSize, BLANK, TILE_RENDER[tileId].bgColor);
        }
        return;
    }

    // Обычные блоки
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
    // Значки рамп: стрелки направления (см. TILE_RENDER). Символ лестницы
    // сам по себе теперь обычный ASCII ('H'), отдельно грузить не нужно —
    // уже покрыт диапазоном 32-126 выше.
    codepoints[351] = 0x25B2; // ▲
    codepoints[352] = 0x25BC; // ▼
    codepoints[353] = 0x25BA; // ►
    codepoints[354] = 0x25C4; // ◄

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
    // Воксельный сдвиг: тело игрока теперь находится на уровне z = 1 (воздух).
    // Прочный пол TILE_FLOOR, созданный в initDefault(), лежит под его ногами на z = 0.
    player = Player{ 1, 1, 1 };

    // Камера сразу фокусируется на уровне глаз игрока
    cameraZ = player.z;
}
inline void GameplayState::tryMoveHorizontal(Game& g, int dx, int dy)
{
    int nx = player.x + dx;
    int ny = player.y + dy;

    if (nx < 0 || nx >= GameMap::Width || ny < 0 || ny >= GameMap::Height) return;

    // 1. ПРОВЕРКА: Стоим ли мы уже на рампе прямо сейчас?
    uint8_t currentUnderfootTile = g.worldMap.get(player.x, player.y, player.z - 1);
    if (IsRampTile(currentUnderfootTile)) {
        int rdx, rdy;
        RampDirectionVector(currentUnderfootTile, rdx, rdy);
        if (dx == rdx && dy == rdy) {
            tryAscendRamp(g, currentUnderfootTile);
            return;
        }
    }

    // 2. ПРОВЕРКА: Пытаемся ли мы зайти на клетку с рампой?
    if (g.worldMap.inBounds(nx, ny, player.z - 1)) {
        uint8_t targetUnderfootTile = g.worldMap.get(nx, ny, player.z - 1);
        if (IsRampTile(targetUnderfootTile)) {
            int rdx, rdy;
            RampDirectionVector(targetUnderfootTile, rdx, rdy);
            if (dx != rdx || dy != rdy) return; // Блокируем шаг сбоку
        }
    }

    // 3. ОБЫЧНОЕ ДВИЖЕНИЕ: Стандартная воксельная проходимость
    if (g.worldMap.isWalkable(nx, ny, player.z)) {
        player.x = nx;
        player.y = ny;
        return;
    }

    // 4. ПРОВЕРКА НА ОБРЫВ/ДЫРУ: Свободно перед лицом, но пусто под ногами
    uint8_t targetBody = g.worldMap.get(nx, ny, player.z);
    uint8_t targetGround = g.worldMap.get(nx, ny, player.z - 1);

    if (!TILE_PHYSICS[targetBody].isSolid && targetGround == TILE_AIR) {
        player.x = nx;
        player.y = ny;
        fallThroughHole(g);
        return;
    }

    // Мягкое завершение: если ни одно условие не выполнено, игрок просто упирается в препятствие
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
            // Если на проверяемой высоте nz под ногами персонажа (на nz - 1) окажется рампа,
            // запрещаем авто-степу запрыгивать туда. По рампам можно ходить только честно через WASD!
            if (g.worldMap.inBounds(nx, ny, nz - 1)) {
                uint8_t groundTile = g.worldMap.get(nx, ny, nz - 1);
                if (IsRampTile(groundTile)) {
                    continue; // Пропускаем этого кандидата по высоте
                }
            }
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

    // Целевая точка для тела игрока (комбинированное смещение Клода):
    // Шаг вперёд по направлению стрелки рампы и подъём на +1 уровень по воксельной оси Z
    int lx = player.x + rdx;
    int ly = player.y + rdy;
    int lz = player.z + 1; // Новая высота тела/глаз игрока

    if (!g.worldMap.inBounds(lx, ly, lz)) return;

    // 1. Проверяем свободное место для тела/головы на новой высоте (lz)
    uint8_t targetBody = g.worldMap.get(lx, ly, lz);
    if (TILE_PHYSICS[targetBody].isSolid) return; // Упёрлись головой в потолок

    // 2. Проверяем, на что встанут ноги на новой клетке.
    // Ноги окажутся на уровне lz - 1 (что физически равно старому player.z).
    // Там должен быть прочный, проходимый пол верхнего этажа.
    uint8_t targetGround = g.worldMap.get(lx, ly, lz - 1);
    if (targetGround == TILE_AIR) {
        return; // Наверху провал/пустота, наступить не на что
    }

    // Перемещаем персонажа (комбинированный шаг Клода выполнен)
    player.x = lx;
    player.y = ly;
    player.z = lz;
}


inline void GameplayState::fallThroughHole(Game& g)
{
    // Летим вниз, пока под ногами (на уровне z - 1) находится воздух (TILE_AIR)
    // Останавливаемся, как только z - 1 упрётся в прочный блок или в дно карты (z = 1)
    int z = player.z;

    while (z > 1 && g.worldMap.get(player.x, player.y, z - 1) == TILE_AIR) {
        --z;
    }

    player.z = z;
}

inline void GameplayState::tryClimb(Game& g, int dz)
{
    // 1. Проверяем наличие стен для опоры на текущем уровне тела игрока (player.z)
    bool hasWallToHold = false;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            if (g.worldMap.get(player.x + dx, player.y + dy, player.z) == TILE_WALL) {
                hasWallToHold = true;
                break;
            }
        }
        if (hasWallToHold) break;
    }

    if (!hasWallToHold) return;

    int nz = player.z + dz;
    // Ограничиваем движение: nz (тело) не может упасть ниже 1 (так как z=0 зарезервирован под блоки земли)
    if (nz < 1 || nz >= GameMap::Depth) return;

    // 2. Проверяем свободное пространство для перемещения тела:
    uint8_t targetBodyTile = g.worldMap.get(player.x, player.y, nz);
    if (TILE_PHYSICS[targetBodyTile].isSolid) {
        return; // Путь заблокирован сплошным блоком (потолок или стена перед лицом)
    }

    // 3. Если всё свободно — перемещаем тело персонажа на новый воксельный уровень
    player.z = nz;
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

    // GEMINI: Управление высотой камеры с помощью колёсика мыши
    float wheel = GetMouseWheelMove();
    if (wheel > 0.0f) {
        if (cameraZ < GameMap::Depth - 1) cameraZ++;
    } else if (wheel < 0.0f) {
        if (cameraZ > 0) cameraZ--;
    }

    // GEMINI: Сброс камеры на уровень игрока по нажатию F1 или колёсика мыши
    if (IsKeyPressed(KEY_F1) || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        cameraZ = player.z;
    }

    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_Q)) {
        g.ChangeState(&g.mMainMenu);
        return;
    }

    bool ctrlHeld = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool fastMove = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    int oldZ = player.z; // Запоминаем Z до начала движения
    int dx = 0, dy = 0;
    bool moved = false;

    if (ctrlHeld) {
        moveRepeatTimer = 0.0f;
        if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) { tryAutoStep(g, -1, 0); moved = true; }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { tryAutoStep(g, 1, 0);  moved = true; }
        if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) { tryAutoStep(g, 0, -1); moved = true; }
        if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) { tryAutoStep(g, 0, 1);  moved = true; }
        if (IsKeyPressed(KEY_E)) { tryAutoStep(g, 1, -1); moved = true; }
        if (IsKeyPressed(KEY_Z)) { tryAutoStep(g, -1, 1); moved = true; }
        if (IsKeyPressed(KEY_X)) { tryAutoStep(g, 1, 1);  moved = true; }
    } else if (fastMove) {
        if (currentMoveDirection(dx, dy)) {
            moved = true; // Бег запущен
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
        if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) { dx = -1; dy = 0;  moved = true; }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { dx = 1;  dy = 0;  moved = true; }
        if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) { dx = 0;  dy = -1; moved = true; }
        if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) { dx = 0;  dy = 1;  moved = true; }

        if (IsKeyPressed(KEY_Q)) { dx = -1; dy = -1; moved = true; }
        if (IsKeyPressed(KEY_E)) { dx = 1;  dy = -1; moved = true; }
        if (IsKeyPressed(KEY_Z)) { dx = -1; dy = 1;  moved = true; }
        if (IsKeyPressed(KEY_X)) { dx = 1;  dy = 1;  moved = true; }

        if (moved) {
            tryMoveHorizontal(g, dx, dy);
        }
    }

    // Явное лазанье (PageUp / PageDown)
    if (IsKeyPressed(KEY_PAGE_UP))   { tryClimb(g, 1);  moved = true; }
    if (IsKeyPressed(KEY_PAGE_DOWN)) { tryClimb(g, -1); moved = true; }

    // В самом конце OnUpdate, перед закрывающей скобкой }
    // Если игрок совершил движение и его физический уровень Z изменился (рампа/падение),
    // камера автоматически следует за ним.
    if (moved && player.z != oldZ) {
        cameraZ = player.z;
    }

}

inline void GameplayState::OnDraw(const Game& g, float dt)
{
    ClearBackground(BLACK);

    float boxSize = g.current_font_size * g.zoom;
    Font fontToUse = (boxSize < 24.0f) ? g.unscii8 : g.JetBrainsMonoNL_SemiBold;

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    float camOffsetX = screenW * 0.5f - (player.x + 0.5f) * boxSize;
    float camOffsetY = screenH * 0.5f - (player.y + 0.5f) * boxSize;

    constexpr int kSliceDepth = 5;
    int topZ = cameraZ;
    int bottomZ = cameraZ - kSliceDepth;
    if (bottomZ < 0) bottomZ = 0;

    const Color kVeilColor = { 40, 120, 220, 90 };

    // 1. ПОСЛОЙНЫЙ РЕНДЕРИНГ КАРТЫ (Снизу вверх)
    for (int z = bottomZ; z <= topZ; ++z) {
        for (int y = 0; y < GameMap::Height; y++) {
            for (int x = 0; x < GameMap::Width; x++) {
                Vector2 pos = { x * boxSize + camOffsetX, y * boxSize + camOffsetY };

                if (pos.x <= -boxSize || pos.x >= screenW || pos.y <= -boxSize || pos.y >= screenH) {
                    continue;
                }

                // Проверка перекрытия (Occlusion Check)
                bool isHidden = false;
                for (int checkZ = z + 1; checkZ <= topZ; ++checkZ) {
                    if (g.worldMap.get(x, y, checkZ) != TILE_AIR) {
                        isHidden = true;
                        break;
                    }
                }
                if (isHidden) continue;

                // Рисуем тайл текущего слоя
                DrawWorldTile(fontToUse, g.worldMap, x, y, z, pos, boxSize, cameraZ);


                // ИСПРАВЛЕНИЕ ВУАЛИ:
                // Уровень под ногами камеры (cameraZ - 1) — это базовая земля, она рисуется БЕЗ вуали.
                // Вуаль ложится ТОЛЬКО на слои, которые находятся СТРОГО ниже этого уровня (z < cameraZ - 1).
                if (z < cameraZ - 1) {
                    DrawRectangle((int)pos.x, (int)pos.y, (int)boxSize, (int)boxSize, kVeilColor);
                }
            }
        }
    }

    // 2. ОТРИСОВКА ПЕРСОНАЖА @ (С воксельной вуалью глубины и прозрачностью над потолком)
    // Рисуем игрока, если его тело попадает в диапазон видимости камеры, ЛИБО если он находится выше среза камеры (над головой)
    if (player.z >= bottomZ) {
        Vector2 playerPos = { player.x * boxSize + camOffsetX, player.y * boxSize + camOffsetY };
        Color playerColor = YELLOW;

        if (player.z < cameraZ) {
            // СИТУАЦИЯ А: Персонаж находится ГЛУБЖЕ камеры (под ногами)
            // Накладываем синюю вуаль глубины
            int playerDepthDiff = cameraZ - player.z;
            float blend = playerDepthDiff * 0.35f;
            if (blend > 0.85f) blend = 0.85f;

            playerColor.r = (unsigned char)(YELLOW.r * (1.0f - blend) + kVeilColor.r * blend);
            playerColor.g = (unsigned char)(YELLOW.g * (1.0f - blend) + kVeilColor.g * blend);
            playerColor.b = (unsigned char)(YELLOW.b * (1.0f - blend) + kVeilColor.b * blend);
        }
        else if (player.z > cameraZ) {
            // СИТУАЦИЯ Б: Персонаж находится ВЫШЕ камеры (над головой / на потолке)
            // Делаем его полупрозрачным фантомом, чтобы игрок понимал, где находится тело
            playerColor = Fade(YELLOW, 0.45f); // Снижаем альфа-канал до 45% (около 115 из 255)
        }

        // Рисуем символ игрока
        DrawTextCodepointInBox(fontToUse, '@', playerPos, boxSize, playerColor, BLANK);
    }

    // 3. СТАТУС-БАР
    DrawRectangle(0, screenH - 20, screenW, 20, { 20, 20, 20, 255 });
    char statusBuf[256];
    snprintf(statusBuf, sizeof(statusBuf),
             " Поз: [%2d, %2d, %2d] | Срез камеры Z: %d | Колёсико: осмотр высот | F1/Клик мыши: сброс камеры | Выход: [Ctrl+Q]",
             player.x, player.y, player.z, cameraZ);
    DrawTextEx(g.unscii8, statusBuf, { 10, (float)screenH - 14 }, 8, 0, RAYWHITE);
}

