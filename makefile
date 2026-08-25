BUILD_DIR    = build
ASSETS_DIR  = assets/sprites
FS_DIR      = filesystem

PROJECT = TD64

# ═════════════════════════════════════════════════════════════════════════════
# SOURCE FILES — Organized by category
# ═════════════════════════════════════════════════════════════════════════════

# Core
CORE_SOURCES = src/core/main.c

# Config (data only, no .c files need compiling from here except units_data.c)
CONFIG_SOURCES = src/config/units_data.c

# Entities
ENTITY_SOURCES = \
	src/entities/entities.c \
	src/entities/animation.c \
	src/entities/collision.c

# World
WORLD_SOURCES = \
	src/world/terrain.c \
	src/world/maps.c \
	src/world/pathfinding.c

# Game Logic
GAME_SOURCES = \
	src/game/game.c \
	src/game/campaign.c

# Systems
SYSTEM_SOURCES = \
	src/systems/score.c \
	src/systems/leveling.c \
	src/systems/effects.c \
	src/systems/debug.c \
	src/systems/audio.c \
	src/systems/save.c \
	src/systems/lang.c \
	src/systems/controls.c

# UI
UI_SOURCES = \
	src/ui/menu.c \
	src/ui/ui.c \
	src/ui/map_editor.c \
	src/ui/custom_map_menu.c \
	src/ui/controls_menu.c \
	src/ui/stats_menu.c

# Resources
RESOURCE_SOURCES = src/resources/resources.c

# All C sources combined
C_SOURCES = \
	$(CORE_SOURCES) \
	$(CONFIG_SOURCES) \
	$(ENTITY_SOURCES) \
	$(WORLD_SOURCES) \
	$(GAME_SOURCES) \
	$(SYSTEM_SOURCES) \
	$(UI_SOURCES) \
	$(RESOURCE_SOURCES)

# ═════════════════════════════════════════════════════════════════════════════
# ASSETS — Sprites (PNG → .sprite)
# ═════════════════════════════════════════════════════════════════════════════

SPRITE_PNGS  = $(wildcard $(ASSETS_DIR)/*.png)
SPRITE_FILES = $(patsubst $(ASSETS_DIR)/%.png, $(FS_DIR)/%.sprite, $(SPRITE_PNGS))

FILESYSTEM_FILES = $(SPRITE_FILES)

# ═════════════════════════════════════════════════════════════════════════════
# BUILD FLAGS
# ═════════════════════════════════════════════════════════════════════════════

ROOTDIR = $(N64_INST)

include $(N64_INST)/include/n64.mk

# Global overrides to force the MIPS cross-compilation toolchain and flags
CC      = $(N64_CC)
CXX     = $(N64_CXX)
AS      = $(N64_AS)
LD      = $(N64_LD)
ASFLAGS = $(N64_ASFLAGS)

N64_CFLAGS += -Wall -Wextra -O2 -g \
              -Isrc/core \
              -Isrc/config \
              -Isrc/entities \
              -Isrc/world \
              -Isrc/game \
              -Isrc/systems \
              -Isrc/ui \
              -Isrc/resources

CFLAGS  = $(N64_CFLAGS)

# Target-specific LDFLAGS for ELF files to prevent the inherited %.z64 LDFLAGS from duplicating the linker script
$(BUILD_DIR)/%.elf: LDFLAGS = $(N64_LDFLAGS)

OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

$(BUILD_DIR)/$(PROJECT).elf: $(OBJS)

# ═════════════════════════════════════════════════════════════════════════════
# ROM
# ═════════════════════════════════════════════════════════════════════════════

ifeq ($(strip $(FILESYSTEM_FILES)),)
$(PROJECT).z64: N64_ROM_TITLE = "TD64"
$(PROJECT).z64: $(BUILD_DIR)/$(PROJECT).elf
else
$(PROJECT).z64: N64_ROM_TITLE = "TD64"
$(PROJECT).z64: $(BUILD_DIR)/$(PROJECT).dfs
$(BUILD_DIR)/$(PROJECT).dfs: $(FILESYSTEM_FILES)
endif

# ═════════════════════════════════════════════════════════════════════════════
# ASSET CONVERSION RULES
# ═════════════════════════════════════════════════════════════════════════════

$(FS_DIR)/%.sprite: $(ASSETS_DIR)/%.png | $(FS_DIR)
	@echo "  MKSPRITE  $@"
	@mksprite --format RGBA16 --output $(FS_DIR) $<

$(FS_DIR):
	@mkdir -p $(FS_DIR)

# ═════════════════════════════════════════════════════════════════════════════
# UTILITY TARGETS
# ═════════════════════════════════════════════════════════════════════════════

.PHONY: sprites gen-sprites gen-maps clean help tree

sprites: $(FS_DIR) $(SPRITE_FILES)
	@echo "✓ $(words $(SPRITE_FILES)) sprites converted"

gen-sprites:
	@echo "Generating sprites from Python..."
	@cd tools && python3 gen_sprites.py
	@echo "✓ Sprites generated in $(ASSETS_DIR)/"

gen-maps:
	@echo "Generating maps and terrain tiles from Python..."
	@cd tools && python3 gen_maps.py
	@echo "✓ Maps generated in $(ASSETS_DIR)/"

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(FS_DIR)
	@echo "✓ Clean complete"

tree:
	@echo "Project structure:"
	@tree -L 3 -I 'build|filesystem' . 2>/dev/null || find . -type d -not -path '*/build*' -not -path '*/filesystem*' | sort

help:
	@echo ""
	@echo "  ╔═══════════════════════════════════════════════════════════╗"
	@echo "  ║           TD64 — Nintendo 64                   ║"
	@echo "  ╚═══════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "  COMMANDS:"
	@echo "    make              Build ROM (converts sprites automatically)"
	@echo "    make gen-sprites  Regenerate PNG sprites from Python script"
	@echo "    make sprites      Only convert PNGs → .sprite format"
	@echo "    make clean        Remove build/ and filesystem/"
	@echo "    make tree         Show project structure"
	@echo "    make help         Show this help"
	@echo ""
	@echo "  PROJECT STRUCTURE:"
	@echo "    src/core/         Engine core and main loop"
	@echo "    src/config/       Game configuration and data"
	@echo "    src/entities/     Units, towers, projectiles"
	@echo "    src/world/        Maps, terrain, pathfinding"
	@echo "    src/game/         Game logic, waves, campaign"
	@echo "    src/systems/      Score, effects, debug, leveling"
	@echo "    src/ui/           Menus and HUD"
	@echo "    src/resources/    Asset loading"
	@echo ""
	@echo "  CONFIGURATION:"
	@echo "    src/config/game_config.h   — Master balance file"
	@echo "    src/config/units_data.c    — All unit stats"
	@echo ""
	@echo "  CONTROLS (defaults — remappable from the main menu's CONTROLS screen):"
	@echo "    D-pad      Move cursor / auto-scroll camera on bigger maps"
	@echo "    A          Place unit       |  C-right   Upgrade tower"
	@echo "    B          Cancel           |  C-up      Debug menu"
	@echo "    L/R        Cycle unit       |  Z         Next wave"
	@echo "    Start      Pause"
	@echo ""
	@echo "  For detailed docs, see README.md"
	@echo ""

.PHONY: all
all: $(PROJECT).z64
