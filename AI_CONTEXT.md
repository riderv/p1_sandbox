# AI Context for p1_sandbox

## Core Tech Stack
* **Language:** C++23
* **Flags:** `-fno-exceptions`, `-fno-rtti` (Strict compile-time errors, memory safety via `std::nothrow` / pointer checks).
* **Framework:** Raylib 6.0+ (pure C integration, pixel graphics focus).
* **Target Layout:** Persistent world simulation, custom voxel framework foundations.
* **Scale target:** hobby project, up to ~50 concurrent players, single server process. No sharding, no strict anti-cheat requirements.
* **Error handling model (explicit decision):** no `noexcept` anywhere in the codebase. Errors are handled either by early crash/abort on unrecoverable conditions, or by explicit return codes / `bool` / `nullptr` checks.

## Renderer Customizations (`DrawTextCodepointInBox`)
* Renders individual characters into exact square tile grids (`boxSize` x `boxSize`), with pixel snapping and font-size-based LOD (vector font at high zoom, `unscii8` at low zoom — resolved, keeping regular not thin).
* **New this session:** optional `flipVertical` parameter (defaults to `false`). Implemented by copying `srcRec` into a `sampleRec` and negating its `height` right before the `DrawTexturePro` call — the copy matters, negating the original `srcRec` directly would corrupt `destRec` sizing (which is computed from `srcRec` earlier in the function) and silently break normal (non-flipped) glyph rendering. Used for the "upside-down stairs" ramp indicator.
* **Font codepoint loading (`Game_LoadFonts`):** fonts only load ASCII (32-126) + Cyrillic (0x400+) by default. **Added this session:** the 4 ramp direction-arrow codepoints (`0x25B2 ▲`, `0x25BC ▼`, `0x25BA ►`, `0x25C4 ◄`) and the ramp "stairs" codepoint `kRampStairsCodepoint = 0x1328D` (Egyptian Hieroglyphs block) at array slots 351-355. **Unverified:** requesting a codepoint from `LoadFontEx` doesn't guarantee the TTF file actually contains that glyph — the arrows are common enough to likely exist in `unscii-8`/JetBrains Mono, but `0x1328D` specifically might not, and could render as `?` (see the fallback logic in `DrawTextCodepointInBox`). Needs a compile+run check; if it shows `?`, pick a different codepoint from an already-working range.
* **`g.zoom` is shared global state on `Game`**, leaks between states; harmless in `GameplayState` thanks to the player-centered camera, but not fixed at the root (`MainMenu::OnUpdate`).
* **Next planned step (not started):** composite Z-slice rendering — draw `player.z-5` through `player.z`, translucent blue veil below the player's own level, solid black beyond `player.z-5`.
* **After that, planned:** mouse-wheel camera height scrubbing (shows the slice window at whatever height is selected), with wheel-click or `F1` resetting to the player's own level.

## Data Structures
* `RenderDef { int codepoint; Color fgColor; Color bgColor; }` / `PhysicsDef { bool isSolid; }`, both indexed by `TileId`.
* **`TileId` reworked this session** — ramps are now 4 directional tiles instead of 2 up/down tiles: `TILE_AIR`, `TILE_FLOOR`, `TILE_WALL`, `TILE_WATER`, `TILE_DOOR`, `TILE_RAMP_N`, `TILE_RAMP_S`, `TILE_RAMP_E`, `TILE_RAMP_W`. Each ramp tile's *direction* is the direction that ascends it (e.g. `TILE_RAMP_E` — walking east up it). All four are non-solid/walkable, gold-colored, arrow-shaped in `TILE_RENDER` (though the actual map tile is drawn as a two-pass composite, not the single `RenderDef` codepoint — see below).
* **New free-function helpers** (defined right after `GetTileDescription`, before `GameMap`, so they're pure `TileId`-level utilities with no `Game`/`GameMap` dependency):
  * `IsRampTile(tile)` — true for any of the 4 ramp tiles.
  * `RampDirectionVector(tile, dx, dy)` — the tile's ascent direction as a unit vector.
  * `OppositeRampTile(tile)` — N↔S, E↔W swap (used for the "look down from above" reverse-arrow render).
  * `RampArrowCodepoint(tile)` — just reads `TILE_RENDER[tile].codepoint` (kept as a named accessor for readability at call sites).
  * `kRampStairsCodepoint = 0x1328D` — the stairs glyph drawn on top of the arrow (see font-loading caveat above).
* `GameMap`: `Depth = 4` (still a stepping stone toward real chunking, not final). `get/set(x,y,z)`, `isWalkable(x,y,z)` (excludes `TILE_AIR` explicitly, then checks `isSolid`). 2D `get(x,y)`/`set(x,y)` wrap `z=0`, used by `MapEditor` (unchanged, still 2D-only).
* **New `DrawWorldTile(font, map, x, y, z, pos, boxSize)`** (defined right after `DrawTextCodepointInBox`) replaces the old single-`RenderDef`-lookup draw call in `GameplayState::OnDraw`'s tile loop. Behavior:
  * Normal tile → single draw, same as before.
  * Ramp tile → **two-pass**: draw the direction arrow first, then `kRampStairsCodepoint` on top (both at the same position/box, relying on the two glyph shapes not fully overlapping).
  * `TILE_AIR` tile whose `z-1` neighbor is a ramp → **two-pass, reversed**: `OppositeRampTile`'s arrow, then `kRampStairsCodepoint` drawn with `flipVertical=true`. Purely a visual cue ("you can step/fall through here") — the actual fall mechanic is unrelated code (`fallThroughHole`), this only affects what gets drawn.
* Map serialization (`saveToFile`/`loadFromFile`) still only persists `z=0`; ramps and all Z content remain procedural in `initDefault()`. SQLite still just under consideration, not started.
* `Player { int x = 1, y = 1, z = 0; }`.

## GameplayState
* Wired to MainMenu **"2. Play map."** Spawns at `{1,1,0}`.
* **Movement input model:**
  * **Plain WASD/arrows/QEZX:** `tryMoveHorizontal` — same-Z movement only, plus the new ramp-entry/ramp-ascend logic below. Walking into a wall just stops the player.
  * **Shift held:** continuous movement, `kFastMoveInterval = 1/33s` (retuned from an initial `1/3s` that felt too slow), `while`-loop accumulator (robust to frame hitches). `currentMoveDirection(dx,dy)` mirrors the tap key mapping.
  * **Ctrl + direction:** `tryAutoStep` — the old `{z, z+1, z-1}` search-based auto-step, now exclusively a deliberate "secret passage behind a wall" action, not a side effect of normal walking. `Ctrl+Q` reserved for exiting to menu, so northwest (`Q`) isn't reachable this way.
  * **Explicit climb:** `PageUp`/`PageDown` → `tryClimb(dz)`, unchanged, for vertical shafts/ladders — earmarked as a possible future skill for specific players/NPCs rather than a universal ability.
* **Ramps — fully reworked this session (directional, single-tile-per-connection):**
  * `tryMoveHorizontal` checks, before a normal move: if the player is *currently standing on* a ramp tile and the pressed direction matches that ramp's own direction, call `tryAscendRamp` instead of moving normally.
  * Entering a ramp tile (from a non-ramp position) is only allowed if the movement direction equals the ramp's direction vector — approaching from any other side is blocked like a wall.
  * `tryAscendRamp(g, rampTile)`: requires the voxel directly above the ramp (`z+1`) to be exactly `TILE_AIR` (headroom reserved for the ascent), and the landing cell (`x+rdx, y+rdy, z+1`) to be walkable; if both hold, moves the player there.
  * **Descending needs no separate tile**: since the space above a ramp is always kept as open air, walking back into that space (from the landing cell, in the opposite direction) hits the existing "step into `TILE_AIR` → `fallThroughHole`" path in `tryMoveHorizontal`, which searches straight down and lands the player back on the ramp. One ramp tile now serves both directions.
* **Camera:** player-centered pan (`camOffsetX/Y`), unchanged this session. Mouse-wheel height scrubbing is separate, still not built.
* **Rendering:** still a single Z-slice (`player.z`) per frame — composite multi-slice rendering (the "blue veil" plan) not implemented yet. Tile drawing now goes through `DrawWorldTile` (see above) instead of a raw `TILE_RENDER` lookup.
* Three procedural test areas in `GameMap::initDefault()`:
  1. **Platform** at `(20-24,18-22)`: `z=0` wall pedestal / `z=1` floor. Reachable only via `Ctrl+direction` (`tryAutoStep`) — the "secret room" test case.
  2. **Climb shaft** at `(30,10)`: floor at every `z` (0..Depth-1) plus small walled rooms on `z>=1`, reachable via `PageUp`/`PageDown`.
  3. **Directional ramp room (reworked this session)** at `(40-46,29-35)`: walled 7×7 room on `z=1`, with a deliberate floor opening at `(40,32,1)` in its west wall. A single `TILE_RAMP_E` sits just outside at `(39,32,0)` — walk east into it, then east again to ascend into the room through the opening; walk west from the opening to fall back down onto the ramp. A separate, unrelated hole (`TILE_AIR`) at `(43,31,1)` tests plain falling into the room's own floor gap.
* **Status: not yet playtested** — the directional-ramp rework (this session) and the earlier Ctrl-autostep/run-speed rework are both still awaiting hands-on testing. The `0x1328D` glyph availability is the biggest known risk to verify first.

## Planned Architecture (not yet implemented — design decisions locked in)
* **True-voxel world model:** tiles are 1×1×1 meter cubes; player/NPCs are 1 tile tall. A building floor = solid slab + open layer above where the player stands. Confirmed to already match engine semantics, mostly a map-authoring concern going forward.
* **3D migration path:** eventual `WorldPos{x,y,z}` + chunked storage; current `Depth=4` flat array is a stepping stone. Chunk size still open (`16x16x16` vs Gemini's `16x16x256` vs configurable).
* **Doors:** map objects (separate layer), not a `TileId`. Not started; `TILE_DOOR` tile's fate still unresolved.
* **Inter-floor connectors:**
  * **Directional ramps** (this session) are the primary, discoverable way to move between adjacent floors — implemented, unplaytested.
  * **Vertical shafts + explicit climb** remain for multi-level shafts/ladders, possibly a future restricted skill.
  * **Ctrl-gated auto-step** is now specifically the "secret passage" mechanic, not general traversal.
* **Feature order:** Z-coordinate/movement — done. Ramps + falling — done, needs playtesting. Next: composite multi-slice rendering, then mouse-wheel camera height. Then: doors as map objects. Then: networking/multiplayer.
* **Networking (planned):** server-authoritative MUD-style, minimal client intents, FOV-scoped state, 300-500ms ticks, chunk-version-counter sync.
* **Persistence:** SQLite under consideration instead of flat `map.dat` — not started.
* **Android port:** wants touch-friendly UI eventually; explicitly wants gameplay not hard-tied to keyboard. `GameplayState::OnUpdate` now has three modifier-gated branches (plain/Shift/Ctrl) directly calling `IsKeyPressed`/`IsKeyDown` — growing coupling to watch before an input-abstraction layer becomes necessary.

## CURRENT TODO LIST / BACKLOG
1. **[VERIFY] Font glyph check:** confirm whether `0x1328D` (stairs) actually renders in `unscii-8`/JetBrains Mono, or falls back to `?`. If it falls back, pick a replacement codepoint from an already-working range.
2. **[VERIFY] Playtest directional ramps:** entering `TILE_RAMP_E` at `(39,32,0)` only from the west, ascending into the room at `(40,32,1)`, descending back by walking west off the landing spot, and the separate hole at `(43,31,1)`.
3. **[VERIFY] Playtest the rest of the movement rework:** plain walking (no auto-step into walls), Shift-run at the new speed, Ctrl+direction secret-passage search on the `(20-24,18-22)` platform, PageUp/PageDown climbing at `(30,10)`.
4. **[NEXT] Composite Z-slice rendering:** `player.z-5` through `player.z`, blue translucent veil below the player's level, black beyond `player.z-5`.
5. **[NEXT] Mouse-wheel camera height:** scrub the displayed slice window; wheel-click or `F1` resets to the player's level.
6. **[BUG] Shared `zoom` on `Game`:** still leaks between states; root cause in `MainMenu::OnUpdate` not fixed (harmless in `GameplayState` due to the camera).
7. **[Feature] Doors:** map objects (separate layer), not started. Decide fate of `TILE_DOOR` tile/hotkey in MapEditor. No hotkey exists yet for painting the new ramp tiles either — currently procedural-only.
8. **[Design] Input abstraction for Android:** `GameplayState::OnUpdate` now has three modifier-gated input branches — worth an intent layer before it grows further.
9. **[Design] Chunk size:** `16x16x16` vs `16x16x256` vs configurable — decide before chunking starts.
10. **[Design] Persistence:** SQLite vs flat `map.dat` — decide.
