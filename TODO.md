# TD64 Roadmap

Tracked improvements and new features, grouped by area. Items are grounded in
gaps found while building/reviewing the current codebase, not aspirational
guesses — see the note on each item for why it's here.

## High Priority

- [ ] **Record/source real audio assets.** `src/systems/audio.c` and the mixer
      wiring are complete, but `assets/audio/` is empty — none of
      `grass.wav64`, `desert.wav64`, `snow.wav64`, `volcano.wav64`,
      `hit.wav64`, `coin.wav64`, or `levelup.wav64` exist yet, so the game is
      currently silent.
- [ ] **Test on real hardware** (EverDrive64, SummerCart64). Everything so
      far has only been validated on the Ares emulator.
- [ ] **Wire up the real wave composition in the debug "Wave Preview"
      overlay.** `src/systems/debug.c` renders `int count_height = 4 + i*3;
      // Mock data` instead of reading the actual `spawn_remaining[]` queue —
      it always shows the same fake bars regardless of the upcoming wave.
- [x] **Compute `gold_per_second` and `dps_total` for the debug Economy
      overlay.** `debug_track_gold_earned()`/`debug_track_damage_dealt()`
      accumulate every gold gain (kills, wave-clear bonus) and damage hit;
      `debug_update_perf()` — already called once/sec from `main.c`'s FPS
      block — rolls the accumulated total into the displayed rate and resets
      it, so the bars now move instead of staying empty.
- [ ] **Persist the selected language across power cycles.** `lang.c` resets
      to `LANG_DEFAULT` (English) on every boot; add a field to
      `GameProgress` (`save.h`/`save.c`) so the last chosen language survives
      a restart like campaign progress already does.

## Map Editor

- [x] Free-form waypoint path drawing. Paint `TERRAIN_PATH` tiles and set
      the PATH tab to CUSTOM — `path_init_from_terrain()` (pathfinding.c)
      traces the painted corridor into real waypoints, falling back to the
      curve preset if the trace is invalid (too short, or doesn't reach a
      second grid edge). Branching corridors aren't supported (a branch is
      silently ignored rather than followed) — acceptable for a hand-painted
      single-width path.
- [x] On-screen name entry for custom maps (C-left opens a simple grid
      keyboard; Z backspaces, B confirms). Shown in the editor's top bar and
      in the Custom Maps slot list.
- [x] Controller Pak support on all 4 ports — `save_system_check()` scans
      every port and uses the first ready (or otherwise first unformatted)
      pak it finds, instead of a hardcoded port 1.
- [x] "Clear terrain" shortcut: hold L+R and tap B to reset the grid to
      all-grass.

## Content

- [x] `tile_lava`/`tile_path` are now real `TerrainType` values
      (`TERRAIN_LAVA`, `TERRAIN_PATH`). Volcanic Pass's lava rivers use
      `TERRAIN_LAVA` for real (previously a `TERRAIN_WATER` placeholder).
      `TERRAIN_PATH` cannot be built on and is also what free-form custom
      paths are painted with (see Map Editor above).
- [ ] More maps, factions, or unit types beyond the current 4 factions x 6
      roles — deliberately NOT attempted in this pass; a new faction alone
      needs 6 balanced units + 6 new animated sprite sheets, which is a
      larger, separate content investment.
- [x] A difficulty-selection screen for the 4 fixed campaigns
      (`STATE_DIFFICULTY_SELECT`, between "Play Campaign" and faction
      select): Easy/Normal/Hard/Extreme scale enemy HP/damage/speed, tower
      cost, and starting gold/lives on top of the `DIFFICULTY_*` baseline in
      `game_config.h`. Custom maps are unaffected (reset to Normal) and keep
      their own per-map difficulty label.
- [x] `GameProgress.campaigns_completed` now gates something: Hard requires
      completing at least 1 campaign, Extreme requires all 4. Permissive
      default (unlocked) if no Controller Pak/progress is readable, so
      pak-less players aren't locked out.
- [x] The 4 fixed campaign maps are now `WORLD_WIDTH x WORLD_HEIGHT`
      (`src/core/screen.h`) — 4x the area of one screen, tiled as 4 normal
      maps arranged 2x2 — with the camera auto-scrolling to follow the
      D-pad cursor (`camera_ensure_visible()`, no separate pan input) and
      the enemy path scaled 2x/2x to match. Custom maps are unaffected and
      stay exactly one screen. Fits in the base 4MB without the Expansion
      Pak (~614KB for the composed terrain surface, still small).

## Multiplayer

- [ ] Investigate 2-4 player support — the N64 has 4 controller ports and
      the engine only reads `JOYPAD_PORT_1` today. Could be co-op (shared
      base) or competitive (dueling lanes).

## Polish / UX

- [ ] UI sound feedback (menu navigation, confirm/cancel) — blocked on the
      Audio item above.
- [x] Remappable controls: 7 in-game actions (place/cancel/prev-unit/
      next-unit/upgrade/spawn-wave/pause) rebindable from a new CONTROLS
      screen on the main menu, persisted to the Controller Pak
      (`src/systems/controls.h`/`.c`). D-pad movement stays fixed by design,
      and C-up can't be assigned since it's hardwired to the debug menu.
- [ ] A volume settings screen once audio exists (control remap above is
      already done independently of audio).
- [ ] Transitions/fades between menu states (currently instant cuts).
- [ ] Accessibility pass on faction palettes for colorblind players — e.g.
      Dawnguard (blue/gold) vs. Veilstorm (cyan/violet) may be hard to tell
      apart for some players at a glance.
- [x] Unified debug menu: replaced the old scattered C-down/C-left/C-right
      overlay toggles and the 4 hidden L+R+C-* cheat combos with a single
      navigable menu (C-up opens it) listing all 18 overlays/cheats. Also
      fixed a real bug found in the old scheme: C-right doubled as both
      "toggle performance overlay" and "upgrade tower", both firing on the
      same press.

## Balance

- [ ] Real playtesting pass on the score/economy formulas in README.md —
      the ratios (cost vs. gold_reward, wave scaling, difficulty
      multipliers) were designed analytically, not empirically tuned.
- [ ] Validate the difficulty curve across all 4 campaigns and the 1-5
      custom-map difficulty labels.

## Engineering / CI

- [ ] Pin the `ghcr.io/dragonminded/libdragon` image tag and the vendored
      SDK source commit instead of `:latest` / the default branch, so CI
      builds are reproducible instead of drifting with upstream.
- [ ] Cache the toolchain bootstrap step in CI — each run currently
      reinstalls the entire libdragon SDK from scratch inside the ephemeral
      container.
- [ ] Auto-attach `TD64.z64` to a GitHub Release when a version tag is pushed.
- [ ] Optional: a headless emulator smoke test in CI (boot the ROM, confirm
      it doesn't hang/crash in the first few seconds).
- [x] Turn `libdragon/` into a real git submodule — matches what
      `.libdragon/config.json` already declared (`"vendorStrategy":
      "submodule"`). CI now checks it out via `actions/checkout`'s
      `submodules: true` instead of a separate `git clone` of the default
      branch, and — as a side effect — the vendored commit is now pinned
      instead of silently drifting to upstream HEAD on every CI run (the
      image *tag* itself, `:latest`, is a separate still-open item above).

## Nice-to-have / Exploratory

- [ ] Additional languages beyond EN/ES — `lang.c`'s string-table
      architecture already supports adding a column with zero UI code
      changes.
- [x] An achievements/stats screen beyond best score: a new STATS option on
      the main menu (`src/ui/stats_menu.h`/`.c`) showing total kills across
      all runs and fastest campaign clear time, alongside the existing best
      score, all persisted to the Controller Pak (`GameProgress`, extended
      in `save.h`).
- [x] `ScoreSystem` now logs a curated event timeline (`RunLog`, `score.h`)
      instead of only aggregate totals — map transitions, perfect/hero-wave
      clears, hero kills, and the run's outcome, each timestamped. Viewed
      via the STATS screen's "Highlights" page. **Scope note:** this is a
      RAM-only highlights list, not the frame-accurate replay/spectate this
      item originally asked for — true replay would need deterministic RNG
      plus recorded input playback, a bigger rearchitecture left for later
      if it's still wanted.
