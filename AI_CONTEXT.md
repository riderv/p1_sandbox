	# AI Context for p1_sandbox

## Repo layout
`src/`: `game.hpp`, `main.cpp`, `pch.h`, `talloc.c`/`talloc.h` (a downloaded hierarchical memory allocator library, not yet relevant). Root: `AI_CONTEXT.md`, `CMakeLists.txt`. `assets/fonts/`: `unscii-8.ttf`, `unscii-8-thin.ttf`, `unscii-16.ttf`, `JetBrainsMonoNL-SemiBold.ttf`, `JetBrainsMono-SemiBoldItalic.ttf`, `FSEX302.ttf`, `PressStart2P.ttf`.

## Core Tech Stack
* C++23, `-fno-exceptions`, `-fno-rtti`, Raylib 6.0+. Hobby project, ~50 concurrent players target, single server process eventually.
* **Error handling:** no `noexcept` anywhere — early crash/abort on unrecoverable errors, or explicit `bool`/`nullptr`/return-code checks otherwise.

## Renderer Customizations (`DrawTextCodepointInBox`)
* Fixed-box glyph rendering with pixel snapping and font-size LOD (vector font at high zoom, `unscii8` regular — not thin — at low zoom).
* Has an optional `flipVertical` parameter (defaults `false`): copies `srcRec` into a separate `sampleRec` before negating its `height` for `DrawTexturePro` — negating the original `srcRec` directly would corrupt `destRec` sizing computed earlier in the function.
* **Font codepoint loading only covers ASCII (32-126) + Cyrillic (0x400+) by default.** Ramp direction arrows (`▲▼►◄` = `0x25B2/0x25BC/0x25BA/0x25C4`) were added to the loaded set and render fine. **The stairs glyph does NOT use an extra codepoint anymore** — see Data Structures below; a hieroglyph attempt (`0x1328D`) rendered as `?` despite looking fine in Qt Creator, because Qt uses OS font-fallback substitution while this raylib pipeline bakes glyphs from one specific TTF file with no fallback. Lesson: a character displaying correctly in some other app doesn't mean the game's actual font files contain that glyph.
* **`g.zoom` is shared global state on `Game`**, leaks between states; harmless in `GameplayState` (player-centered camera), root cause in `MainMenu::OnUpdate` still not fixed.
* **Composite multi-slice rendering — implemented this session** (see GameplayState below).
* **Still planned, not started:** mouse-wheel camera height scrubbing (view a different Z window than the player's own), wheel-click or `F1` resets to the player's level.

## Data Structures
* `RenderDef`/`PhysicsDef` indexed by `TileId`.
* `TileId`: `TILE_AIR`, `TILE_FLOOR`, `TILE_WALL`, `TILE_WATER`, `TILE_DOOR`, `TILE_RAMP_N/S/E/W` (4 directional ramps, each tile's direction = the direction that ascends it). All ramps non-solid, gold, arrow-shaped in `TILE_RENDER`.
* Ramp helpers (free functions, right after `GetTileDescription`): `IsRampTile`, `RampDirectionVector`, `OppositeRampTile`, `RampArrowCodepoint`, and `kRampStairsCodepoint`. **`kRampStairsCodepoint` is now `'H'`** (plain ASCII, always available) — was `0x1328D`, changed after the font-fallback issue above. Note `'H'` looks the same flipped or not, so the `flipVertical` effect on it is currently invisible; swap to something asymmetric (e.g. Cyrillic `Ш`/`Ж`, already loaded) if that visual distinction matters later.
* `GameMap`: `Depth = 4` (stepping stone, not final chunking). `isWalkable` excludes `TILE_AIR` explicitly before checking `isSolid`.
* `DrawWorldTile(font, map, x, y, z, pos, boxSize)` (next to `DrawTextCodepointInBox`): normal tile = one draw; ramp tile = two-pass (arrow, then `kRampStairsCodepoint` on top); `TILE_AIR` whose `z-1` neighbor is a ramp = two-pass reversed (opposite arrow + flipped stairs glyph) as a purely visual "you can step/fall through here" hint.
* Persistence: `saveToFile`/`loadFromFile` still only `z=0`; SQLite still just under consideration.
* `Player { int x = 1, y = 1, z = 0; }`.

## GameplayState
* Movement: WASD/arrows/QEZX same-level only; Shift = continuous run (`kFastMoveInterval = 1/33s`, `while`-loop accumulator); Ctrl+direction = `tryAutoStep` (old `{z,z+1,z-1}` search, now exclusively a deliberate "secret passage" action); PageUp/PageDown = `tryClimb` (unchanged, vertical shafts/ladders).
* **Ramps (directional, reworked previously):** entering a ramp tile requires approaching in its own direction (blocked otherwise, like a wall). Standing on a ramp and pressing that same direction again → `tryAscendRamp`: requires `TILE_AIR` directly above the ramp (headroom) and a walkable landing cell one step further + one Z up. **Descending needs no separate tile** — walking back into the open air above a ramp triggers the existing `fallThroughHole` path, which drops the player straight back onto the ramp.
* **Composite rendering — new this session:** `OnDraw` now draws `player.z - 5` through `player.z` per column, bottom-to-top (so upper opaque tiles correctly occlude lower ones, and openings let lower layers show through). Every layer below the player's own (`z < player.z`) gets a translucent blue rectangle overlay (`kVeilColor = {40,120,220,90}`) drawn on top of it, so it reads as "underfoot, not your level." Nothing is drawn below `player.z - 5` — already black from `ClearBackground(BLACK)`, so no extra fill was needed. `kSliceDepth = 5` and `kVeilColor` are both easy to retune if the look needs adjusting.
* Three procedural test areas in `initDefault()`:
  1. Platform `(20-24,18-22)` — `Ctrl+direction` secret-passage test.
  2. Climb shaft `(30,10)` — `PageUp`/`PageDown` test.
  3. **Ramp room `(40-46,29-35)` on `z=1`, now with 3 entrances** (extended this session): west `TILE_RAMP_E` at `(39,32,0)`, north `TILE_RAMP_S` at `(43,28,0)` (wall opening at `(43,29,1)`), south `TILE_RAMP_N` at `(43,36,0)` (wall opening at `(43,35,1)`). All three ascend/descend the same way (see ramp mechanic above). Separate isolated hole at `(43,31,1)` for plain fall-through-hole testing.
* **Status: mostly playtested and confirmed working this session** — ramps (all 3 entrances), the stairs-glyph font issue (found and fixed), and composite rendering with the veil were all just implemented; composite rendering specifically has not been confirmed hands-on yet (implemented right before the session's token limit hit).

## Planned Architecture (unchanged since last update, still not implemented)
* True-voxel world model confirmed (1×1×1 tiles, floor = solid slab + open layer above). 3D migration to chunked `WorldPos{x,y,z}` still pending; `Depth=4` is a stepping stone. Chunk size open (`16x16x16` vs `16x16x256` vs configurable).
* Doors: map objects (separate layer), not a `TileId`. Not started.
* Networking: server-authoritative MUD-style, planned, not built.
* Persistence: SQLite under consideration vs current flat `map.dat`.
* Android port: wants touch-friendly UI eventually, gameplay not hard-tied to keyboard. `GameplayState::OnUpdate` now has three modifier-gated input branches (plain/Shift/Ctrl) — growing coupling to watch before an input-abstraction layer becomes necessary.

## CURRENT TODO LIST / BACKLOG
1. **[VERIFY] Playtest composite rendering:** confirm the veil appears correctly on levels below the player, layering/occlusion looks right through openings, and performance is fine iterating 6 Z-layers × 80×45 tiles per frame.
2. **[VERIFY] Playtest ramps end-to-end** (should mostly already be confirmed): all 3 entrances to the `(40-46,29-35)` room, plus the standalone hole at `(43,31,1)`.
3. **[NEXT] Mouse-wheel camera height:** scrub the displayed slice window; wheel-click or `F1` resets to the player's level.
4. **[BUG] Shared `zoom` on `Game`:** still leaks between states; root cause in `MainMenu::OnUpdate` not fixed.
5. **[Feature] Doors:** map objects (separate layer), not started. No MapEditor hotkey exists yet for painting ramp tiles either — still procedural-only.
6. **[Design] Input abstraction for Android:** `GameplayState::OnUpdate`'s three modifier branches are the coupling to watch.
7. **[Design] Chunk size** and **[Design] Persistence (SQLite vs flat file)** — both still open decisions.

##Критика от Google Gemini:
В чём проблемы реализации Клода?1. Серьезная просадка FPS (Критический баг O(N³))Посмотрите на порядок вложенности циклов в его OnDraw
for (int y = 0; y < GameMap::Height; y++) {
    for (int x = 0; x < GameMap::Width; x++) {
        // ... проверка отсечения ...
        for (int z = bottomZ; z <= topZ; ++z) {
            DrawWorldTile(...); // ❌ Отрисовка И вуаль вызываются по Z для КАЖДОЙ плитки x,y
            if (z < topZ) {
                DrawRectangle(..., kVeilColor); 
            }
        }
    }
}
Используйте код с осторожностью.Почему это баг: Вместо того чтобы нарисовать один слой целиком, а потом поверх него рисовать следующий, Клод итерируется по координате (x, y) и внутри нее запускает цикл по Z.К чему это приведет: Из-за этого вызовы DrawTextCodepointInBox и DrawRectangle ломают кэш текстурных батчей Raylib для каждого тайла. Вместо нескольких эффективных батчей отрисовки на слой, видеокарта делает тысячи переключений состояний (Draw Calls) на каждый чих. Игра начнет сильно лагать, как только карта заполнится тайлами.

 В чём корень проблемы и как её исправить?В коде Клода циклы идут так: X ➔ Y ➔ Z. То есть для плитки (0,0) вызывается отрисовка Z=0, Z=1, Z=2. Потом для плитки (1,0) снова Z=0, Z=1, Z=2.Внутри DrawWorldTile для каждого прохода (DrawTextCodepointInBox) движок Raylib пытается активировать текстуру шрифта. Из-за постоянного переключения между координатами и слоями Z внутренний батчер Raylib (rlDrawRenderBatch) принудительно сбрасывает геометрию на видеокарту сотни раз за кадр.Решение: Мы меняем порядок циклов на Z ➔ Y ➔ X.Мы отрисуем сначала весь слой z = 0 (все его тайлы), затем наложим синюю вуаль на весь экран (или только на те клетки, где рендерился этот слой), затем перейдём к z = 1 и так далее. Для Raylib это идеальный паттерн: он сможет сгруппировать отрисовку тысяч символов одного слоя в один единственный Draw Call.Плюс ко всему, мы интегрируем сюда управление свободной камерой по Z, которое мы отладили ранее (ведь Клод его не написал, и рендер у него жёстко завязан на player.z).
 
 ## Renderer Customizations (`DrawTextCodepointInBox`)
* Fixed-box glyph rendering with pixel snapping and font-size LOD.
* **Composite multi-slice rendering & Camera Z scrubbing — implemented this session.**
* Render loops optimized to `Z -> Y -> X` ordering. This fixed a critical O(N³) Raylib batching performance issue present in the initial Claude implementation (which did X -> Y -> Z and broke texture batching every cell).
* **Top-Down Occlusion Check implemented:** Upper opaque tiles fully block lower layers from rendering on the same X/Y cell. `kVeilColor` accumulates smoothly only through open air (`TILE_AIR`) shafts and holes, creating an accurate, readable visual depth.
* Global `g.zoom` bug still leaks between states; root cause in `MainMenu::OnUpdate` still not fixed.

## CURRENT TODO LIST / BACKLOG
1. **[VERIFY] Playtest updated composite rendering:** confirm the Z-order optimization holds up, occlusion functions correctly, and performance stays crisp.
2. **[NEXT] Fix Climb Mechanic & Map Shaft:** `tryClimb` allows walking through ceilings. Need to restrict it so player can only climb if there is `TILE_AIR` directly above them and a wall adjacent to hold onto. Update `initDefault()` area #2 to make the shaft realistic (open air in the center instead of solid floors).
3. **[BUG] Shared `zoom` on `Game`:** still leaks between states.
4. **[Feature] Doors:** map objects (separate layer), not started.

##!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
## True-Voxel World Model (Minecraft-style Z-axis)
* **Crucial Paradigm Shift:** The `z` coordinate represents the space occupied by the entity's body/head (which must be `TILE_AIR`), NOT the floor level. 
* The actual solid block the player stands on is always located at `player.z - 1`. 
* Example: To stand on the ground level (bedrock/earth at `z=0`), the player must have `player.z = 1`. Moving to `z=0` is blocked because it is physically inside the solid ground block.
* All physics checks (`isWalkable`, `tryClimb`, `fallThroughHole`, and ramps) must be updated to inspect `z - 1` for standing support and `z` for body headroom.



## Core Tech Stack
* C++23, `-fno-exceptions`, `-fno-rtti`, Raylib 6.0+. Hobby project.
* **Error handling:** no `noexcept` anywhere — early crash/abort on unrecoverable errors, or explicit `bool`/`nullptr`/return-code checks otherwise.

## Renderer Customizations (`DrawTextCodepointInBox`)
* Fixed-box glyph rendering with pixel snapping and font-size LOD.
* **Composite multi-slice rendering & Camera Z scrubbing — fully integrated.**
* Render loops optimized to `Z -> Y -> X` ordering for efficient Raylib batching.
* **Top-Down Occlusion Check implemented:** Upper opaque tiles fully block lower layers from rendering on the same X/Y cell. `kVeilColor` depth overlays apply strictly to empty air spaces (`TILE_AIR`) below the camera's baseline (`z < cameraZ - 1`).
* **Smart Ramp Arrow Rendering:** `DrawWorldTile` dynamically chooses a single valid arrow representation depending on the camera's Z. Ascending arrows display from below, while descending arrows render only when scrubbing above the headroom layer. The overlapping 'H' stairs character has been removed for a cleaner ASCII aesthetic.
* **Dynamic Player Veil:** `@` automatically darkens and blends with `kVeilColor` when the camera scrubs above the entity's physical location. It fades to a translucent phantom silhouette if the camera dives completely underneath its standing layer.

## Data Structures
* `RenderDef`/`PhysicsDef` indexed by `TileId`.
* `TileId`: `TILE_AIR`, `TILE_FLOOR`, `TILE_WALL`, `TILE_WATER`, `TILE_DOOR`, `TILE_RAMP_N/S/E/W`.
* `GameMap`: `Depth = 4`. High-altitude boundary walls generated seamlessly up to `Depth - 1` inside `initDefault()`.

## True-Voxel World Model (Minecraft-style Z-axis)
* **Crucial Paradigm Shift — Completed:** The entity's `z` coordinate represents the exact space occupied by the body/head (which expects `TILE_AIR`). The solid block supporting the feet is located strictly at `z - 1`.
* Spawn points, camera focus, and map triggers shifted from `z = 0` to `z = 1` across all states.
* `isWalkable` rigorously verifies non-solid body headroom at `z` and solid grounding underneath at `z - 1`.
* `fallThroughHole` cascades downward through open air, safely locking the entity's position exactly one tile above the first discovered solid layer.

## GameplayState
* Movement: WASD/arrows/QEZX same-level only; Shift = continuous run.
* Boundary safety checks embedded into `tryMoveHorizontal` to mitigate off-map OOB read/write memory violations.
* **Ramps (Voxel Adaptation):** Accessing a ramp cell requires entering through its ascending vector. Standing on a ramp and stepping forward activates a combined `tryAscendRamp` translation (`dx/dy` forward, `z+1` upward). Descending leverages automatic gravity fall-throughs upon retreating into the open headroom.
* `tryClimb` (vertical shafts): Restricts movement to valid `TILE_AIR` paths adjacent to at least one anchoring `TILE_WALL`.
* `tryAutoStep` (Ctrl held): Patched to prevent clipping through directional ramp angles from invalid side trajectories.

## CURRENT TODO LIST / BACKLOG
1. **[VERIFY] Debug newly discovered anomalous behavior:** Playtest Room 3 to analyze the strange physics quirk found during ramp scrubbing.
2. **[BUG] Shared `zoom` on `Game`:** still leaks between states; root cause in `MainMenu::OnUpdate` not fixed.
3. **[Feature] Doors:** map objects (separate layer), not started.

## Multi-Story Voxel Structures (Ceilings & Floors)
* Hanging 2D planes/plates do not exist. A floor separating two stories is a full 1x1x1 solid voxel layer.
* Standard 2-story building layout:
  - `z = 0`: Ground foundation under the building (Solid floor/stone).
  - `z = 1`: 1st-story living space (TILE_AIR for body/head, surrounded by TILE_WALL).
  - `z = 2`: Inter-floor structural slab (Solid TILE_FLOOR acting as 1st-story ceiling and 2nd-story ground support).
  - `z = 3`: 2nd-story living space (TILE_AIR for body/head, surrounded by TILE_WALL).



# AI Context for p1_sandbox

## Repo layout
`src/`: `game.hpp`, `main.cpp`, `pch.h`, `talloc.c`/`talloc.h`. 
Root: `AI_CONTEXT.md`, `CMakeLists.txt`. 
`assets/fonts/`: `unscii-8.ttf`, `unscii-8-thin.ttf`, `unscii-16.ttf`, `JetBrainsMonoNL-SemiBold.ttf`.

## Core Tech Stack
* C++23, `-fno-exceptions`, `-fno-rtti`, Raylib 6.0+. 
* **Error handling:** no `noexcept` — early crash/abort on unrecoverable errors, or explicit return-code/bounds checks otherwise.

## True-Voxel World Model (Minecraft-style Z-axis)
* **Core Paradigm:** The entity's `z` coordinate represents the exact space occupied by the body/head (which expects `TILE_AIR` or non-solid blocks). The solid block supporting the feet is located strictly at `z - 1`.
* Spawn points, camera focus, and physics constraints shifted from old 2D floors (`z = 0`) to eye-level space (`z = 1`).
* `isWalkable` checks non-solid body headroom at `z` and solid grounding underneath at `z - 1`. It explicitly blocks entering solid voxels like `TILE_FLOOR` or `TILE_WALL` horizontally.
* `fallThroughHole` cascades downward through open air, safely locking the entity's position exactly one tile above the first discovered solid layer.
* **Multi-Story Buildings:** Floors separating stories are full 1x1x1 solid voxel layers (`TILE_FLOOR`). A standard 2-story building layout uses `z=1` for 1st-floor living space, `z=2` for the concrete slab ceiling/floor, and `z=3` for 2nd-floor living space.

## Renderer Customizations (`DrawTextCodepointInBox`)
* Fixed-box glyph rendering with pixel snapping and font-size LOD.
* **Top-Down Occlusion Check Loop:** The render loop inside `OnDraw` iterates over `z` from `bottomZ` up to `topZ`, and checks `checkZ` from `z+1` to `topZ` to see if a solid block occludes the tile. If hidden, rendering is skipped.
* **Voxel Wall Roofs:** `DrawWorldTile` dynamically renders flat dot roofs (`.`) on walls (`TILE_WALL` and `TILE_BORDER_WALL`) if they are below the camera baseline (`z < cameraZ`), preventing players from feeling "inside the block" when standing on top of walls.
* **Dynamic Depth Veil:** Layers below the camera's baseline (`z < cameraZ - 1`) receive a translucent overlay (`kVeilColor = {40,120,220,90}`).
* **Dynamic Player Veil:** `@` smoothly darkens/blends with the veil below the camera, and fades to a translucent phantom silhouette if the camera dives underneath the player's layer (`player.z > cameraZ`).
* **The `↔` Staircase Visual Bug:** Currently, `DrawWorldTile` renders the ramp arrow at `z=0` AND the reverse tile at `z=1` simultaneously if both layers are inside the camera's visibility window, merging `►` and `◄` into a single bidirectional symbol `↔`. This provides a cool, unintentional ASCII indicator but needs code separation later.

## Data Structures
* `TileId`: `TILE_AIR`, `TILE_FLOOR`, `TILE_WALL`, `TILE_WATER`, `TILE_DOOR`, `TILE_RAMP_N/S/E/W`, `TILE_BORDER_WALL`.
* **`TILE_BORDER_WALL` & Smart Accessors:** `GameMap::get` automatically generates an infinite boundary wall of dark gray `#` symbols if X/Y requests go outside map dimensions. This completely prevents out-of-bounds array violations and SegFaults.
* **`TILE_PHYSICS`:** `TILE_FLOOR` is marked as `.isSolid = false` in physics because it's walkable, but acts as a solid slab block when checked via grid arrays.

## GameplayState
* Movement: WASD/arrows/QEZX same-level; Shift = fast run; Ctrl + direction = `tryClimbLedge` (vaulting over a single block; restricted from side-clipping directional ramps).
* **Voxel Staircases (Ramps):** Approaching a ramp requires entering through its ascending vector. Walking forward into a higher step before your face (`player.z`) triggers an automated staircase climb. Descending works via stepping back into thin air, triggering `fallThroughHole` onto the ramp below.
* `tryClimb` (vertical shafts): PageUp/PageDown movement restricted to valid `TILE_AIR` paths adjacent to at least one anchoring `TILE_WALL`.
* Three procedurally shifted debug areas near spawn `(1,1,1)`:
  1. Platform `(20-24,18-22)` — `tryClimbLedge` vault test.
  2. Vertical Shaft `(5,5)` — 3x3 open air shaft with a south entryway at `(5,6,1)`.
  3. Two-Story Voxel House `(10-14,3-7)` — Features `z=1` living space, `z=2` concrete solid slab floor with a drop hole at `(12,5,2)`, `z=3` upper space, a southern doorway on the 1st floor, and a complete Minecraft-style staircase march on the west wall.

## CURRENT TODO LIST / BACKLOG
1. **[BUG / NEXT] Fix Ramp Symbol Overlay:** Separate `DrawWorldTile` logic so that the ascending arrow (`z=0`) and descending arrow (`z=1`) do not combine into `↔` when looking at `cameraZ = 1`.
2. **[BUG] Shared `zoom` on `Game`:** still leaks between states; root cause in `MainMenu::OnUpdate` not fixed.
3. **[Feature] Doors:** map objects (separate layer), not started.
