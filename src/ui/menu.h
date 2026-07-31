#ifndef MENU_H
#define MENU_H
struct GameState;
void menu_render(const struct GameState* game);
int menu_handle_input(struct GameState* game);
#endif
