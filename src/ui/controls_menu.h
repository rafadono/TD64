#ifndef CONTROLS_MENU_H
#define CONTROLS_MENU_H
struct GameState;

void controls_menu_enter(void);
void controls_menu_handle_input(struct GameState* game);
void controls_menu_render(const struct GameState* game);

#endif // CONTROLS_MENU_H
