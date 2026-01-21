# Makefile for RetroPac - Ultimarc i-pac LED Controller

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -ljson-c -lusb-1.0
TARGET = retropac
INSTALL_DIR = /usr/local/bin
CONFIG_DIR = /home/pi/RetroPie/configs/retropac

SOURCES = main.c config.c ipac.c
HEADERS = retropac.h
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	@echo "Installing $(TARGET) to $(INSTALL_DIR)..."
	install -D -m 755 $(TARGET) $(INSTALL_DIR)/$(TARGET)
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
	@echo "   $(INSTALL_DIR)/$(TARGET) \"\$$1\" \"\$$3\""
	@echo ""

uninstall:
	@echo "Removing $(TARGET) from $(INSTALL_DIR)..."
	rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "Note: Config files in $(CONFIG_DIR) were not removed"
	@echo "Run 'sudo rm -rf $(CONFIG_DIR)' to remove configuration"

# Help target
help:
	@echo "RetroPac Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the program (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to $(INSTALL_DIR)"
	@echo "  uninstall - Remove from $(INSTALL_DIR)"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - libjson-c-dev"
	@echo "  - libusb-1.0-0-dev"
	@echo ""
	@echo "Install dependencies with:"
	@echo "  sudo apt-get install libjson-c-dev libusb-1.0-0-dev"
