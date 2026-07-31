#ifndef PATHFINDING_H
#define PATHFINDING_H
#define MAX_WAYPOINTS 16
typedef struct { float x, y; } Waypoint;
typedef struct Path { Waypoint points[MAX_WAYPOINTS]; int count; } Path;
struct Entity; // forward
void path_init_straight(Path* p);
void path_init_curve(Path* p);
void path_init_zigzag(Path* p);
void path_init_spiral(Path* p);
void path_follow(struct Entity* e, const Path* path, float dt);
#endif
