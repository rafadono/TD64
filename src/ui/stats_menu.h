#ifndef STATS_MENU_H
#define STATS_MENU_H
struct GameState;

void stats_menu_enter(void);
void stats_menu_handle_input(struct GameState* game);
void stats_menu_render(const struct GameState* game);

#endif // STATS_MENU_H
