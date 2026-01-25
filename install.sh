#!/bin/bash
#
# RetroPac Installation Script
# 
# This script installs all dependencies and builds RetroPac for Raspberry Pi / RetroPie.
#
# Usage:
#   chmod +x install.sh
#   ./install.sh
#
# Options:
#   --no-web      Skip building the web animation editor
#   --no-server   Skip building the anim-server
#   --no-install  Build only, don't install to system
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Parse command line options
BUILD_WEB=true
BUILD_SERVER=true
DO_INSTALL=true

for arg in "$@"; do
    case $arg in
        --no-web)
            BUILD_WEB=false
            ;;
        --no-server)
            BUILD_SERVER=false
            ;;
        --no-install)
            DO_INSTALL=false
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --no-web      Skip building the web animation editor"
            echo "  --no-server   Skip building the anim-server"
            echo "  --no-install  Build only, don't install to system"
            echo ""
            exit 0
            ;;
    esac
done

echo -e "${BLUE}"
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║           RetroPac Installation Script                    ║"
echo "║     Ultimarc I-PAC LED Controller for RetroPie            ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Check if running as root (we don't want that for the main install)
if [ "$EUID" -eq 0 ]; then
    echo -e "${YELLOW}Warning: Running as root. Some steps will use sudo anyway.${NC}"
fi

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${GREEN}Working directory: $SCRIPT_DIR${NC}"
echo ""

# ============================================================================
# Step 1: Install system dependencies
# ============================================================================
echo -e "${BLUE}[1/7] Installing system dependencies...${NC}"

sudo apt-get update

# Core dependencies for retropac
sudo apt-get install -y \
    build-essential \
    libjson-c-dev \
    libusb-1.0-0-dev

# Dependencies for anim-server
if [ "$BUILD_SERVER" = true ]; then
    sudo apt-get install -y libmicrohttpd-dev
fi

# Optional: for RGBcommander config converter
sudo apt-get install -y libxml2-dev || true

echo -e "${GREEN}✓ System dependencies installed${NC}"
echo ""

# ============================================================================
# Step 2: Check and install Node.js (if building web)
# ============================================================================
if [ "$BUILD_WEB" = true ]; then
    echo -e "${BLUE}[2/7] Checking Node.js installation...${NC}"
    
    # Check if Node.js is installed
    if command -v node &> /dev/null; then
        NODE_VERSION=$(node --version)
        echo -e "${GREEN}Node.js is already installed: $NODE_VERSION${NC}"
        
        # Check if version is >= 18
        NODE_MAJOR=$(echo "$NODE_VERSION" | cut -d. -f1 | tr -d 'v')
        if [ "$NODE_MAJOR" -lt 18 ]; then
            echo -e "${YELLOW}Node.js version is too old. Installing Node.js 20.x...${NC}"
            curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
            sudo apt-get install -y nodejs
        fi
    else
        echo "Node.js not found. Installing Node.js 20.x LTS..."
        curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
        sudo apt-get install -y nodejs
    fi
    
    # Verify installation
    echo "Node.js version: $(node --version)"
    echo "npm version: $(npm --version)"
    
    echo -e "${GREEN}✓ Node.js ready${NC}"
else
    echo -e "${YELLOW}[2/7] Skipping Node.js (--no-web)${NC}"
fi
echo ""

# ============================================================================
# Step 3: Build retropac binary
# ============================================================================
echo -e "${BLUE}[3/7] Building retropac...${NC}"

make clean || true
make

echo -e "${GREEN}✓ retropac built successfully${NC}"
echo ""

# ============================================================================
# Step 4: Build RGBcommander converter (optional)
# ============================================================================
echo -e "${BLUE}[4/7] Building RGBcommander converter...${NC}"

make converter || {
    echo -e "${YELLOW}Note: RGBcommander converter build failed (optional component)${NC}"
}

echo -e "${GREEN}✓ Converter build complete${NC}"
echo ""

# ============================================================================
# Step 5: Build anim-server
# ============================================================================
if [ "$BUILD_SERVER" = true ]; then
    echo -e "${BLUE}[5/7] Building animation server...${NC}"
    
    make server
    
    echo -e "${GREEN}✓ anim-server built successfully${NC}"
else
    echo -e "${YELLOW}[5/7] Skipping anim-server (--no-server)${NC}"
fi
echo ""

# ============================================================================
# Step 6: Build web application
# ============================================================================
if [ "$BUILD_WEB" = true ]; then
    echo -e "${BLUE}[6/7] Building web application...${NC}"
    
    cd web
    
    echo "Installing npm packages (this may take a few minutes on Raspberry Pi)..."
    npm install
    
    echo "Building static web files..."
    npm run generate
    
    cd ..
    
    echo -e "${GREEN}✓ Web application built successfully${NC}"
else
    echo -e "${YELLOW}[6/7] Skipping web build (--no-web)${NC}"
fi
echo ""

# ============================================================================
# Step 7: Install to system
# ============================================================================
if [ "$DO_INSTALL" = true ]; then
    echo -e "${BLUE}[7/7] Installing to system...${NC}"
    
    sudo make install
    
    echo -e "${GREEN}✓ Installation complete${NC}"
else
    echo -e "${YELLOW}[7/7] Skipping system install (--no-install)${NC}"
fi
echo ""

# ============================================================================
# Done!
# ============================================================================
echo -e "${GREEN}"
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║              Installation Complete!                        ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

echo "What's installed:"
echo "  • retropac binary: /usr/local/bin/retropac"
echo "  • Config file: /etc/retropac/config.json"

if [ "$BUILD_SERVER" = true ]; then
    echo "  • Animation server: $SCRIPT_DIR/bin/anim-server"
fi

if [ "$BUILD_WEB" = true ]; then
    echo "  • Web editor files: $SCRIPT_DIR/web/dist/"
fi

echo ""
echo "Next steps:"
echo ""
echo "  1. Edit your configuration:"
echo "     nano /etc/retropac/config.json"
echo ""
echo "     Or use the web-based config editor (see step 3)"
echo ""
echo "  2. Test the LED control:"
echo "     retropac mame sf2"
echo ""

if [ "$BUILD_SERVER" = true ]; then
    IP_ADDR=$(hostname -I | awk '{print $1}')
    echo "  3. Start the animation editor server:"
    echo "     ./bin/anim-server"
    echo ""
    echo "     Then open in your browser:"
    echo "     http://$IP_ADDR:8080"
    echo ""
    echo "     The web interface includes:"
    echo "       • Animation Editor - Create and preview LED animations"
    echo "       • Config Editor - Edit button mappings, colors, and ROMs"
    echo ""
fi

echo "  4. Set up RetroPie integration (see docs/INTEGRATION.md):"
echo "     Add to /opt/retropie/configs/all/runcommand-onstart.sh:"
echo '     /usr/local/bin/retropac "$1" "$3"'
echo ""

echo -e "${GREEN}Enjoy RetroPac! 🕹️${NC}"
