#!/bin/bash
# validate.sh - Validate code structure and syntax

set -e

echo "RetroPac Code Validation"
echo "========================"
echo ""

# Check that all required files exist
echo "Checking required files..."
FILES="retropac.h main.c config.c ipac.c Makefile config.example.json README.md"
for file in $FILES; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ $file - MISSING"
        exit 1
    fi
done
echo ""

# Check for common syntax errors in C files
echo "Checking C syntax..."
for file in main.c config.c ipac.c; do
    echo "  Checking $file..."
    
    # Check for unclosed braces (basic check)
    open_braces=$(grep -o '{' "$file" | wc -l)
    close_braces=$(grep -o '}' "$file" | wc -l)
    if [ "$open_braces" -ne "$close_braces" ]; then
        echo "    ✗ Mismatched braces: $open_braces open, $close_braces close"
        exit 1
    fi
    
    # Check for basic includes
    if grep -q "#include.*retropac.h" "$file"; then
        echo "    ✓ Includes retropac.h"
    fi
done
echo ""

# Validate JSON configuration
echo "Validating JSON configuration..."
if command -v python3 &> /dev/null; then
    python3 -c "import json; json.load(open('config.example.json'))" && \
        echo "  ✓ config.example.json is valid JSON" || \
        echo "  ✗ config.example.json has JSON errors"
else
    echo "  ⚠ Python3 not available, skipping JSON validation"
fi
echo ""

# Check header file structure
echo "Checking header file..."
if grep -q "#ifndef RETROPAC_H" retropac.h && \
   grep -q "#define RETROPAC_H" retropac.h && \
   grep -q "#endif" retropac.h; then
    echo "  ✓ Header guards present"
else
    echo "  ✗ Header guards missing or incorrect"
    exit 1
fi
echo ""

# Count button enums
echo "Checking button enumerations..."
enum_count=$(grep -c "P[1-4]_" retropac.h || echo "0")
echo "  ✓ Found $enum_count button-related lines"
echo ""

# Check for all required functions
echo "Checking function declarations..."
FUNCTIONS="load_config free_config button_name_to_enum button_enum_to_name ipac_init ipac_set_led ipac_set_all_leds ipac_close extract_rom_name"
for func in $FUNCTIONS; do
    if grep -q "$func" retropac.h; then
        echo "  ✓ $func declared"
    else
        echo "  ✗ $func not found"
        exit 1
    fi
done
echo ""

# Check Makefile targets
echo "Checking Makefile..."
if grep -q "^all:" Makefile && \
   grep -q "^clean:" Makefile && \
   grep -q "^install:" Makefile; then
    echo "  ✓ Required targets present (all, clean, install)"
else
    echo "  ✗ Missing required Makefile targets"
    exit 1
fi
echo ""

# Check documentation
echo "Checking documentation..."
DOCS="README.md HARDWARE.md INTEGRATION.md TESTING.md"
for doc in $DOCS; do
    if [ -f "$doc" ]; then
        lines=$(wc -l < "$doc")
        echo "  ✓ $doc ($lines lines)"
    else
        echo "  ✗ $doc - MISSING"
    fi
done
echo ""

echo "=========================================="
echo "✓ All validation checks passed!"
echo "=========================================="
echo ""
echo "Note: This validates code structure only."
echo "To fully test, install dependencies and compile:"
echo "  sudo apt-get install libjson-c-dev libusb-1.0-0-dev"
echo "  make"
echo ""
