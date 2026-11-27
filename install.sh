#!/usr/bin/env bash
# ============================================================================
# Mizzle Desktop Environment Installation & Update Script
# ============================================================================

set -euo pipefail

INSTALL_DIR="${MIZZLE_DIR:-$HOME/.mizzle}"
LOCAL_PREFIX="$HOME/.local"
BIN_TARGET="$LOCAL_PREFIX/bin/mizzle"
CORE_BIN_TARGET="$LOCAL_PREFIX/bin/liquid-glass-ui"
REPO_URL="https://github.com/TattvaOrg/mizzle.git"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
DIM='\033[2m'
BOLD='\033[1m'
NC='\033[0m'

TARGET_CHANNEL="" # "release", "main", or "local"
NON_INTERACTIVE=false

for arg in "$@"; do
    case "$arg" in
        --release|--latest)
            TARGET_CHANNEL="release"
            ;;
        --main)
            TARGET_CHANNEL="main"
            ;;
        --local)
            TARGET_CHANNEL="local"
            ;;
        -y|--yes|--non-interactive)
            NON_INTERACTIVE=true
            ;;
        -h|--help)
            echo "Mizzle Installer & Updater"
            echo ""
            echo "Usage: ./install.sh [options]"
            echo ""
            echo "Options:"
            echo "  --release, --latest Install/update to the latest stable update/release"
            echo "  --main              Install/update to the latest development commit on main"
            echo "  --local             Install from the current local repository directory"
            echo "  -y, --yes           Non-interactive mode (use defaults)"
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

SCRIPT_SOURCE=""
SCRIPT_DIR=""
if [[ -n "${BASH_SOURCE[0]:-}" ]] && [[ -f "${BASH_SOURCE[0]}" ]]; then
    SCRIPT_SOURCE="$(readlink -f "${BASH_SOURCE[0]}")"
    SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_SOURCE")" && pwd)"
fi

IS_LOCAL_REPO=false
if [[ -n "$SCRIPT_DIR" ]] && [[ "$SCRIPT_DIR" != "$INSTALL_DIR" ]]; then
    if [[ -f "$SCRIPT_DIR/CMakeLists.txt" ]] && [[ -f "$SCRIPT_DIR/mizzle" ]] && [[ -d "$SCRIPT_DIR/src" ]]; then
        IS_LOCAL_REPO=true
    fi
fi

# Detect existing Mizzle installation
IS_UPDATE=false
if [[ -d "$INSTALL_DIR" ]] || [[ -L "$BIN_TARGET" ]] || [[ -f "$BIN_TARGET" ]] || [[ -f "$CORE_BIN_TARGET" ]]; then
    IS_UPDATE=true
fi

WAS_RUNNING=false
if pgrep -x "liquid-glass-ui" >/dev/null 2>&1; then
    WAS_RUNNING=true
fi

echo -e "${CYAN}${BOLD}"
echo "  ========================================"
if [[ "$IS_UPDATE" == true ]]; then
    echo "               Mizzle Updater             "
else
    echo "              Mizzle Installer            "
fi
echo "  ========================================"
echo -e "${NC}"

if [[ "$IS_UPDATE" == true ]]; then
    echo -e "${BLUE}Detected existing Mizzle installation. Updating in $INSTALL_DIR...${NC}"
else
    echo -e "${BLUE}Installing Mizzle Desktop Environment into $INSTALL_DIR...${NC}"
fi
echo ""

# 0. Dependency check
check_deps() {
    local missing=()
    for cmd in cmake pkg-config; do
        command -v "$cmd" &>/dev/null || missing+=("$cmd")
    done
    if ! command -v g++ &>/dev/null && ! command -v clang++ &>/dev/null; then
        missing+=("g++/clang++")
    fi
    if command -v pkg-config &>/dev/null; then
        for mod in gtk4 gtk4-layer-shell-0 epoxy pangocairo egl; do
            pkg-config --exists "$mod" 2>/dev/null || missing+=("pkg:$mod")
        done
    fi
    if [[ ${#missing[@]} -gt 0 ]]; then
        echo -e "${RED}Missing required build dependencies: ${missing[*]}${NC}"
        echo "Install them (e.g. on Arch: sudo pacman -S cmake ninja gcc pkgconf gtk4 gtk4-layer-shell libepoxy pango cairo grim)"
        exit 1
    fi
    if ! command -v grim &>/dev/null; then
        echo -e "${YELLOW}[warn] 'grim' not found. Install 'grim' for live desktop backdrop capture.${NC}"
    fi
}
check_deps

# Handle local clone source prompt
if [[ "$IS_LOCAL_REPO" == true ]] && [[ -z "$TARGET_CHANNEL" ]]; then
    echo -e "${CYAN}Running installer from local repository: ${BOLD}$SCRIPT_DIR${NC}"
    echo "Choose installation source:"
    echo -e "  ${BOLD}1)${NC} Fetch & install/update from GitHub to $INSTALL_DIR ${GREEN}(Recommended)${NC}"
    echo -e "  ${BOLD}2)${NC} Install from current local directory into $INSTALL_DIR ${DIM}(Developer mode)${NC}"
    echo ""
    src_choice="$(prompt_user "Select option [1/2] (default: 1): " "1")"
    if [[ "$src_choice" == "2" ]]; then
        TARGET_CHANNEL="local"
    fi
    echo ""
fi

# 1. Source Preparation
echo -e "${BLUE}[1/6] Preparing Mizzle source files...${NC}"

if [[ "$TARGET_CHANNEL" == "local" ]]; then
    echo -e "  Syncing local files from $SCRIPT_DIR to $INSTALL_DIR..."
    mkdir -p "$INSTALL_DIR"
    if command -v rsync &>/dev/null; then
        rsync -a --exclude='.git' --exclude='build' "$SCRIPT_DIR/" "$INSTALL_DIR/"
    else
        cp -rf "$SCRIPT_DIR/CMakeLists.txt" "$SCRIPT_DIR/mizzle" "$SCRIPT_DIR/install.sh" \
               "$SCRIPT_DIR/update.sh" "$SCRIPT_DIR/uninstall.sh" "$SCRIPT_DIR/view.sh" \
               "$SCRIPT_DIR/README.md" "$SCRIPT_DIR/LICENSE" "$INSTALL_DIR/" 2>/dev/null || true
        mkdir -p "$INSTALL_DIR/src" "$INSTALL_DIR/shaders"
        cp -rf "$SCRIPT_DIR/src/." "$INSTALL_DIR/src/"
        cp -rf "$SCRIPT_DIR/shaders/." "$INSTALL_DIR/shaders/"
    fi
    echo -e "${GREEN}  [ok] Synced local repository to $INSTALL_DIR${NC}"
else
    if ! command -v git &>/dev/null; then
        echo -e "${RED}Error: git is required to install or update Mizzle from GitHub.${NC}"
        exit 1
    fi

    if [[ ! -d "$INSTALL_DIR/.git" ]]; then
        if [[ -d "$INSTALL_DIR" ]]; then
            BACKUP_DIR="${INSTALL_DIR}.bak.$(date +%s)"
            echo -e "${YELLOW}  Notice: $INSTALL_DIR exists but is not a git repository. Backing up to $BACKUP_DIR...${NC}"
            mv "$INSTALL_DIR" "$BACKUP_DIR"
        fi
        echo -e "  Cloning repository into $INSTALL_DIR..."
        git clone "$REPO_URL" "$INSTALL_DIR"
    fi

    echo -e "  Fetching latest updates and release tags..."
    git -C "$INSTALL_DIR" fetch --tags origin --quiet

    LATEST_TAG="$(git -C "$INSTALL_DIR" tag -l --sort=-v:refname | head -n 1)"
    MAIN_REV="$(git -C "$INSTALL_DIR" rev-parse --short origin/main 2>/dev/null || echo 'main')"

    if [[ -z "$TARGET_CHANNEL" ]]; then
        echo ""
        if [[ "$IS_UPDATE" == true ]]; then
            echo -e "${BOLD}Select update channel:${NC}"
        else
            echo -e "${BOLD}Select installation channel:${NC}"
        fi
        if [[ -n "$LATEST_TAG" ]]; then
            echo -e "  ${BOLD}1)${NC} Latest Update / Release ${GREEN}(Recommended - tag: $LATEST_TAG)${NC}"
        else
            echo -e "  ${BOLD}1)${NC} Latest Update ${GREEN}(Recommended - latest verified main: $MAIN_REV)${NC}"
        fi
        echo -e "  ${BOLD}2)${NC} Main Branch ${DIM}(Bleeding edge - origin/main @ $MAIN_REV)${NC}"
        echo ""
        channel_choice="$(prompt_user "Choice [1/2] (default: 1): " "1")"
        if [[ "$channel_choice" == "2" ]]; then
            TARGET_CHANNEL="main"
        else
            TARGET_CHANNEL="release"
        fi
        echo ""
    fi

    git -C "$INSTALL_DIR" stash --quiet 2>/dev/null || true
    if [[ "$TARGET_CHANNEL" == "release" ]]; then
        if [[ -n "$LATEST_TAG" ]]; then
            echo -e "  Checking out latest release: ${BOLD}$LATEST_TAG${NC}..."
            git -C "$INSTALL_DIR" checkout "$LATEST_TAG" --quiet
            echo -e "${GREEN}  [ok] Checked out release $LATEST_TAG${NC}"
        else
            echo -e "  No release tags published yet; updating to latest ${BOLD}main${NC}..."
            git -C "$INSTALL_DIR" checkout main --quiet
            git -C "$INSTALL_DIR" pull --ff-only origin main --quiet
            echo -e "${GREEN}  [ok] Updated to latest main ($MAIN_REV)${NC}"
        fi
    else
        echo -e "  Checking out ${BOLD}main${NC} branch (latest development)..."
        git -C "$INSTALL_DIR" checkout main --quiet
        git -C "$INSTALL_DIR" pull --ff-only origin main --quiet
        echo -e "${GREEN}  [ok] Updated to latest main${NC}"
    fi
fi

# 2. Build & Install Native C++20 Binary + Shaders
echo -e "${BLUE}[2/6] Building Mizzle with CMake...${NC}"
GEN_ARGS=()
if command -v ninja &>/dev/null; then
    GEN_ARGS=(-G Ninja)
fi
cmake -S "$INSTALL_DIR" -B "$INSTALL_DIR/build" "${GEN_ARGS[@]}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$LOCAL_PREFIX"
cmake --build "$INSTALL_DIR/build" -j
cmake --install "$INSTALL_DIR/build"
echo -e "${GREEN}  [ok] Built and installed liquid-glass-ui and shaders to $LOCAL_PREFIX${NC}"

# 3. Link CLI & Desktop Entry
echo -e "${BLUE}[3/6] Linking CLI to $BIN_TARGET...${NC}"
mkdir -p "$LOCAL_PREFIX/bin" "$LOCAL_PREFIX/share/applications"
chmod +x "$INSTALL_DIR/mizzle" "$INSTALL_DIR/install.sh" "$INSTALL_DIR/update.sh" "$INSTALL_DIR/uninstall.sh" "$INSTALL_DIR/view.sh"
ln -sf "$INSTALL_DIR/mizzle" "$BIN_TARGET"

cat <<'EOF' > "$LOCAL_PREFIX/share/applications/mizzle.desktop"
[Desktop Entry]
Type=Application
Name=Mizzle Settings
Comment=macOS Tahoe Liquid Glass Desktop Environment
Exec=mizzle settings
Terminal=false
Categories=Settings;DesktopSettings;
EOF

if command -v update-desktop-database &>/dev/null; then
    update-desktop-database "$LOCAL_PREFIX/share/applications" 2>/dev/null || true
fi
echo -e "${GREEN}  [ok] CLI symlink: $BIN_TARGET -> $INSTALL_DIR/mizzle${NC}"

# 4. Compositor & Autostart Integration
echo -e "${BLUE}[4/6] Configuring desktop environment autostart...${NC}"
AUTOSTART_DIR="$HOME/.config/autostart"
mkdir -p "$AUTOSTART_DIR"
cat <<'EOF' > "$AUTOSTART_DIR/mizzle.desktop"
[Desktop Entry]
Type=Application
Name=Mizzle
Comment=macOS Tahoe Liquid Glass Desktop Environment UI
Exec=mizzle start
Terminal=false
Categories=Utility;
X-GNOME-Autostart-enabled=true
EOF
echo -e "${GREEN}  [ok] Configured XDG autostart (~/.config/autostart/mizzle.desktop)${NC}"

NIRI_CONFIG="$HOME/.config/niri/config.kdl"
if [[ -f "$NIRI_CONFIG" ]]; then
    python3 - <<PYEOF 2>/dev/null || true
kdl_path = "$NIRI_CONFIG"
with open(kdl_path, "r") as f:
    content = f.read()

def remove_mizzle_layer_rules(text):
    result = []
    i = 0
    while i < len(text):
        idx = text.find("layer-rule", i)
        if idx == -1:
            result.append(text[i:])
            break
        brace_start = text.find("{", idx)
        if brace_start == -1:
            result.append(text[i:])
            break
        depth = 1
        pos = brace_start + 1
        while pos < len(text) and depth > 0:
            if text[pos] == "{":
                depth += 1
            elif text[pos] == "}":
                depth -= 1
            pos += 1
        block = text[idx:pos]
        if 'namespace="^mizzle-' in block or 'namespace="mizzle-' in block:
            while pos < len(text) and text[pos] in " \t\r\n":
                if text[pos] == "\n":
                    pos += 1
                    break
                pos += 1
            result.append(text[i:idx])
            i = pos
        else:
            result.append(text[i:pos])
            i = pos
    return "".join(result)

content = remove_mizzle_layer_rules(content)
lines = [ln for ln in content.splitlines(True) if not ('spawn-at-startup' in ln and 'mizzle' in ln)]
content = "".join(lines).rstrip() + "\n\n// Start Mizzle Desktop Environment UI\nspawn-at-startup \"mizzle\" \"start\"\n"
with open(kdl_path, "w") as f:
    f.write(content)
PYEOF
    if command -v niri &>/dev/null && niri validate >/dev/null 2>&1; then
        niri msg action load-config-file >/dev/null 2>&1 || true
        echo -e "${GREEN}  [ok] Configured Niri startup and validated config.kdl${NC}"
    else
        echo -e "${GREEN}  [ok] Added Mizzle startup entry to Niri config${NC}"
    fi
fi

HYPR_CONFIG="$HOME/.config/hypr/hyprland.conf"
if [[ -f "$HYPR_CONFIG" ]]; then
    if ! grep -q "mizzle start" "$HYPR_CONFIG"; then
        printf "\n# Start Mizzle Desktop Environment UI\nexec-once = mizzle start\n" >> "$HYPR_CONFIG"
        echo -e "${GREEN}  [ok] Added Mizzle startup entry to Hyprland config${NC}"
    fi
fi

# 5. Restart if running during update
echo -e "${BLUE}[5/6] Checking active Mizzle service...${NC}"
if [[ "$WAS_RUNNING" == true ]]; then
    echo -e "  Restarting Mizzle to apply update..."
    "$BIN_TARGET" restart || true
else
    echo -e "${DIM}  Mizzle was not running. Start anytime with 'mizzle start'.${NC}"
fi

# 6. PATH check
echo -e "${BLUE}[6/6] Verifying PATH...${NC}"
if echo "$PATH" | grep -q "$LOCAL_PREFIX/bin"; then
    echo -e "${GREEN}  [ok] $LOCAL_PREFIX/bin is in your PATH${NC}"
else
    echo -e "${YELLOW}  [warn] $LOCAL_PREFIX/bin is NOT in your PATH${NC}"
    echo "  Add this to ~/.bashrc or ~/.zshrc:"
    echo "    export PATH=\"\$HOME/.local/bin:\$PATH\""
fi

echo ""
if [[ "$IS_UPDATE" == true ]]; then
    echo -e "${GREEN}${BOLD}Mizzle successfully updated in $INSTALL_DIR.${NC}"
else
    echo -e "${GREEN}${BOLD}Mizzle installation complete in $INSTALL_DIR.${NC}"
fi

echo ""
echo "Usage:"
echo -e "  ${CYAN}mizzle start${NC}         Start desktop environment (Bar, Dock, Widgets)"
echo -e "  ${CYAN}mizzle stop${NC}          Stop desktop environment"
echo -e "  ${CYAN}mizzle restart${NC}       Restart desktop environment"
echo -e "  ${CYAN}mizzle spotlight${NC}     Toggle Spotlight Search"
echo -e "  ${CYAN}mizzle settings${NC}      Open Shader & Widget Lab"
echo -e "  ${CYAN}mizzle update${NC}        Update Mizzle (release or main branch)"
echo -e "  ${CYAN}mizzle uninstall${NC}     Uninstall Mizzle"
