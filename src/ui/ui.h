#ifndef UI_H
#define UI_H
struct GameState;
void ui_draw_hud(const struct GameState* game);
void ui_draw_build_panel(const struct GameState* game);
void ui_draw_unit_tooltip(const struct GameState* game);
#endif
