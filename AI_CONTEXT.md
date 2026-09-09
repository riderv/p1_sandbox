# AI Context for p1_sandbox

## Core Tech Stack
* **Language:** C++23
* **Flags:** `-fno-exceptions`, `-fno-rtti` (Strict compile-time errors, memory safety via `std::nothrow` / pointer checks).
* **Framework:** Raylib 6.0+ (pure C integration, pixel graphics focus).
* **Target Layout:** Persistent world simulation, custom voxel framework foundations.
* **Scale target:** hobby project, up to ~50 concurrent players, single server process. No sharding, no strict anti-cheat requirements.

## Renderer Customizations (`DrawTextCodepointInBox`)
* Renders individual characters into exact square tile grids (`boxSize` x `boxSize`).
* Features **Pixel Snapping** (`std::floor`) to prevent sub-pixel blur on zoom.
* **LOD System:** Vector fonts (`JetBrains Mono`) for high zooms (adds custom compression `widthRatio = 0.85f`), pixel-art raster fonts (`unscii8`) with hardcoded 1px borders for small zooms to prevent visual font gluing.
  * `g.unscii8` = `unscii-8.ttf` (regular). `g.unscii8t` = `unscii-8-thin.ttf` (thin variant). **Note:** these names were swapped relative to file content until the 2026-09 font-load fix — if anything downstream ever assumed the old (swapped) mapping, re-check it.

## Data Structures
* **Tile data is split by concern, not bundled:**
  * `RenderDef { int codepoint; Color fgColor; Color bgColor; }` — indexed by `TileId` via `TILE_RENDER[TILE_COUNT]`. Positional init (table-like, reads as a spreadsheet).
  * `PhysicsDef { bool isSolid; }` — indexed by `TileId` via `TILE_PHYSICS[TILE_COUNT]`. Designated init (`{ .isSolid = true }`) — kept explicit on purpose since bare bools are unreadable positionally, especially as this struct grows more flags (`isOpaque`, `isSwimmable`, etc. planned).
  * Tile descriptions: **no separate `TileDescId` enum** — that was a redundant indirection over `TileId` itself. Access via `GetTileDescription(TileId id)`, which currently reads a static `TILE_DESC_STRINGS[TILE_COUNT]` array but is designed to move to a file/DB later without changing call sites.
  * `GameMap` still stores only `array<uint8_t, Width*Height> tileIds` (flat, single Z layer, `Width=80, Height=45`) — flyweight pattern preserved, all semantics looked up via `TileId` in the tables above. `map.dat` format unchanged (raw `TileId` bytes).
* **State Management:** Screens allocated inside the global `Game` struct as local fields (`mMainMenu`, `mMapEditor`), using `IGameState` abstract base with virtual functions (no exceptions runtime cost, standard vtable).
* **Map Serialization:** Custom `saveToFile` / `loadFromFile` via Raylib's binary functions `SaveFileData` / `LoadFileData`, flat array data mapped straight to `map.dat`.

## MapEditor UX & Features
* Left-click mouse drawing, right-click eraser.
* Hotkeys `1`-`4` select tiles `TILE_FLOOR`/`TILE_WALL`/`TILE_WATER`/`TILE_DOOR` directly.
* **Nikon-Style Double Confirmation:** Pressing palette shortcuts `[A]`-`[E]` once highlights the row, pressing the same key again locks the brush and closes the palette window.
* Full Russian Cyrillic support natively baked into raster custom fonts.
* Palette tile names now render dynamically via `GetTileDescription()` — no hardcoded strings in `MapEditor::OnDraw`.

## Planned Architecture (not yet implemented — design decisions locked in)
* **3D migration path:** world coordinates will move from 2D (`x,y`) to `WorldPos{x,y,z}` with chunked storage (target `16x16x256` per chunk, smaller Z for now). `TileId`/`RenderDef`/`PhysicsDef` split exists specifically so the render layer can be swapped for 3D meshes later without touching simulation or save-format code.
* **Feature order:** (1) add Z-coordinate + player movement/collision via a new `GameplayState` (player `@` walks the map, `TILE_PHYSICS[id].isSolid` blocks movement) — this needs Z added first; (2) stateful world objects (doors open/close) — both of these are the "material" the networking layer will need to synchronize; (3) networking/multiplayer.
* **Networking design (planned, not built yet):** server-authoritative, MUD-style — client sends minimal intent commands only, server holds full world state, client receives only what's in its FOV. Command-queue + tick model, tick every 300-500ms (turn-based/semi-turn-based gameplay, latency-tolerant by design). State sync approach: chunk-version-counter hybrid (client tracks `lastKnownVersion` per chunk; server resends a chunk only when its version is newer) rather than pure full-snapshot or pure event-delta sync.

## CURRENT TODO LIST / BACKLOG
1. **[BUG] Зум в Главном Меню:** `MainMenu::OnUpdate` всё ещё некорректно обрабатывает зум (влияние глобального `Game::Update` на локальный стейт меню) — не разбирали, актуально.
2. **[LOD] Уточнение по шрифтам:** после фикса имён (`unscii8`/`unscii8t` теперь соответствуют файлам верно) — `MapEditor::OnDraw` при низком зуме переключается на `g.unscii8` (строка с комментарием `// Растровый unscii-8-thin...`), но это уже **не** thin-версия после фикса. Нужно: (a) поправить логику на `g.unscii8t`, если по задумке при малом зуме должен использоваться именно thin-шрифт для читаемости сетки, (b) поправить сам устаревший комментарий.
3. ~~[Feature] Глобальная палитра сущностей~~ — **done**: названия тайлов теперь берутся из `GetTileDescription()`, ничего не захардкожено.
4. **[Feature] GameplayState:** следующий шаг — Z-координата + движение персонажа `@` по карте с коллизией по `TILE_PHYSICS[id].isSolid`.
