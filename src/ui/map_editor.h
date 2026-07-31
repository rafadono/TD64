#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H
struct GameState;

// Enters the editor. slot >= 0 loads that saved map for editing; slot < 0
// starts a new blank map (grass, curved path, 200 gold, 20 lives).
void map_editor_enter(int slot);

void map_editor_handle_input(struct GameState* game);
void map_editor_render(const struct GameState* game);

#endif // MAP_EDITOR_H
