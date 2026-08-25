#include "../core/engine.h"
#include "../config/game_config.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static Entity pool[MAX_ENTITIES];
static uint32_t next_entity_id = 1;

// Shared animation defs. The 24 sprites (tools/gen_sprites.py) are already
// generated as 8-frame sheets (1 idle + 4 walk + 3 attack, 32px each) in a
// single horizontal strip per unit — frame_width=32 enables the sub-rect
// cropping in entities_render(). start_frame marks where each cycle starts
// within the shared sheet: idle at 0, walk at 1, attack at 5.
static const AnimDef ANIM_IDLE   = {0, 1, 1.0f,  true,  32};
static const AnimDef ANIM_WALK   = {1, 4, 0.12f, true,  32};
static const AnimDef ANIM_ATTACK = {5, 3, 0.10f, false, 32};

// -----------------------------------------------------------------------------

void entities_init(void) {
    memset(pool, 0, sizeof(pool));
    next_entity_id = 1;
}

void entities_clear(void) {
    for (int i = 0; i < MAX_ENTITIES; i++) pool[i].active = false;
}

Entity* entity_alloc(void) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!pool[i].active) {
            memset(&pool[i], 0, sizeof(Entity));
            pool[i].active = true;
            pool[i].id = next_entity_id++;
            return &pool[i];
        }
    }
    debugf("WARNING: entity pool full!\n");
    return NULL;
}

int entities_count_active(void) {
    int n = 0;
    for (int i = 0; i < MAX_ENTITIES; i++) if (pool[i].active) n++;
    return n;
}

// -----------------------------------------------------------------------------
// Apply base stats from the config table to an entity
// -----------------------------------------------------------------------------
static void entity_apply_stats(Entity* e, const UnitStats* s) {
    e->hp     = e->hp_max     = s->hp_max;
    e->damage = e->base_damage = s->damage;
    e->attack_range  = e->base_range   = s->attack_range;
    e->attack_cooldown = e->base_cooldown = s->attack_cooldown;
    e->base_speed = e->speed = s->move_speed;
    e->gold_reward  = s->gold_reward;
    e->score_reward = s->score_reward;
    e->xp_reward    = s->xp_reward;
    e->w = e->h = s->size;
    e->tint = s->color_primary;
    e->ability_timer = s->ability_cooldown;
    e->ability_ready = false;
}

// -----------------------------------------------------------------------------
// Spawn a tower (MODE_TOWER, TEAM_PLAYER by default)
// -----------------------------------------------------------------------------
Entity* unit_spawn_tower(FactionId f, UnitType t, float x, float y) {
    const UnitStats* s = unit_get_stats(f, t);
    if (!s) return NULL;

    Entity* e = entity_alloc();
    if (!e) return NULL;

    e->is_projectile = false;
    e->team         = TEAM_PLAYER;
    e->mode         = MODE_TOWER;
    e->faction      = f;
    e->unit_type    = t;
    e->x = x;
    e->y = y;
    e->upgrade_tier = 0;

    entity_apply_stats(e, s);
    level_init(&e->level_data);

    e->sprite = resources_get_unit_sprite(f, t);
    animator_play(&e->anim, &ANIM_IDLE);

    debugf("Tower spawned: %s [%s]\n",
           UNIT_NAME(f, t), FACTION_NAME(f));
    return e;
}

// -----------------------------------------------------------------------------
// Spawn a runner/enemy (MODE_RUNNER, TEAM_ENEMY by default)
// Enemy level scales HP/damage based on wave number
// -----------------------------------------------------------------------------
Entity* unit_spawn_runner(FactionId f, UnitType t, const Path* path, int wave) {
    const UnitStats* s = unit_get_stats(f, t);
    if (!s || !path || path->count == 0) return NULL;

    Entity* e = entity_alloc();
    if (!e) return NULL;

    e->is_projectile    = false;
    e->team             = TEAM_ENEMY;
    e->mode             = MODE_RUNNER;
    e->faction          = f;
    e->unit_type        = t;
    e->x = path->points[0].x;
    e->y = path->points[0].y;
    e->current_waypoint = 0;
    e->upgrade_tier     = 0;

    entity_apply_stats(e, s);
    level_init(&e->level_data);

    // Wave-based scaling
    int enemy_level = 1 + (wave / 5);
    if (enemy_level > 1) {
        float hp_scale    = 1.0f + (enemy_level - 1) * s->level_hp_pct;
        float dmg_scale   = 1.0f + (enemy_level - 1) * s->level_damage_pct;
        float spd_scale   = 1.0f + (enemy_level - 1) * s->level_speed_pct;
        float gold_scale  = 1.0f + (enemy_level - 1) * 0.2f;

        e->hp     = e->hp_max     = (int)(s->hp_max    * hp_scale);
        e->damage = e->base_damage = (int)(s->damage   * dmg_scale);
        e->base_speed = e->speed  = s->move_speed      * spd_scale;
        e->gold_reward             = (int)(s->gold_reward * gold_scale);

        // Darken tint for higher levels
        float tint_mult = 1.0f - (enemy_level - 1) * 0.08f;
        if (tint_mult < 0.6f) tint_mult = 0.6f;
        e->tint.r = (uint8_t)(e->tint.r * tint_mult);
        e->tint.g = (uint8_t)(e->tint.g * tint_mult);
        e->tint.b = (uint8_t)(e->tint.b * tint_mult);
    }

    // Global difficulty knobs: DIFFICULTY_* (game_config.h, dev-tunable
    // baseline, 1.0 = no change) combined with the player-chosen campaign
    // difficulty scale (campaign.h, 1.0 on custom maps/Normal).
    e->hp     = e->hp_max     = (int)(e->hp_max     * DIFFICULTY_ENEMY_HP_MULT * game_difficulty_enemy_hp_mult());
    e->damage = e->base_damage = (int)(e->base_damage * DIFFICULTY_ENEMY_DMG_MULT * game_difficulty_enemy_dmg_mult());
    e->base_speed = e->speed  = e->speed * DIFFICULTY_ENEMY_SPEED_MULT * game_difficulty_enemy_speed_mult();
    e->gold_reward             = (int)(e->gold_reward * DIFFICULTY_ENEMY_GOLD_MULT);

    e->sprite = resources_get_unit_sprite(f, t);
    animator_play(&e->anim, &ANIM_WALK);
    return e;
}

// -----------------------------------------------------------------------------
// Projectile
// -----------------------------------------------------------------------------
Entity* projectile_spawn(Entity* owner, Entity* target) {
    if (!owner || !target) return NULL;

    Entity* e = entity_alloc();
    if (!e) return NULL;

    e->is_projectile = true;
    e->team          = owner->team;
    e->faction       = owner->faction;
    e->x = owner->x + owner->w * 0.5f;
    e->y = owner->y + owner->h * 0.5f;
    e->w = e->h = 6;
    e->proj_target   = target;
    e->proj_target_id = target->id;
    e->proj_damage   = owner->damage;
    e->owner_id      = owner->id;
    e->tint          = owner->tint;
    e->speed         = PROJECTILE_BASE_SPEED;

    float dx = target->x - e->x;
    float dy = target->y - e->y;
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist > 0.001f) {
        e->vx = (dx / dist) * e->speed;
        e->vy = (dy / dist) * e->speed;
    }

    e->sprite = resources_get_projectile_sprite(owner->faction);
    return e;
}

// -----------------------------------------------------------------------------
// Upgrade a tower
// -----------------------------------------------------------------------------
bool unit_upgrade(Entity* e, GameState* game) {
    if (!e || e->mode != MODE_TOWER) return false;
    if (e->upgrade_tier >= 3) return false;

    int cost = game_upgrade_cost(e);
    if (game->gold < cost) return false;

    game->gold -= cost;
    e->upgrade_tier++;

    const UnitStats* s = unit_get_stats(e->faction, e->unit_type);
    float tier_mult = 1.0f + e->upgrade_tier * 0.35f;

    e->base_damage    = (int)(s->damage         * tier_mult);
    e->damage         = e->base_damage;
    e->base_range     = s->attack_range          * (1.0f + e->upgrade_tier * 0.15f);
    e->attack_range   = e->base_range;
    e->base_cooldown  = s->attack_cooldown        * (1.0f - e->upgrade_tier * 0.08f);
    e->attack_cooldown = e->base_cooldown;

    // Gold tint at max tier
    if (e->upgrade_tier >= 3) {
        e->tint = RGBA32(255, 215, 0, 255);
    } else {
        float b = 1.0f + e->upgrade_tier * 0.2f;
        e->tint.r = (uint8_t)fminf(255, s->color_primary.r * b);
        e->tint.g = (uint8_t)fminf(255, s->color_primary.g * b);
        e->tint.b = (uint8_t)fminf(255, s->color_primary.b * b);
    }

    particles_emit_explosion(e->x + e->w/2, e->y + e->h/2,
                             RGBA32(255, 215, 0, 255), 10);
    return true;
}

// -----------------------------------------------------------------------------
static void entity_deal_damage(Entity* attacker, Entity* target, int damage, GameState* game) {
    if (!target->active) return;

    if (debug.one_hit_kills && target->mode == MODE_RUNNER && damage < target->hp) {
        damage = target->hp;
    }

    target->hp -= damage;
    debug_track_damage_dealt(damage);
    particles_emit_hit(target->x, target->y, attacker ? attacker->tint : RGBA32(255, 255, 255, 255));

    char txt[16];
    snprintf(txt, sizeof(txt), "-%d", damage);
    {
        // Floating text is stored/rendered in screen-space (unlike
        // particles, it doesn't go through the camera again at render
        // time), so world-anchored text converts once here at spawn time.
        float ftx = target->x, fty = target->y - 10;
        camera_apply(&game->camera, &ftx, &fty);
        floating_text_spawn(txt, ftx, fty, RGBA32(255, 80, 80, 255));
    }

    if (target->hp <= 0) {
        // Death
        game->gold += target->gold_reward;
        debug_track_gold_earned(target->gold_reward);
        score_on_kill(&game->score, target->gold_reward, target->score_reward);
        if (target->unit_type == UNIT_HERO) {
            run_log_add(&game->score.log, RUN_EVENT_HERO_KILLED, game->score.run_elapsed, game->wave);
        }
        game->enemies_remaining--;

        particles_emit_explosion(target->x + target->w/2,
                                 target->y + target->h/2,
                                 target->tint, 12);
        particles_emit_coins(target->x, target->y, target->gold_reward);
        camera_shake(&game->camera, 3.0f, 0.12f);

        char gold_txt[16];
        snprintf(gold_txt, sizeof(gold_txt), "+%d", target->gold_reward);
        {
            float gtx = target->x, gty = target->y;
            camera_apply(&game->camera, &gtx, &gty);
            floating_text_spawn(gold_txt, gtx, gty, RGBA32(255, 215, 0, 255));
        }

        // Audio trigger for kill!
        audio_play_sfx("coin");

        // XP for the tower
        Entity* xp_receiver = NULL;
        if (attacker) {
            if (attacker->mode == MODE_TOWER) {
                xp_receiver = attacker;
            } else if (attacker->is_projectile) {
                // Find the tower that actually fired this projectile (by owner_id),
                // not just any tower currently targeting the same runner — two towers
                // can share a target, and the shooter may have since retargeted.
                for (int k = 0; k < MAX_ENTITIES; k++) {
                    Entity* tw = &pool[k];
                    if (tw->active && tw->mode == MODE_TOWER && tw->id == attacker->owner_id) {
                        xp_receiver = tw;
                        break;
                    }
                }
            }
        }
        if (xp_receiver) {
            const UnitStats* s = unit_get_stats(xp_receiver->faction, xp_receiver->unit_type);
            level_add_xp(&xp_receiver->level_data, target->xp_reward, s, xp_receiver);
        }
        target->active = false;
    }
}

// UPDATE
// -----------------------------------------------------------------------------
void entities_update(float dt, GameState* game) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        Entity* e = &pool[i];
        if (!e->active) continue;

        animator_update(&e->anim, dt);

        // ---- PROJECTILE ----
        if (e->is_projectile) {
            if (!e->proj_target || !e->proj_target->active || e->proj_target->id != e->proj_target_id) {
                e->active = false;
                continue;
            }

            // Home in
            float dx = e->proj_target->x - e->x;
            float dy = e->proj_target->y - e->y;
            float dist_sq = dx*dx + dy*dy;

            if (dist_sq < 36.0f) {
                // Hit!
                audio_play_sfx("hit");
                entity_deal_damage(e, e->proj_target, e->proj_damage, game);
                e->active = false;
                continue;
            }

            if (dist_sq > 0.001f) {
                float dist = sqrtf(dist_sq);
                e->vx = (dx / dist) * e->speed;
                e->vy = (dy / dist) * e->speed;
            }
            e->x += e->vx * dt;
            e->y += e->vy * dt;

            // Off-map cull: bounded by the actual map's world size (not the
            // screen) — with a scrollable map, a projectile can be well
            // within the playable world while outside the CURRENT viewport,
            // and shouldn't be culled just because the camera has scrolled
            // away from it.
            if (e->x < -20 || e->x > game->map.width + 20 ||
                e->y < -20 || e->y > game->map.height + 20) {
                e->active = false;
            }
            continue;
        }

        // ---- RUNNER (enemy) ----
        if (e->mode == MODE_RUNNER) {
            terrain_apply_to_entity(e, &game->map.terrain);
            if (e->slow_timer > 0) {
                e->slow_timer -= dt;
                e->speed *= e->slow_mult;
                if (e->slow_timer <= 0) {
                    e->slow_timer = 0;
                    e->slow_mult = 1.0f;
                }
            }
            path_follow(e, &game->map.runner_path, dt);

            if (!e->active) {
                // Reached end -> lose a life
                game->enemies_remaining--;
                if (game->enemies_remaining < 0) game->enemies_remaining = 0;

                if (!debug.godmode) {
                    game->lives--;
                    if (game->lives < 0) game->lives = 0;
                }
                camera_shake(&game->camera, 5.0f, 0.2f);
                if (!debug.godmode && game->lives <= 0) {
                    game->game_over = true;
                    game->flow = STATE_GAME_OVER;
                    score_final(&game->score);
                    run_log_add(&game->score.log, RUN_EVENT_DEFEAT, game->score.run_elapsed, game->wave);

                    // Kills count toward the lifetime total on ANY run's
                    // end, defeat included, and regardless of campaign vs.
                    // custom map — unlike best_score/fastest_clear (campaign
                    // victory only, see campaign.c), a defeated run's kills
                    // are still real kills.
                    if (save_system_check() == SAVE_STATUS_READY) {
                        GameProgress progress;
                        if (!save_read_progress(&progress)) {
                            memset(&progress, 0, sizeof(progress));
                        }
                        progress.total_kills += (uint32_t)game->score.total_kills;
                        save_write_progress(&progress);
                    }
                }
            }
            continue;
        }

        // ---- TOWER ----
        if (e->mode == MODE_TOWER) {
            // Apply level bonuses to local base stats, then terrain modifiers
            const UnitStats* s = unit_get_stats(e->faction, e->unit_type);
            int base_dmg = e->base_damage;
            float base_rng = e->base_range;
            float base_cd = e->base_cooldown;

            if (s && e->level_data.level > 1) {
                int lv = e->level_data.level;
                base_dmg += (int)(s->damage * s->level_damage_pct * (lv-1));
                base_rng *= level_get_range_bonus(lv);
                base_cd  *= level_get_speed_bonus(lv);
            }

            TerrainModifier m = terrain_get_modifier(terrain_get(&game->map.terrain, e->x, e->y));
            e->damage        = (int)(base_dmg * m.tower_damage_mult * DIFFICULTY_TOWER_DMG_MULT);
            e->attack_range  = base_rng * m.tower_range_mult * DIFFICULTY_TOWER_RANGE_MULT;
            // DIFFICULTY_TOWER_SPEED_MULT: "higher = lower cooldown" (faster attacks)
            e->attack_cooldown = base_cd * m.tower_speed_mult / DIFFICULTY_TOWER_SPEED_MULT;

            // Advance timer
            e->attack_timer += dt;
            e->ability_timer -= dt;
            if (e->ability_timer <= 0) e->ability_ready = true;

            // Find/validate target
            bool target_valid = false;
            if (e->target && e->target->active && e->target->id == e->target_id && e->target->mode == MODE_RUNNER) {
                float d = distance_squared(e->x, e->y, e->target->x, e->target->y);
                if (d <= e->attack_range * e->attack_range) {
                    target_valid = true;
                }
            }

            if (!target_valid) {
                e->target = NULL;
                e->target_id = 0;
                float closest = e->attack_range * e->attack_range;
                for (int j = 0; j < MAX_ENTITIES; j++) {
                    Entity* runner = &pool[j];
                    if (!runner->active || runner->mode != MODE_RUNNER) continue;
                    if (runner->team == e->team) continue; // Don't attack own team
                    float d = distance_squared(e->x, e->y, runner->x, runner->y);
                    if (d < closest) {
                        e->target = runner;
                        e->target_id = runner->id;
                        closest = d;
                    }
                }
            }

            // Attack
            if (e->target && e->attack_timer >= e->attack_cooldown) {
                projectile_spawn(e, e->target);
                e->attack_timer = 0;
                animator_play(&e->anim, &ANIM_ATTACK);
            }

            // Ability activation
            if (e->target && e->ability_ready && s && s->ability_cooldown > 0) {
                e->ability_ready = false;
                e->ability_timer = s->ability_cooldown;

                {
                    float atx = e->x, aty = e->y - 12;
                    camera_apply(&game->camera, &atx, &aty);
                    floating_text_spawn(s->ability_name, atx, aty, RGBA32(255, 215, 0, 255));
                }
                particles_emit_explosion(e->x + e->w/2, e->y + e->h/2, s->color_secondary, 8);
                audio_play_sfx("levelup");

                if (strcmp(s->ability_name, "Slam") == 0) {
                    e->target->slow_timer = 2.0f;
                    e->target->slow_mult = 0.0f;
                    entity_deal_damage(e, e->target, e->damage * 2, game);
                } else if (strcmp(s->ability_name, "Bone Burst") == 0) {
                    float tx = e->target->x;
                    float ty = e->target->y;
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 50.0f * 50.0f) {
                                entity_deal_damage(e, runner, e->damage, game);
                            }
                        }
                    }
                } else if (strcmp(s->ability_name, "Smite") == 0) {
                    entity_deal_damage(e, e->target, e->damage * 3, game);
                } else if (strcmp(s->ability_name, "Divine Wrath") == 0) {
                    e->target->slow_timer = 2.0f;
                    e->target->slow_mult = 0.0f;
                    entity_deal_damage(e, e->target, e->damage * 4, game);
                } else if (strcmp(s->ability_name, "Death Coil") == 0) {
                    entity_deal_damage(e, e->target, e->damage * 3, game);
                    game->lives = fminf(20, game->lives + 1);
                } else if (strcmp(s->ability_name, "Life Drain") == 0) {
                    entity_deal_damage(e, e->target, (int)(e->damage * 1.5f), game);
                    game->gold += 5;
                } else if (strcmp(s->ability_name, "Poison Arrow") == 0) {
                    e->target->slow_timer = 3.0f;
                    e->target->slow_mult = 0.6f;
                    entity_deal_damage(e, e->target, e->damage * 2, game);
                } else if (strcmp(s->ability_name, "Firestorm") == 0) {
                    float tx = e->target->x;
                    float ty = e->target->y;
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 60.0f * 60.0f) {
                                entity_deal_damage(e, runner, e->damage, game);
                            }
                        }
                    }
                } else if (strcmp(s->ability_name, "War Cry") == 0) {
                    float tx = e->target->x;
                    float ty = e->target->y;
                    entity_deal_damage(e, e->target, e->damage * 3, game);
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 80.0f * 80.0f) {
                                runner->slow_timer = 2.0f;
                                runner->slow_mult = 0.5f;
                            }
                        }
                    }
                } else if (strcmp(s->ability_name, "Tempest") == 0) {
                    float tx = e->target->x;
                    float ty = e->target->y;
                    entity_deal_damage(e, e->target, e->damage * 3, game);
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner != e->target && runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 70.0f * 70.0f) {
                                runner->slow_timer = 3.0f;
                                runner->slow_mult = 0.5f;
                                entity_deal_damage(e, runner, e->damage * 2, game);
                            }
                        }
                    }
                } else if (strcmp(s->ability_name, "Dash") == 0) {
                    // Dawnguard Scout — quick strike with two instant hits
                    entity_deal_damage(e, e->target, e->damage, game);
                    entity_deal_damage(e, e->target, e->damage, game);
                } else if (strcmp(s->ability_name, "Holy Shield") == 0) {
                    // Dawnguard Warrior — strikes and protects the base with an extra life
                    entity_deal_damage(e, e->target, e->damage * 2, game);
                    game->lives = (int)fminf(20, game->lives + 1);
                } else if (strcmp(s->ability_name, "Volley") == 0) {
                    // Dawnguard Archer — arrow rain: hits the target and nearby enemies
                    float tx = e->target->x;
                    float ty = e->target->y;
                    entity_deal_damage(e, e->target, e->damage * 2, game);
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner != e->target && runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 90.0f * 90.0f) {
                                entity_deal_damage(e, runner, e->damage, game);
                            }
                        }
                    }
                } else if (strcmp(s->ability_name, "Fortify") == 0) {
                    // Dawnguard Tank — strikes and slows a wide area (holds the line)
                    float tx = e->target->x;
                    float ty = e->target->y;
                    entity_deal_damage(e, e->target, e->damage * 2, game);
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 100.0f * 100.0f) {
                                runner->slow_timer = 2.5f;
                                runner->slow_mult = 0.6f;
                            }
                        }
                    }
                } else if (strcmp(s->ability_name, "Phase") == 0) {
                    // Ironbone Scout — high-damage surprise strike
                    entity_deal_damage(e, e->target, e->damage * 3, game);
                } else if (strcmp(s->ability_name, "Undying") == 0) {
                    // Ironbone Tank — drains all nearby enemies and steals gold
                    float tx = e->target->x;
                    float ty = e->target->y;
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 90.0f * 90.0f) {
                                entity_deal_damage(e, runner, e->damage, game);
                                game->gold += 3;
                            }
                        }
                    }
                } else if (strcmp(s->ability_name, "Berserk") == 0) {
                    // Ashclaw Scout — frenzy with a single brutal strike
                    entity_deal_damage(e, e->target, e->damage * 3, game);
                } else if (strcmp(s->ability_name, "Hurl") == 0) {
                    // Ashclaw Archer — throws a heavy weapon that stuns on impact
                    e->target->slow_timer = 2.0f;
                    e->target->slow_mult = 0.5f;
                    entity_deal_damage(e, e->target, e->damage * 3, game);
                } else if (strcmp(s->ability_name, "Rage") == 0) {
                    // Ashclaw Tank — furious strike with cleave to nearby enemies
                    float tx = e->target->x;
                    float ty = e->target->y;
                    entity_deal_damage(e, e->target, e->damage * 3, game);
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner != e->target && runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 70.0f * 70.0f) {
                                entity_deal_damage(e, runner, e->damage, game);
                            }
                        }
                    }
                } else if (strcmp(s->ability_name, "Blink") == 0) {
                    // Veilstorm Scout — repositions and strikes, disorienting the target
                    e->target->slow_timer = 1.5f;
                    e->target->slow_mult = 0.7f;
                    entity_deal_damage(e, e->target, e->damage * 2, game);
                } else if (strcmp(s->ability_name, "Chain Strike") == 0) {
                    // Veilstorm Warrior — bolt that chains between nearby enemies
                    float tx = e->target->x;
                    float ty = e->target->y;
                    entity_deal_damage(e, e->target, e->damage * 2, game);
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner != e->target && runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 100.0f * 100.0f) {
                                entity_deal_damage(e, runner, e->damage, game);
                            }
                        }
                    }
                } else if (strcmp(s->ability_name, "Rune Burst") == 0) {
                    // Veilstorm Archer — small arcane explosion
                    float tx = e->target->x;
                    float ty = e->target->y;
                    entity_deal_damage(e, e->target, e->damage * 2, game);
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner != e->target && runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 55.0f * 55.0f) {
                                entity_deal_damage(e, runner, e->damage, game);
                            }
                        }
                    }
                } else if (strcmp(s->ability_name, "Arcane Surge") == 0) {
                    // Veilstorm Mage — single-target nuke, pure glass cannon
                    entity_deal_damage(e, e->target, e->damage * 4, game);
                } else if (strcmp(s->ability_name, "Barrier") == 0) {
                    // Veilstorm Tank — strikes and raises a containment zone
                    float tx = e->target->x;
                    float ty = e->target->y;
                    entity_deal_damage(e, e->target, e->damage * 2, game);
                    for (int j = 0; j < MAX_ENTITIES; j++) {
                        Entity* runner = &pool[j];
                        if (runner->active && runner->mode == MODE_RUNNER && runner->team != e->team) {
                            float dist_sq = distance_squared(tx, ty, runner->x, runner->y);
                            if (dist_sq <= 90.0f * 90.0f) {
                                runner->slow_timer = 3.0f;
                                runner->slow_mult = 0.5f;
                            }
                        }
                    }
                } else {
                    // Defensive fallback (should never be reached: all 24 abilities are covered above)
                    entity_deal_damage(e, e->target, e->damage * 2, game);
                }
            }

            if (e->anim.finished) animator_play(&e->anim, &ANIM_IDLE);
            continue;
        }
    }
}

// -----------------------------------------------------------------------------
// RENDER
// -----------------------------------------------------------------------------
// Renders in passes grouped by color instead of interleaved per entity:
// the HP bar, tier dots, and the debug range circle all use the SAME fixed
// color for the ~128 entities in the pool, so setting that color once per
// pass (instead of once per entity) avoids hundreds of redundant
// SET_PRIM_COLOR commands per frame when the pool is full.
//
// Behavior note: with the old interleaved render, if two entities visually
// overlapped, the decoration (HP bar/dots) of the first one could end up
// covered by the sprite of the second if it was drawn afterward. With
// separate passes, decorations ALWAYS end up on top of all sprites — more
// readable, but it's a visible behavior change worth confirming on the
// emulator.
void entities_render(const Camera* cam, bool show_debug) {
    rdpq_set_mode_standard();

    // Pass 1: sprite or color rect per entity (the tint varies per unit,
    // so the color change can't be avoided here).
    for (int i = 0; i < MAX_ENTITIES; i++) {
        Entity* e = &pool[i];
        if (!e->active) continue;

        float rx = e->x, ry = e->y;
        camera_apply(cam, &rx, &ry);

        if (e->sprite) {
            rdpq_mode_combiner(RDPQ_COMBINER_TEX);
            if (e->anim.current && e->anim.current->frame_width > 0) {
                // Animated sprite sheet: crops the current frame from the horizontal strip.
                // start_frame allows a single sheet to contain several concatenated
                // animations (e.g. idle+walk+attack in one strip) — each AnimDef
                // marks the offset where its own cycle starts within the sheet.
                int frame = e->anim.current->start_frame + e->anim.frame_idx;
                rdpq_sprite_blit(e->sprite, (int)rx, (int)ry, &(rdpq_blitparms_t){
                    .s0    = frame * e->anim.current->frame_width,
                    .width = e->anim.current->frame_width,
                });
            } else {
                // Single-pose sprite (current case): no sub-rect.
                rdpq_sprite_blit(e->sprite, (int)rx, (int)ry, NULL);
            }
        } else {
            rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
            rdpq_set_prim_color(e->tint);
            rdpq_fill_rectangle(rx, ry, rx + e->w, ry + e->h);
        }
    }

    // Pass 2: HP bar background — same red for the whole pool.
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(180, 0, 0, 255));
    for (int i = 0; i < MAX_ENTITIES; i++) {
        Entity* e = &pool[i];
        if (!e->active || e->is_projectile || e->hp_max <= 0) continue;
        float rx = e->x, ry = e->y;
        camera_apply(cam, &rx, &ry);
        rdpq_fill_rectangle(rx, ry - 5, rx + e->w, ry - 2);
    }

    // Pass 3: HP bar fill — same green for the whole pool.
    rdpq_set_prim_color(RGBA32(0, 220, 60, 255));
    for (int i = 0; i < MAX_ENTITIES; i++) {
        Entity* e = &pool[i];
        if (!e->active || e->is_projectile || e->hp_max <= 0) continue;
        float rx = e->x, ry = e->y;
        camera_apply(cam, &rx, &ry);
        float pct = (float)e->hp / e->hp_max;
        rdpq_fill_rectangle(rx, ry - 5, rx + e->w * pct, ry - 2);
    }

    // Pass 4: upgrade dots — same gold color for all towers.
    bool any_tier = false;
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (pool[i].active && pool[i].mode == MODE_TOWER && pool[i].upgrade_tier > 0) { any_tier = true; break; }
    }
    if (any_tier) {
        rdpq_set_prim_color(RGBA32(255, 215, 0, 255));
        for (int i = 0; i < MAX_ENTITIES; i++) {
            Entity* e = &pool[i];
            if (!e->active || e->mode != MODE_TOWER || e->upgrade_tier <= 0) continue;
            float rx = e->x, ry = e->y;
            camera_apply(cam, &rx, &ry);
            for (int t = 0; t < e->upgrade_tier; t++) {
                rdpq_fill_rectangle(rx + t*4, ry + e->h + 2,
                                    rx + t*4 + 3, ry + e->h + 5);
            }
        }
    }

    // Pass 5 (debug only): approximate range circle — same translucent white.
    if (show_debug) {
        rdpq_set_prim_color(RGBA32(255, 255, 255, 40));
        for (int i = 0; i < MAX_ENTITIES; i++) {
            Entity* e = &pool[i];
            if (!e->active || e->mode != MODE_TOWER || e->attack_range <= 0) continue;
            float rx = e->x, ry = e->y;
            camera_apply(cam, &rx, &ry);
            float r = e->attack_range;
            float cx = rx + e->w/2, cy = ry + e->h/2;
            rdpq_fill_rectangle(cx - r, cy - 2, cx + r, cy + 2);
            rdpq_fill_rectangle(cx - 2, cy - r, cx + 2, cy + r);
        }
    }
}
