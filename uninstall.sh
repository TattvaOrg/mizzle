#!/usr/bin/env bash
# ============================================================================
# Mizzle Desktop Environment Uninstaller
# ============================================================================

set -euo pipefail

INSTALL_DIR="${MIZZLE_DIR:-$HOME/.mizzle}"
CONFIG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/mizzle"
LOCAL_PREFIX="$HOME/.local"
BIN_TARGET="$LOCAL_PREFIX/bin/mizzle"
CORE_BIN_TARGET="$LOCAL_PREFIX/bin/liquid-glass-ui"
RUNTIME_DIR="${XDG_RUNTIME_DIR:-/tmp}"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
DIM='\033[2m'
BOLD='\033[1m'
NC='\033[0m'

NON_INTERACTIVE=false

for arg in "$@"; do
    case "$arg" in
        -y|--yes|--non-interactive)
            NON_INTERACTIVE=true
            ;;
        -h|--help)
            echo "Mizzle Uninstaller"
            echo ""
            echo "Usage: ./uninstall.sh [options]"
            echo ""
            echo "Options:"
            echo "  -y, --yes           Non-interactive mode (preserves configs/repo by default)"
            echo "  -h, --help          Show this help message"
            exit 0
            ;;
    esac
done

prompt_user() {
    local prompt_msg="$1"
    local default_val="${2:-}"
    local reply=""

    if [[ "$NON_INTERACTIVE" == true ]]; then
        echo "$default_val"
        return
    fi

    if [[ -t 0 ]]; then
        read -r -p "$prompt_msg" reply
    elif [[ -e /dev/tty ]]; then
        read -r -p "$prompt_msg" reply < /dev/tty
    else
        reply="$default_val"
    fi

    if [[ -z "$reply" ]]; then
        reply="$default_val"
    fi
    echo "$reply"
}

echo -e "${CYAN}${BOLD}"
echo "  ========================================"
echo "             Mizzle Uninstaller           "
echo "  ========================================"
echo -e "${NC}"

# 1. Stop active Mizzle processes
echo -e "${BLUE}[1/5] Stopping active Mizzle instances...${NC}"
if pgrep -x "liquid-glass-ui" >/dev/null 2>&1; then
    pkill -x "liquid-glass-ui" 2>/dev/null || true
    sleep 0.3
    echo -e "${GREEN}  [ok] Mizzle stopped${NC}"
else
    echo -e "${DIM}  No active Mizzle process found.${NC}"
fi
rm -f "$RUNTIME_DIR/mizzle.sock" "$RUNTIME_DIR/mizzle.pid" 2>/dev/null || true

# 2. Remove binaries, shaders, and desktop entry
echo -e "${BLUE}[2/5] Removing installed binaries and assets...${NC}"
removed_any=false
for path in \
    "$BIN_TARGET" \
    "$CORE_BIN_TARGET" \
    "$LOCAL_PREFIX/share/applications/mizzle.desktop"; do
    if [[ -L "$path" ]] || [[ -f "$path" ]]; then
        rm -f "$path"
        echo -e "${GREEN}  [ok] Removed $path${NC}"
        removed_any=true
    fi
done
if [[ -d "$LOCAL_PREFIX/share/mizzle" ]]; then
    rm -rf "$LOCAL_PREFIX/share/mizzle"
    echo -e "${GREEN}  [ok] Removed $LOCAL_PREFIX/share/mizzle${NC}"
    removed_any=true
fi
if [[ "$removed_any" == false ]]; then
    echo -e "${DIM}  No installed binaries or assets found.${NC}"
fi

# 3. Remove XDG autostart
echo -e "${BLUE}[3/5] Removing XDG autostart entry...${NC}"
AUTOSTART_FILE="$HOME/.config/autostart/mizzle.desktop"
if [[ -f "$AUTOSTART_FILE" ]]; then
    rm -f "$AUTOSTART_FILE"
    echo -e "${GREEN}  [ok] Removed $AUTOSTART_FILE${NC}"
else
    echo -e "${DIM}  XDG autostart entry was not present.${NC}"
fi

# 4. Clean compositor autostart configs
echo -e "${BLUE}[4/5] Cleaning compositor configurations...${NC}"
NIRI_CONFIG="$HOME/.config/niri/config.kdl"
if [[ -f "$NIRI_CONFIG" ]]; then
    python3 - <<PYEOF 2>/dev/null || true
kdl_path = "$NIRI_CONFIG"
with open(kdl_path, "r") as f:
    lines = f.readlines()

cleaned = []
for ln in lines:
    s = ln.strip()
    if s == "// Start Mizzle Desktop Environment UI":
        continue
    if s.startswith("spawn-at-startup") and "mizzle" in s:
        continue
    cleaned.append(ln)

with open(kdl_path, "w") as f:
    f.writelines(cleaned)
PYEOF
    if command -v niri &>/dev/null && niri validate >/dev/null 2>&1; then
        niri msg action load-config-file >/dev/null 2>&1 || true
    fi
    echo -e "${GREEN}  [ok] Cleaned Niri config${NC}"
fi

HYPR_CONFIG="$HOME/.config/hypr/hyprland.conf"
if [[ -f "$HYPR_CONFIG" ]]; then
    python3 - <<PYEOF 2>/dev/null || true
conf_path = "$HYPR_CONFIG"
with open(conf_path, "r") as f:
    lines = f.readlines()
cleaned = [ln for ln in lines if ("mizzle start" not in ln and "# Start Mizzle Desktop Environment UI" not in ln)]
with open(conf_path, "w") as f:
    f.writelines(cleaned)
PYEOF
    echo -e "${GREEN}  [ok] Cleaned Hyprland config${NC}"
fi

# 5. Optional cleanup of user config and local clone
echo -e "${BLUE}[5/5] Cleaning data directories...${NC}"
if [[ -d "$CONFIG_DIR" ]]; then
    del_config="$(prompt_user "Remove user configuration directory ($CONFIG_DIR)? [y/N]: " "n")"
    if [[ "$del_config" =~ ^[Yy]$ ]]; then
        rm -rf "$CONFIG_DIR"
        echo -e "${GREEN}  [ok] Removed $CONFIG_DIR${NC}"
    else
        echo -e "${DIM}  Preserved $CONFIG_DIR${NC}"
    fi
fi

DELETE_INSTALL_DIR=false
if [[ -d "$INSTALL_DIR" ]]; then
    del_install="$(prompt_user "Remove Mizzle installation directory ($INSTALL_DIR)? [y/N]: " "n")"
    if [[ "$del_install" =~ ^[Yy]$ ]]; then
        DELETE_INSTALL_DIR=true
    else
        echo -e "${DIM}  Preserved $INSTALL_DIR${NC}"
    fi
fi

echo ""
echo -e "${GREEN}${BOLD}Mizzle has been uninstalled.${NC}"

if [[ "$DELETE_INSTALL_DIR" == true ]]; then
    rm -rf "$INSTALL_DIR"
    echo -e "${GREEN}  [ok] Removed $INSTALL_DIR${NC}"
fi
