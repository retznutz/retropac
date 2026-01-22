# Makefile for RetroPac - Ultimarc i-pac LED Controller

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -Iinclude
LDFLAGS = -ljson-c -lusb-1.0
LDFLAGS_XML = -lxml2

# Directories
SRCDIR = src
INCDIR = include
TOOLSDIR = tools
OBJDIR = obj
BINDIR = bin

TARGET = $(BINDIR)/retropac
CONVERTER = $(BINDIR)/rgbcmd2retropac
INSTALL_DIR = /usr/local/bin
CONFIG_DIR = /home/pi/RetroPie/configs/retropac

SOURCES = $(wildcard $(SRCDIR)/*.c)
HEADERS = $(wildcard $(INCDIR)/*.h)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Find libxml2 include path
XML2_CFLAGS = $(shell pkg-config --cflags libxml-2.0 2>/dev/null || echo "-I/usr/include/libxml2")
XML2_LDFLAGS = $(shell pkg-config --libs libxml-2.0 2>/dev/null || echo "-lxml2")

.PHONY: all clean install uninstall converter

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR) $(OBJDIR):
	mkdir -p $@

# Build the RGBcommander to RetroPac converter
converter: $(CONVERTER)

$(CONVERTER): $(TOOLSDIR)/rgbcmd2retropac.c | $(BINDIR)
	$(CC) $(CFLAGS) $(XML2_CFLAGS) -o $@ $< $(XML2_LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

install: $(TARGET)
	@echo "Installing retropac to $(INSTALL_DIR)..."
	install -D -m 755 $(TARGET) $(INSTALL_DIR)/retropac
	@echo "Creating config directory at $(CONFIG_DIR)..."
	mkdir -p $(CONFIG_DIR)
	@if [ ! -f $(CONFIG_DIR)/config.json ]; then \
		echo "Installing example configuration..."; \
		install -D -m 644 config.example.json $(CONFIG_DIR)/config.json; \
	else \
		echo "Config file already exists, not overwriting"; \
	fi
	@echo ""
	@echo "Installation complete!"
	@echo ""
	@echo "Next steps:"
	@echo "1. Edit $(CONFIG_DIR)/config.json with your configuration"
	@echo "2. Add udev rules if needed (see README.md)"
	@echo "3. Add to RetroPie runcommand-onstart.sh:"
	@echo "   $(INSTALL_DIR)/retropac \"\$$1\" \"\$$3\""
	@echo ""

uninstall:
	@echo "Removing retropac from $(INSTALL_DIR)..."
	rm -f $(INSTALL_DIR)/retropac
	@echo "Note: Config files in $(CONFIG_DIR) were not removed"
	@echo "Run 'sudo rm -rf $(CONFIG_DIR)' to remove configuration"

# Help target
help:
	@echo "RetroPac Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the main program (default)"
	@echo "  converter - Build the RGBcommander to RetroPac converter"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to $(INSTALL_DIR)"
	@echo "  uninstall - Remove from $(INSTALL_DIR)"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - libjson-c-dev"
	@echo "  - libusb-1.0-0-dev"
	@echo "  - libxml2-dev (for converter only)"
	@echo ""
	@echo "Install dependencies with:"
	@echo "  sudo apt-get install libjson-c-dev libusb-1.0-0-dev libxml2-dev"
