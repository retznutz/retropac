# Animation Editor Setup Guide

This guide walks you through setting up the RetroPac Animation Editor web server on your Raspberry Pi.

## Prerequisites

- Raspberry Pi 3 or 4 running Raspberry Pi OS (or RetroPie)
- SSH access to your Raspberry Pi
- Another computer on the same network with a web browser

## Step 1: Install System Dependencies

SSH into your Raspberry Pi and run:

```bash
# Update package lists
sudo apt update

# Install build tools and libraries
sudo apt install -y build-essential libjson-c-dev libusb-1.0-0-dev libmicrohttpd-dev git
```

## Step 2: Install Node.js

The web application requires Node.js to build. Install Node.js 20.x LTS:

```bash
# Download and run the NodeSource setup script
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -

# Install Node.js
sudo apt install -y nodejs

# Verify installation
node --version
npm --version
```

You should see version numbers displayed (e.g., `v20.x.x` and `10.x.x`).

## Step 3: Clone or Copy RetroPac

If you haven't already, clone the RetroPac repository:

```bash
cd ~
git clone https://github.com/yourusername/retropac.git
cd retropac
```

Or if you're copying files manually, ensure all files are in `/home/pi/retropac/` (or your preferred location).

## Step 4: Build the HTTP Server

Build the animation editor server:

```bash
make server
```

This creates `bin/anim-server`.

## Step 5: Build the Web Application

Install Node.js dependencies and build the web app:

```bash
cd web
npm install
npm run generate
cd ..
```

This creates the static web files in `web/dist/`.

> **Note:** The first `npm install` may take several minutes on a Raspberry Pi.

## Step 6: Find Your Pi's IP Address

You'll need your Raspberry Pi's IP address to connect from another computer:

```bash
hostname -I
```

This will display something like: `192.168.1.100`

Note this IP address - you'll use it to access the editor.

## Step 7: Run the Animation Editor Server

Start the server:

```bash
./bin/anim-server
```

You should see:

```
===========================================
  RetroPac Animation Editor Server
===========================================

Server running at: http://localhost:8080
Animations directory: ./animations
Web directory: ./web/dist

Press Ctrl+C to stop the server.
```

## Step 8: Connect from Another Computer

On another computer connected to the same network:

1. Open a web browser (Chrome, Firefox, Safari, Edge)
2. Navigate to: `http://<your-pi-ip>:8080`
   
   For example: `http://192.168.1.100:8080`

3. The Animation Editor interface should load

## Using the Animation Editor

### Creating a New Animation

1. Click **+ New** in the Animations panel
2. Enter a name for your animation
3. Click buttons on the arcade panel to select them
4. Use the color picker to set the color
5. Click **Apply** to set the color on selected buttons
6. Adjust frame settings (fade, delay) as needed
7. Click **+** in the timeline to add more frames
8. **Drag and drop** frames in the timeline to reorder them
9. Click **💾 Save** to save your animation

### Previewing Animations

1. Click **▶ Preview** to play the animation
2. The currently playing frame is highlighted with a **yellow pulsing border** in the timeline
3. Click **⏹ Stop** to stop the preview

### Editing an Existing Animation

1. Click on an animation name in the left panel
2. The animation will load with all its frames
3. Click on frames in the timeline to edit them
4. Make your changes and click **Save**

### Animation Settings

| Setting | Description |
|---------|-------------|
| **Name** | Display name for the animation |
| **Speed (ms)** | Base timing interval between animation updates |
| **Loop** | Whether the animation repeats continuously |

### Frame Settings

| Setting | Description |
|---------|-------------|
| **Enable Fade** | Smooth transition to target colors |
| **Fade Speed (ms)** | Duration of the fade effect |
| **Delay (ms)** | Wait time before this frame executes |

## Server Options

The server accepts command-line options:

```bash
# Run on a different port
./bin/anim-server --port 3000

# Use a different animations directory
./bin/anim-server --animations-dir /home/pi/RetroPie/configs/retropac/animations

# Show all options
./bin/anim-server --help
```

### All Options

| Option | Default | Description |
|--------|---------|-------------|
| `--port <port>` | 8080 | HTTP server port |
| `--animations-dir <path>` | ./animations | Path to animations directory |
| `--web-dir <path>` | ./web/dist | Path to built web files |

## Firewall Configuration (If Needed)

If you can't connect from another computer, you may need to allow the port through the firewall:

```bash
# Using iptables
sudo iptables -A INPUT -p tcp --dport 8080 -j ACCEPT

# Or if using ufw
sudo ufw allow 8080/tcp
```

## Running from a Different Directory

If you want to run the server from a different location (e.g., after installing RetroPac):

```bash
./bin/anim-server \
    --animations-dir /home/pi/RetroPie/configs/retropac/animations \
    --web-dir /home/pi/retropac/web/dist
```

## Stopping the Server

Press **Ctrl+C** in the terminal where the server is running.

The server runs in the foreground intentionally - this way it only uses resources when you need it.

## Troubleshooting

### "Could not open animations directory"

Make sure the animations directory exists:

```bash
mkdir -p animations
```

### "Web directory not found"

Make sure you've built the web application:

```bash
cd web && npm run generate && cd ..
```

### Can't connect from another computer

1. Verify the Pi's IP address: `hostname -I`
2. Make sure you're on the same network
3. Check if the server is running
4. Try accessing locally first: `curl http://localhost:8080`
5. Check firewall settings

### npm install is very slow

This is normal on Raspberry Pi. The first install can take 5-10 minutes. Subsequent builds are faster.

### "ENOSPC: System limit for number of file watchers"

If you see this during development:

```bash
echo fs.inotify.max_user_watches=524288 | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

## Quick Reference

```bash
# Build everything
make server
cd web && npm install && npm run generate && cd ..

# Run the server
./bin/anim-server

# Access from another computer
# http://<raspberry-pi-ip>:8080
```
