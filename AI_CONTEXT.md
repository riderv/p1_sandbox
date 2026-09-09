# AI Context for p1_sandbox

## Core Tech Stack
* **Language:** C++23 
* **Flags:** `-fno-exceptions`, `-fno-rtti` (Strict compile-time errors, memory safety via `std::nothrow` / pointer checks).
* **Framework:** Raylib 6.0+ (pure C integration, pixel graphics focus).
* **Target Layout:** Persistent world simulation, custom voxel framework foundations.

## Renderer Customizations (`DrawTextCodepointInBox`)
* Renders individual characters into exact square tile grids (`boxSize` x `boxSize`).
* Features **Pixel Snapping** (`std::floor`) to prevent sub-pixel blur on zoom.
* **LOD System:** Vector fonts (`JetBrains Mono`) for high zooms (adds custom compression `widthRatio = 0.85f`), pixel-art raster fonts (`unscii8`) with hardcoded 1px borders for small zooms to prevent visual font gluing.

## Data Structures
* **Flyweight Pattern for Tiles:** `GameMap` holds a flat un-fragmented `std::array<uint8_t, 80 * 45>` of `TileId`s. All visual/physical semantics (codepoints, Colors, collisions, and localization descriptions via `TileDescId`) are mapped instantly from a static database array `TILE_DATABASE`.
* **State Management:** Stated screens are allocated inside the global `Game` struct as local fields (`mMainMenu`, `mMapEditor`), using C++ standard abstract classes (`IGameState`) with virtual functions (no exceptions runtime cost, standard vtable).
* **Map Serialization:** Custom `saveToFile` / `loadFromFile` via Raylib's binary standard functions `SaveFileData` / `LoadFileData` mapping flat array data straight to `map.dat`.

## MapEditor UX & Features
* Left-click mouse drawing, right-click eraser.
* **Nikon-Style Double Confirmation:** Pressing palette shortcuts `[A]`, `[B]`, `[C]`, `[E]` once highlights the row, pressing the same character key the second time locks the brush selection and closes the layout window.
* Full Russian Cyrillic support natively baked into raster custom fonts.

## CURRENT TODO LIST / BACKLOG
1. **[BUG] Исправить управление зумом в Главном Меню:** В `MainMenu::OnUpdate` сейчас некорректно обрабатывается изменение зума (мозг подтормаживал, нужно перепроверить влияние глобального `Game::Update` на локальный стейт меню).
2. **[LOD] Заменить `g.unscii8` на `g.unscii8t` (thin):** При падении зума в редакторе ниже порога переключать карту на тонкую версию растрового шрифта для лучшей читаемости сетки.
3. **[Feature] Глобальная палитра сущностей:** Перевести жестко зашитые названия тайлов из `MapEditor::OnDraw` на динамический вывод из справочника `TILE_DESCRIPTIONS` (уже подготовлена база через `DescID`).
4. **[Feature] Создание GameplayState:** Написать третий игровой стейт для полноценного игрового процесса, где персонаж `@` сможет физически ходить по сгенерированной карте, а стены `TILE_WALL` будут блокировать его движение (`isSolid`).
