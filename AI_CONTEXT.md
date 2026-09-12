# AI Context for p1_sandbox

## Core Tech Stack
* **Language:** C++23
* **Flags:** `-fno-exceptions`, `-fno-rtti` (Strict compile-time errors, memory safety via `std::nothrow` / pointer checks).
* **Framework:** Raylib 6.0+ (pure C integration, pixel graphics focus).
* **Target Layout:** Persistent world simulation, custom voxel framework foundations.
* **Scale target:** hobby project, up to ~50 concurrent players, single server process. No sharding, no strict anti-cheat requirements.
* **Error handling model (explicit decision):** no `noexcept` anywhere in the codebase — it doesn't add a real guarantee on top of `-fno-exceptions` and just becomes stale documentation. Errors are handled either by early crash/abort on unrecoverable conditions, or by explicit return codes / `bool` / `nullptr` checks (see `loadFromFile`, `isWalkable`, etc.).

## Renderer Customizations (`DrawTextCodepointInBox`)
* Renders individual characters into exact square tile grids (`boxSize` x `boxSize`).
* Features **Pixel Snapping** (`std::floor`) to prevent sub-pixel blur on zoom.
* **LOD System:** Vector fonts (`JetBrains Mono`) for high zooms (adds custom compression `widthRatio = 0.85f`), pixel-art raster fonts (`unscii8`) with hardcoded 1px borders for small zooms to prevent visual font gluing.
  * `g.unscii8` = `unscii-8.ttf` (regular). `g.unscii8t` = `unscii-8-thin.ttf` (thin variant). These names were swapped relative to file content until the 2026-09 font-load fix.
  * **Resolved:** low-zoom rendering intentionally stays on `g.unscii8` (regular), not `g.unscii8t` (thin). No change needed, was just a stale-comment concern.
* **`g.zoom` is shared global state on `Game`**, not per-state — leaks between `MainMenu`/`MapEditor`/`GameplayState`. Not fixed at the root yet (backlog #2), but `GameplayState`'s player-centered camera makes it visually harmless there.
* **Next planned step (not started):** composite Z-slice rendering for `GameplayState`. Currently only `player.z` is drawn. Plan (user's spec): render from `player.z - 5` up through `player.z`, with everything strictly below the player's own level tinted with a translucent blue "veil" so it reads as "underfoot, not on your level" rather than being confused with the current floor; anything below `player.z - 5` is just drawn black (not rendered in detail). Not implemented yet — next task after this document.
* **After that, planned:** a proper camera concept — mouse wheel changes the displayed Z-slice height (the window described above, still spanning 5 levels down from whatever height is selected); pressing the mouse wheel button or `F1` resets the camera back to the player's own level. Not implemented yet — planned after the rendering change above.

## Data Structures
* **Tile data is split by concern, not bundled:**
  * `RenderDef { int codepoint; Color fgColor; Color bgColor; }` — indexed by `TileId` via `TILE_RENDER[TILE_COUNT]`.
  * `PhysicsDef { bool isSolid; }` — indexed by `TileId` via `TILE_PHYSICS[TILE_COUNT]`.
  * `TileId` now has 7 members: `TILE_AIR`, `TILE_FLOOR`, `TILE_WALL`, `TILE_WATER`, `TILE_DOOR`, and **new this session:** `TILE_RAMP_UP` (`'<'`), `TILE_RAMP_DOWN` (`'>'`), both non-solid/walkable, gold-colored. `TILE_COUNT`-driven arrays/loops (palette rendering, static_asserts) picked these up automatically, no other code needed updating for the new tiles.
  * Tile descriptions via `GetTileDescription(TileId id)`, reads `TILE_DESC_STRINGS[TILE_COUNT]`, designed to move to a file/DB later.
  * `GameMap` stores a Z dimension: `array<uint8_t, Width*Height*Depth> tileIds`, `Depth = 4` (fixed for now — a stepping stone toward real chunking, not the final chunk system). `get(x,y,z)` / `set(x,y,z,id)` / `isWalkable(x,y,z)`. Old 2D `get(x,y)` / `set(x,y)` are wrappers over `z = 0` (used by MapEditor, unchanged).
  * `isWalkable` excludes `TILE_AIR` explicitly before checking `isSolid` — air is "no floor", not "floor that doesn't block". This is the standing-surface concept; `isSolid` alone still means "blocks entering this voxel at all" (walls/doors) and is checked separately in the movement code below for the wall-vs-hole distinction.
* **State Management:** `Game` holds `mMainMenu`, `mMapEditor`, `mGameplayState`, `worldMap` (as `GameMap`), via `IGameState` virtual interface.
* **Map Serialization:** `saveToFile` / `loadFromFile` still only persist the `z = 0` layer (`map.dat` format unchanged). All Z content (test areas, ramps) is procedural in `initDefault()`, not saved/loaded. Considering SQLite instead of the flat format — not started.
* `Player { int x = 1, y = 1, z = 0; }`.

## MapEditor UX & Features
* Left-click mouse drawing, right-click eraser. Hotkeys `1`-`4` select `TILE_FLOOR`/`TILE_WALL`/`TILE_WATER`/`TILE_DOOR`. No hotkeys added yet for the new ramp tiles — they're currently only placed procedurally in `initDefault()`, not paintable in the editor.
* **Nikon-Style Double Confirmation:** press a palette shortcut once to highlight, again to lock the brush and close the palette.
* Still 2D-only: edits only `z = 0`. Multi-layer editing is future work, alongside chunking.
* `TILE_DOOR` exists as a tile today, but doors are now planned as **map objects** (separate layer), not a `TileId` — see Planned Architecture. Fate of the `TILE_DOOR` tile/hotkey itself is unresolved.

## GameplayState
* Wired to MainMenu **"2. Play map."** Spawns at `{1,1,0}`.
* **Movement input model (reworked this session around a UX complaint — auto-stepping into walls wasn't discoverable):**
  * **Plain WASD/arrows/QEZX (diagonals):** `tryMoveHorizontal` — moves only on the *same* Z level. Walking into a wall simply stops you; no automatic elevation change.
  * **Shift held:** continuous movement at a fixed rate (`kFastMoveInterval = 1/33s`, tuned by ear — the original `1/3s` felt too slow at the current tile size/zoom). Timer-based in `OnUpdate` via a `while` catch-up loop (robust to frame hitches); `tryMoveHorizontal` itself is unchanged, just called on a schedule instead of once per keypress. `currentMoveDirection(dx,dy)` reads the same 8 key mappings as the tap path to drive this.
  * **Ctrl + direction:** `tryAutoStep` — the *old* auto-step search (`{z, z+1, z-1}` in that order) lives here now, exclusively. This is the "secret passage behind a wall" mechanic — deliberate, not a side effect of normal walking. `Ctrl+Q` is reserved for exiting to the menu, so northwest (`Q`) isn't reachable as a Ctrl-direction (minor known gap, not fixed).
  * **Ramps:** stepping onto `TILE_RAMP_UP`/`TILE_RAMP_DOWN` via normal `tryMoveHorizontal` triggers `applyRampTransition`, which shifts `player.z` by ±1 automatically as part of that same step (no modifier key needed) — this is now the *primary*, visually-obvious way to move between floors within a walkable path, replacing the old "walk into a raised platform and get silently auto-stepped up" trick for anything the player is meant to notice on their own.
  * **Falling through holes:** if the target tile at the player's current Z is `TILE_AIR` (not a wall, just no floor), `tryMoveHorizontal` lets the player step into that space and calls `fallThroughHole`, which searches straight down for the first walkable floor (or lands at `z=0` if none found) and puts the player there.
  * **Explicit climb:** `PageUp`/`PageDown` → `tryClimb(dz)`, unchanged — vertical, in-place, independent of horizontal movement, for shafts/ladders. Deliberately left as-is this session; flagged as a good candidate for a future "climbing" skill gated to certain players/NPCs rather than being universally available.
* **Camera:** player-centered (`camOffsetX/Y` computed in `OnDraw`), added last session to fix tiles being off-screen with no scrolling. Still just a fixed 2D pan — the mouse-wheel height-scrubbing camera described above is separate, planned work, not built yet.
* **Rendering:** still draws only `player.z` (single slice) — the planned blue-veil multi-slice composite (see Renderer Customizations above) hasn't been implemented yet.
* Three procedural test areas in `GameMap::initDefault()`:
  1. **Platform** at `(20-24, 18-22)`: `z=0` is `TILE_WALL` (pedestal, blocks normal walking), `z=1` is `TILE_FLOOR`. Only reachable via `Ctrl+direction` (`tryAutoStep`) now that plain walking doesn't auto-step — good test case for the "secret room" mechanic specifically.
  2. **Climb shaft** at `(30,10)`: floor on every `z` (0..Depth-1) plus small walled rooms on `z>=1`, reachable via `PageUp`/`PageDown`.
  3. **Ramp + hole (new this session)** at `(39-45, 29-35)`: a walled 7×7 room on `z=1`. Entrance via `TILE_RAMP_UP` at `(40,32,0)` (walk onto it from ground level, get lifted to the room automatically). Exit via `TILE_RAMP_DOWN` at `(44,32,1)`. A deliberate hole (`TILE_AIR`) at `(42,31,1)` to test falling — steps into it should drop the player back to `z=0`.
* **Status: not yet playtested since these latest movement-model changes** (Ctrl-autostep split, ramps, falling, run-speed retune). Compiles as of last check, but the actual feel/correctness of the new input model hasn't been confirmed hands-on.

## Planned Architecture (not yet implemented — design decisions locked in)
* **True-voxel world model confirmed this session:** tiles are 1x1x1 meter cubes (Minecraft-like); player/NPCs are also exactly 1 tile tall. A building floor is a solid slab layer plus an open layer above it where the player actually stands (e.g. a player's logical "floor 0" position physically occupies world voxel `z=1`, standing on the `z=0` slab). This already matches the existing `isWalkable`/movement semantics — no core rework was needed for this realization, it mostly changes how future maps get authored.
* **3D migration path:** world coordinates will eventually move to `WorldPos{x,y,z}` with chunked storage. The current `Depth=4` flat 3D array is a stepping stone, not the final chunk system.
  * **Chunk size still open:** Gemini suggested `16x16x256`; the user's original instinct was `16x16x16` to fit a 4KB memory page. Considering making it configurable.
* **Doors:** now understood as map objects (separate layer) rather than a `TileId`. Not started; interacts with the still-unresolved fate of the `TILE_DOOR` tile.
* **Inter-floor connectors — settled this session:**
  * **Ramps** (new `TILE_RAMP_UP`/`TILE_RAMP_DOWN`) are the primary, discoverable way to move between adjacent floors during normal walking — implemented.
  * **Vertical shafts + explicit climb** (`PageUp`/`PageDown`) remain for multi-level shafts/ladders — kept as-is, earmarked as a possible future skill restricted to certain players/NPCs rather than a universal ability.
  * **Auto-step onto raised platforms** is now gated behind `Ctrl+direction` specifically as a "secret passage" mechanic, not a general traversal tool.
* **Feature order:** ~~(1) Z-coordinate + player movement/collision~~ — done. ~~(1b) ramps + falling~~ — done, needs playtesting. Next up in this thread: (2a) composite multi-slice rendering, (2b) mouse-wheel camera height. Then: (3) stateful world objects (doors as map objects). (4) networking/multiplayer.
* **Networking design (planned, not built yet):** server-authoritative, MUD-style — client sends minimal intent commands only, server holds full world state, client receives only what's in its FOV. Tick every 300-500ms. Chunk-version-counter sync.
* **Persistence:** considering SQLite instead of the current flat-binary `map.dat` — not started.
* **Android port:** wants to eventually port to Android — reworked UI/touch controls, and explicitly wants gameplay not hard-tied to keyboard input. `GameplayState::OnUpdate` calls `IsKeyPressed`/`IsKeyDown` directly throughout (now even more of it, with the Ctrl/Shift modifier branches) — the coupling to watch before it grows further. No concrete input-abstraction design yet.

## CURRENT TODO LIST / BACKLOG
1. **[VERIFY] Playtest the reworked movement model:** plain walking (no auto-step into walls), Shift-run at the new speed, Ctrl+direction secret-passage search on the `(20-24,18-22)` platform, the new ramp room at `(39-45,29-35)` (enter via `<` at `(40,32,0)`, exit via `>` at `(44,32,1)`, fall through the hole at `(42,31,1)`), and PageUp/PageDown climbing at `(30,10)`.
2. **[NEXT] Composite Z-slice rendering:** draw `player.z-5` through `player.z`, blue translucent veil on everything below the player's own level, black beyond `player.z-5`. See Renderer Customizations above for the agreed spec.
3. **[NEXT] Mouse-wheel camera height:** scrub the displayed slice window up/down with the mouse wheel; wheel-click or `F1` resets to the player's level.
4. **[BUG] Shared `zoom` on `Game`:** still leaks between `MainMenu`/`MapEditor`/`GameplayState`; root cause in `MainMenu::OnUpdate` not fixed (harmless in `GameplayState` due to the player-centered camera).
5. **[Feature] Doors:** map objects (separate layer), not started. Decide fate of `TILE_DOOR` tile/hotkey in MapEditor.
6. **[Design] Input abstraction for Android:** before `GameplayState::OnUpdate` grows further (it now has three modifier-gated branches), worth designing an intent layer (move/climb/interact) decoupled from the concrete input source.
7. **[Design] Chunk size:** `16x16x16` vs `16x16x256` vs configurable — decide before chunking implementation starts.
8. **[Design] Persistence:** decide whether to move to SQLite for map/world saving instead of the current flat `map.dat`.
