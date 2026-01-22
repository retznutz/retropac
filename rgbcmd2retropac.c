/*
 * rgbcmd2retropac - Convert RGBcommander XML config to RetroPac JSON config
 * 
 * This tool converts the rgbcmdd.xml format used by RGBcommander to the
 * config.json format used by RetroPac.
 * 
 * Usage: rgbcmd2retropac <input.xml> <output.json>
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define MAX_COLOURS 256
#define MAX_CONTROLS 128
#define MAX_ROMS 512
#define MAX_EMULATORS 64
#define MAX_BUTTONS_PER_ROM 64

/* Colour definition from XML */
typedef struct {
    char name[64];
    int r, g, b;
} ColourDef;

/* Control/pin mapping */
typedef struct {
    char name[64];
    int r_pin, g_pin, b_pin;
} ControlMapping;

/* ROM button config */
typedef struct {
    char name[64];
    char colour[64];
} RomButton;

/* ROM configuration */
typedef struct {
    char id[256];
    RomButton buttons[MAX_BUTTONS_PER_ROM];
    int button_count;
} RomDef;

/* Emulator configuration - uses pointers for ROMs to reduce struct size */
typedef struct {
    char binary[128];
    char name[128];
    char active[64];
    char inactive[64];
    RomDef *roms;
    int rom_count;
    int rom_capacity;
} EmulatorDef;

/* Global storage */
static ColourDef colours[MAX_COLOURS];
static int colour_count = 0;

static ControlMapping controls[MAX_CONTROLS];
static int control_count = 0;

static EmulatorDef emulators[MAX_EMULATORS];
static int emulator_count = 0;

/* Find colour by name and return hex string */
static const char *colour_to_hex(const char *name, char *hex_buf) {
    for (int i = 0; i < colour_count; i++) {
        if (strcasecmp(colours[i].name, name) == 0) {
            sprintf(hex_buf, "#%02X%02X%02X", colours[i].r, colours[i].g, colours[i].b);
            return hex_buf;
        }
    }
    /* Default to black if not found */
    strcpy(hex_buf, "#000000");
    return hex_buf;
}

/* Parse RGB string "r,g,b" */
static void parse_rgb(const char *rgb_str, int *r, int *g, int *b) {
    *r = *g = *b = 0;
    sscanf(rgb_str, "%d,%d,%d", r, g, b);
}

/* Parse pin string "r,g,b" */
static void parse_pins(const char *pin_str, int *r_pin, int *g_pin, int *b_pin) {
    *r_pin = *g_pin = *b_pin = -1;
    sscanf(pin_str, "%d,%d,%d", r_pin, g_pin, b_pin);
}

/* Parse colours section */
static void parse_colours(xmlNode *colours_node) {
    for (xmlNode *node = colours_node->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && xmlStrcmp(node->name, (const xmlChar *)"colour") == 0) {
            xmlChar *name = xmlGetProp(node, (const xmlChar *)"name");
            xmlChar *rgb = xmlGetProp(node, (const xmlChar *)"rgb");
            
            if (name && rgb && colour_count < MAX_COLOURS) {
                strncpy(colours[colour_count].name, (const char *)name, sizeof(colours[colour_count].name) - 1);
                parse_rgb((const char *)rgb, &colours[colour_count].r, &colours[colour_count].g, &colours[colour_count].b);
                colour_count++;
            }
            
            if (name) xmlFree(name);
            if (rgb) xmlFree(rgb);
        }
    }
    printf("Parsed %d colours\n", colour_count);
}

/* Parse ledboards section */
static void parse_ledboards(xmlNode *ledboards_node) {
    for (xmlNode *board = ledboards_node->children; board; board = board->next) {
        if (board->type == XML_ELEMENT_NODE && xmlStrcmp(board->name, (const xmlChar *)"ledboard") == 0) {
            xmlChar *board_name = xmlGetProp(board, (const xmlChar *)"name");
            
            /* Only parse ULTIMATEIO boards for now */
            if (board_name && strstr((const char *)board_name, "ULTIMATEIO") != NULL) {
                printf("Parsing LED board: %s\n", board_name);
                
                for (xmlNode *ctrl = board->children; ctrl; ctrl = ctrl->next) {
                    if (ctrl->type == XML_ELEMENT_NODE && xmlStrcmp(ctrl->name, (const xmlChar *)"control") == 0) {
                        xmlChar *name = xmlGetProp(ctrl, (const xmlChar *)"name");
                        xmlChar *pin = xmlGetProp(ctrl, (const xmlChar *)"pin");
                        
                        if (name && pin && control_count < MAX_CONTROLS) {
                            strncpy(controls[control_count].name, (const char *)name, sizeof(controls[control_count].name) - 1);
                            parse_pins((const char *)pin, 
                                       &controls[control_count].r_pin, 
                                       &controls[control_count].g_pin, 
                                       &controls[control_count].b_pin);
                            control_count++;
                        }
                        
                        if (name) xmlFree(name);
                        if (pin) xmlFree(pin);
                    }
                }
            }
            
            if (board_name) xmlFree(board_name);
        }
    }
    printf("Parsed %d control mappings\n", control_count);
}

/* Parse a single ROM */
static void parse_rom(xmlNode *rom_node, EmulatorDef *emu) {
    /* Grow the roms array if needed */
    if (emu->rom_count >= emu->rom_capacity) {
        int new_capacity = emu->rom_capacity == 0 ? 16 : emu->rom_capacity * 2;
        RomDef *new_roms = realloc(emu->roms, new_capacity * sizeof(RomDef));
        if (!new_roms) {
            fprintf(stderr, "Error: Out of memory allocating ROMs\n");
            return;
        }
        emu->roms = new_roms;
        emu->rom_capacity = new_capacity;
    }
    
    RomDef *rom = &emu->roms[emu->rom_count];
    memset(rom, 0, sizeof(RomDef));
    rom->button_count = 0;
    
    xmlChar *id = xmlGetProp(rom_node, (const xmlChar *)"id");
    if (id) {
        strncpy(rom->id, (const char *)id, sizeof(rom->id) - 1);
        xmlFree(id);
    } else {
        strcpy(rom->id, "default");
    }
    
    for (xmlNode *ctrl = rom_node->children; ctrl; ctrl = ctrl->next) {
        if (ctrl->type == XML_ELEMENT_NODE && xmlStrcmp(ctrl->name, (const xmlChar *)"control") == 0) {
            xmlChar *name = xmlGetProp(ctrl, (const xmlChar *)"name");
            xmlChar *colour = xmlGetProp(ctrl, (const xmlChar *)"colour");
            
            if (name && colour && rom->button_count < MAX_BUTTONS_PER_ROM) {
                strncpy(rom->buttons[rom->button_count].name, (const char *)name, sizeof(rom->buttons[0].name) - 1);
                strncpy(rom->buttons[rom->button_count].colour, (const char *)colour, sizeof(rom->buttons[0].colour) - 1);
                rom->button_count++;
            }
            
            if (name) xmlFree(name);
            if (colour) xmlFree(colour);
        }
    }
    
    emu->rom_count++;
}

/* Parse emulators section */
static void parse_emulators(xmlNode *emulators_node) {
    for (xmlNode *emu = emulators_node->children; emu; emu = emu->next) {
        if (emu->type == XML_ELEMENT_NODE && xmlStrcmp(emu->name, (const xmlChar *)"emulator") == 0) {
            if (emulator_count >= MAX_EMULATORS) break;
            
            EmulatorDef *emulator = &emulators[emulator_count];
            memset(emulator, 0, sizeof(EmulatorDef));
            emulator->rom_count = 0;
            emulator->rom_capacity = 0;
            emulator->roms = NULL;
            
            xmlChar *binary = xmlGetProp(emu, (const xmlChar *)"binary");
            xmlChar *name = xmlGetProp(emu, (const xmlChar *)"name");
            xmlChar *active = xmlGetProp(emu, (const xmlChar *)"active");
            xmlChar *inactive = xmlGetProp(emu, (const xmlChar *)"inactive");
            
            if (binary) {
                strncpy(emulator->binary, (const char *)binary, sizeof(emulator->binary) - 1);
                xmlFree(binary);
            }
            if (name) {
                strncpy(emulator->name, (const char *)name, sizeof(emulator->name) - 1);
                xmlFree(name);
            }
            if (active) {
                strncpy(emulator->active, (const char *)active, sizeof(emulator->active) - 1);
                xmlFree(active);
            }
            if (inactive) {
                strncpy(emulator->inactive, (const char *)inactive, sizeof(emulator->inactive) - 1);
                xmlFree(inactive);
            }
            
            /* Parse ROMs */
            for (xmlNode *rom = emu->children; rom; rom = rom->next) {
                if (rom->type == XML_ELEMENT_NODE && xmlStrcmp(rom->name, (const xmlChar *)"rom") == 0) {
                    parse_rom(rom, emulator);
                }
            }
            
            if (emulator->rom_count > 0) {
                emulator_count++;
            } else {
                /* Free any allocated memory if no ROMs were found */
                free(emulator->roms);
                emulator->roms = NULL;
            }
        }
    }
    printf("Parsed %d emulators\n", emulator_count);
}

/* Write JSON output */
static int write_json(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error: Could not open output file '%s'\n", filename);
        return -1;
    }
    
    char hex_buf[16];
    
    fprintf(fp, "{\n");
    
    /* Write ipac_controllers section */
    fprintf(fp, "  \"ipac_controllers\": [\n");
    fprintf(fp, "    {\n");
    fprintf(fp, "      \"device\": \"ipac-ultimate\",\n");
    fprintf(fp, "      \"vendor_id\": \"0xd209\",\n");
    fprintf(fp, "      \"product_id\": \"0x0410\",\n");
    fprintf(fp, "      \"pin_mappings\": {\n");
    
    for (int i = 0; i < control_count; i++) {
        fprintf(fp, "        \"%s\": {\"r_pin\": %d, \"g_pin\": %d, \"b_pin\": %d}%s\n",
                controls[i].name,
                controls[i].r_pin,
                controls[i].g_pin,
                controls[i].b_pin,
                (i < control_count - 1) ? "," : "");
    }
    
    fprintf(fp, "      }\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "  ],\n");
    
    /* Write default section (use first emulator's default rom if available) */
    fprintf(fp, "  \"default\": {\n");
    int default_written = 0;
    for (int e = 0; e < emulator_count && !default_written; e++) {
        for (int r = 0; r < emulators[e].rom_count; r++) {
            if (strcmp(emulators[e].roms[r].id, "default") == 0) {
                RomDef *rom = &emulators[e].roms[r];
                for (int b = 0; b < rom->button_count; b++) {
                    fprintf(fp, "    \"%s\": \"%s\"%s\n",
                            rom->buttons[b].name,
                            colour_to_hex(rom->buttons[b].colour, hex_buf),
                            (b < rom->button_count - 1) ? "," : "");
                }
                default_written = 1;
                break;
            }
        }
    }
    fprintf(fp, "  },\n");
    
    /* Write emulators section */
    fprintf(fp, "  \"emulators\": {\n");
    
    for (int e = 0; e < emulator_count; e++) {
        EmulatorDef *emu = &emulators[e];
        
        fprintf(fp, "    \"%s\": {\n", emu->binary);
        fprintf(fp, "      \"roms\": {\n");
        
        for (int r = 0; r < emu->rom_count; r++) {
            RomDef *rom = &emu->roms[r];
            
            fprintf(fp, "        \"%s\": {\n", rom->id);
            
            for (int b = 0; b < rom->button_count; b++) {
                fprintf(fp, "          \"%s\": \"%s\"%s\n",
                        rom->buttons[b].name,
                        colour_to_hex(rom->buttons[b].colour, hex_buf),
                        (b < rom->button_count - 1) ? "," : "");
            }
            
            fprintf(fp, "        }%s\n", (r < emu->rom_count - 1) ? "," : "");
        }
        
        fprintf(fp, "      }\n");
        fprintf(fp, "    }%s\n", (e < emulator_count - 1) ? "," : "");
    }
    
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    return 0;
}

/* Parse the XML file */
static int parse_xml(const char *filename) {
    xmlDoc *doc = xmlReadFile(filename, NULL, XML_PARSE_NOWARNING | XML_PARSE_NOERROR);
    if (!doc) {
        fprintf(stderr, "Error: Could not parse XML file '%s'\n", filename);
        return -1;
    }
    
    xmlNode *root = xmlDocGetRootElement(doc);
    if (!root) {
        fprintf(stderr, "Error: Empty XML document\n");
        xmlFreeDoc(doc);
        return -1;
    }
    
    /* Walk the XML tree */
    for (xmlNode *node = root->children; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) continue;
        
        if (xmlStrcmp(node->name, (const xmlChar *)"colours") == 0) {
            parse_colours(node);
        }
        else if (xmlStrcmp(node->name, (const xmlChar *)"hardware") == 0) {
            /* Find ledboards within hardware */
            for (xmlNode *hw = node->children; hw; hw = hw->next) {
                if (hw->type == XML_ELEMENT_NODE && xmlStrcmp(hw->name, (const xmlChar *)"ledboards") == 0) {
                    parse_ledboards(hw);
                }
            }
        }
        else if (xmlStrcmp(node->name, (const xmlChar *)"emulators") == 0) {
            parse_emulators(node);
        }
    }
    
    xmlFreeDoc(doc);
    return 0;
}

int main(int argc, char *argv[]) {
    printf("rgbcmd2retropac - RGBcommander to RetroPac Config Converter\n");
    printf("============================================================\n\n");
    
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.xml> <output.json>\n", argv[0]);
        fprintf(stderr, "\nConverts RGBcommander XML config to RetroPac JSON format.\n");
        fprintf(stderr, "\nExample:\n");
        fprintf(stderr, "  %s rgbcmdd.xml config.json\n", argv[0]);
        return 1;
    }
    
    const char *input_file = argv[1];
    const char *output_file = argv[2];
    
    printf("Input:  %s\n", input_file);
    printf("Output: %s\n\n", output_file);
    
    /* Initialize libxml */
    LIBXML_TEST_VERSION
    
    /* Parse the XML */
    if (parse_xml(input_file) < 0) {
        return 1;
    }
    
    /* Write JSON output */
    if (write_json(output_file) < 0) {
        return 1;
    }
    
    printf("\nConversion complete!\n");
    printf("  - %d colours\n", colour_count);
    printf("  - %d pin mappings\n", control_count);
    printf("  - %d emulators\n", emulator_count);
    
    int total_roms = 0;
    for (int i = 0; i < emulator_count; i++) {
        total_roms += emulators[i].rom_count;
    }
    printf("  - %d ROM configurations\n", total_roms);
    
    /* Cleanup allocated memory */
    for (int i = 0; i < emulator_count; i++) {
        free(emulators[i].roms);
    }
    
    /* Cleanup libxml */
    xmlCleanupParser();
    
    return 0;
}
