#ifndef CUSTOM_MAP_MENU_H
#define CUSTOM_MAP_MENU_H
struct GameState;

// Call when entering STATE_CUSTOM_MAP_MENU (refreshes the Controller Pak
// status and the slot list — does real I/O, don't call every frame).
void custom_map_menu_enter(void);

void custom_map_menu_handle_input(struct GameState* game);
void custom_map_menu_render(const struct GameState* game);

#endif // CUSTOM_MAP_MENU_H
