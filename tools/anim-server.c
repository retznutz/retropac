/*
 * RetroPac Animation Editor Server
 * 
 * A simple HTTP server that serves the animation editor web application
 * and provides REST API endpoints for managing animation files.
 * 
 * Usage: anim-server [--port <port>] [--animations-dir <path>] [--web-dir <path>]
 * 
 * Default port: 8080
 * Default animations dir: ./animations
 * Default web dir: ./web/dist
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <microhttpd.h>
#include <json-c/json.h>

#define DEFAULT_PORT 8080
#define DEFAULT_ANIM_DIR "./animations"
#define DEFAULT_WEB_DIR "./web/dist"
#define DEFAULT_CONFIG_PATH "/etc/retropac/config.json"
#define DEFAULT_CONFIG_DIR  "/etc/retropac"
#define CONFIG_EXAMPLE_PATH "/usr/share/retropac/config.example.json"
#define DEFAULT_PID_FILE "/tmp/anim-server.pid"
#define MAX_POST_SIZE (1024 * 1024)  /* 1MB max request body */

/* Global configuration */
static int server_port = DEFAULT_PORT;
static char *animations_dir = NULL;
static char *web_dir = NULL;
static char *config_path = NULL;
static char *pid_file = NULL;
static volatile sig_atomic_t running = 1;

/* Valid button names */
static const char *valid_buttons[] = {
    "P1_COIN", "P2_COIN", "P3_COIN", "P4_COIN",
    "P1_START", "P2_START", "P3_START", "P4_START",
    "P1_BUTTON1", "P1_BUTTON2", "P1_BUTTON3", "P1_BUTTON4", "P1_BUTTON5", "P1_BUTTON6",
    "P2_BUTTON1", "P2_BUTTON2", "P2_BUTTON3", "P2_BUTTON4", "P2_BUTTON5", "P2_BUTTON6",
    "P3_BUTTON1", "P3_BUTTON2", "P3_BUTTON3", "P3_BUTTON4", "P3_BUTTON5", "P3_BUTTON6",
    "P4_BUTTON1", "P4_BUTTON2", "P4_BUTTON3", "P4_BUTTON4", "P4_BUTTON5", "P4_BUTTON6",
    "P1_JOYSTICK", "P2_JOYSTICK", "P3_JOYSTICK", "P4_JOYSTICK",
    "P1_TRACKBALL", "P2_TRACKBALL", "P3_TRACKBALL", "P4_TRACKBALL",
    "P1_LIGHTGUN", "P2_LIGHTGUN",
    "P1_DIAL", "P2_DIAL",
    "P1_PADDLE", "P2_PADDLE",
    "P1_STICK", "P2_STICK",
    NULL
};

/* Write PID to file */
static int write_pid_file(const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "Error: Cannot create PID file '%s': %s\n", path, strerror(errno));
        return -1;
    }
    
    char pid_str[32];
    int len = snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    if (write(fd, pid_str, len) != len) {
        fprintf(stderr, "Error: Cannot write to PID file: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

/* Read PID from file */
static pid_t read_pid_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    
    pid_t pid = -1;
    if (fscanf(fp, "%d", &pid) != 1) {
        pid = -1;
    }
    fclose(fp);
    return pid;
}

/* Remove PID file */
static void remove_pid_file(void) {
    if (pid_file) {
        unlink(pid_file);
    }
}

/* Daemonize the process */
static int daemonize(void) {
    pid_t pid = fork();
    
    if (pid < 0) {
        fprintf(stderr, "Error: Fork failed: %s\n", strerror(errno));
        return -1;
    }
    
    if (pid > 0) {
        /* Parent process - exit successfully */
        printf("Started anim-server daemon (PID: %d)\n", pid);
        exit(0);
    }
    
    /* Child process continues */
    
    /* Create new session */
    if (setsid() < 0) {
        fprintf(stderr, "Error: setsid failed: %s\n", strerror(errno));
        return -1;
    }
    
    /* Fork again to prevent acquiring a controlling terminal */
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Error: Second fork failed: %s\n", strerror(errno));
        return -1;
    }
    
    if (pid > 0) {
        /* First child exits */
        exit(0);
    }
    
    /* Grandchild continues as the daemon */
    
    /* Change working directory to root to avoid blocking unmounts */
    if (chdir("/") < 0) {
        /* Non-fatal, just warn */
        fprintf(stderr, "Warning: Could not change directory to /\n");
    }
    
    /* Set file permissions mask */
    umask(0);
    
    /* Redirect standard file descriptors to /dev/null */
    int null_fd = open("/dev/null", O_RDWR);
    if (null_fd >= 0) {
        dup2(null_fd, STDIN_FILENO);
        dup2(null_fd, STDOUT_FILENO);
        dup2(null_fd, STDERR_FILENO);
        if (null_fd > STDERR_FILENO) {
            close(null_fd);
        }
    }
    
    return 0;
}

/* Stop a running daemon */
static int stop_daemon(const char *pidfile) {
    pid_t pid = read_pid_file(pidfile);
    
    if (pid <= 0) {
        fprintf(stderr, "Error: Cannot read PID file '%s' or daemon not running\n", pidfile);
        return 1;
    }
    
    /* Check if process exists */
    if (kill(pid, 0) < 0) {
        if (errno == ESRCH) {
            fprintf(stderr, "Daemon not running (stale PID file)\n");
            unlink(pidfile);
            return 1;
        }
        fprintf(stderr, "Error: Cannot check process %d: %s\n", pid, strerror(errno));
        return 1;
    }
    
    printf("Stopping anim-server daemon (PID: %d)...\n", pid);
    
    /* Send SIGTERM */
    if (kill(pid, SIGTERM) < 0) {
        fprintf(stderr, "Error: Cannot stop daemon: %s\n", strerror(errno));
        return 1;
    }
    
    /* Wait for process to exit (up to 5 seconds) */
    for (int i = 0; i < 50; i++) {
        usleep(100000); /* 100ms */
        if (kill(pid, 0) < 0 && errno == ESRCH) {
            printf("Daemon stopped successfully\n");
            unlink(pidfile);
            return 0;
        }
    }
    
    fprintf(stderr, "Warning: Daemon did not stop gracefully, sending SIGKILL\n");
    kill(pid, SIGKILL);
    usleep(100000);
    unlink(pidfile);
    printf("Daemon killed\n");
    
    return 0;
}

/* Signal handler for graceful shutdown */
static void signal_handler(int signum) {
    (void)signum;
    running = 0;
}

/* Setup signal handlers */
static void setup_signals(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

/* Get the first non-loopback IPv4 address */
static const char *get_local_ip(void) {
    static char ip_buffer[INET_ADDRSTRLEN] = "localhost";
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        return ip_buffer;
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        /* Only interested in IPv4 */
        if (ifa->ifa_addr->sa_family != AF_INET) continue;
        
        /* Skip loopback */
        if (strcmp(ifa->ifa_name, "lo") == 0) continue;
        
        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &addr->sin_addr, ip_buffer, INET_ADDRSTRLEN);
        break;
    }
    
    freeifaddrs(ifaddr);
    return ip_buffer;
}
/* Get MIME type for file extension */
static const char *get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    if (strcmp(ext, ".woff") == 0) return "font/woff";
    if (strcmp(ext, ".woff2") == 0) return "font/woff2";
    if (strcmp(ext, ".ttf") == 0) return "font/ttf";
    
    return "application/octet-stream";
}

/* Read entire file into memory */
static char *read_file(const char *path, size_t *size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(fp);
        return NULL;
    }
    
    char *data = malloc(file_size + 1);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    
    size_t read_size = fread(data, 1, file_size, fp);
    fclose(fp);
    
    data[read_size] = '\0';
    if (size) *size = read_size;
    
    return data;
}

/* Write data to file */
static int write_file(const char *path, const char *data, size_t size) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return -1;
    
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    
    return (written == size) ? 0 : -1;
}

/* Check if path is safe (no directory traversal) */
static int is_safe_path(const char *path) {
    if (!path) return 0;
    if (strstr(path, "..") != NULL) return 0;
    if (path[0] == '/') return 0;
    return 1;
}

/* Create JSON error response */
static struct MHD_Response *json_error_response(const char *message) {
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "error", json_object_new_string(message));
    
    const char *json_str = json_object_to_json_string(obj);
    char *response_data = strdup(json_str);
    json_object_put(obj);
    
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_data), response_data, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    
    return response;
}

/* Create JSON success response */
static struct MHD_Response *json_success_response(struct json_object *data) {
    const char *json_str = json_object_to_json_string_ext(data, JSON_C_TO_STRING_PRETTY);
    char *response_data = strdup(json_str);
    
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_data), response_data, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    
    return response;
}

/* Handle GET /api/buttons - list valid button names */
static struct MHD_Response *handle_get_buttons(void) {
    struct json_object *arr = json_object_new_array();
    
    for (int i = 0; valid_buttons[i] != NULL; i++) {
        json_object_array_add(arr, json_object_new_string(valid_buttons[i]));
    }
    
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "buttons", arr);
    
    struct MHD_Response *response = json_success_response(obj);
    json_object_put(obj);
    
    return response;
}

/* Handle GET /api/animations - list all animations with metadata */
static struct MHD_Response *handle_list_animations(void) {
    struct json_object *arr = json_object_new_array();
    DIR *dir = opendir(animations_dir);
    
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".json") == 0) {
                /* Remove .json extension for the name */
                char *filename = strdup(entry->d_name);
                char *dot = strrchr(filename, '.');
                if (dot) *dot = '\0';
                
                /* Try to read the friendly name from the file */
                size_t path_len = strlen(animations_dir) + strlen(entry->d_name) + 2;
                char *path = malloc(path_len);
                snprintf(path, path_len, "%s/%s", animations_dir, entry->d_name);
                
                size_t size;
                char *data = read_file(path, &size);
                free(path);
                
                const char *friendly_name = filename;
                struct json_object *anim_obj = NULL;
                
                if (data) {
                    anim_obj = json_tokener_parse(data);
                    free(data);
                    
                    if (anim_obj) {
                        struct json_object *name_obj;
                        if (json_object_object_get_ex(anim_obj, "name", &name_obj)) {
                            friendly_name = json_object_get_string(name_obj);
                        }
                    }
                }
                
                /* Create animation entry with both names */
                struct json_object *entry_obj = json_object_new_object();
                json_object_object_add(entry_obj, "filename", json_object_new_string(filename));
                json_object_object_add(entry_obj, "name", json_object_new_string(friendly_name));
                json_object_array_add(arr, entry_obj);
                
                if (anim_obj) json_object_put(anim_obj);
                free(filename);
            }
        }
        closedir(dir);
    }
    
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "animations", arr);
    
    struct MHD_Response *response = json_success_response(obj);
    json_object_put(obj);
    
    return response;
}

/* Handle GET /api/animations/:name - get specific animation */
static struct MHD_Response *handle_get_animation(const char *name, int *status_code) {
    if (!is_safe_path(name)) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid animation name");
    }
    
    /* Build file path */
    size_t path_len = strlen(animations_dir) + strlen(name) + 10;
    char *path = malloc(path_len);
    snprintf(path, path_len, "%s/%s.json", animations_dir, name);
    
    /* Read animation file */
    size_t size;
    char *data = read_file(path, &size);
    free(path);
    
    if (!data) {
        *status_code = MHD_HTTP_NOT_FOUND;
        return json_error_response("Animation not found");
    }
    
    /* Parse and re-serialize to validate JSON */
    struct json_object *anim = json_tokener_parse(data);
    free(data);
    
    if (!anim) {
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Invalid animation JSON");
    }
    
    struct MHD_Response *response = json_success_response(anim);
    json_object_put(anim);
    *status_code = MHD_HTTP_OK;
    
    return response;
}

/* Handle PUT /api/animations/:name - save animation */
static struct MHD_Response *handle_put_animation(const char *name, const char *body, int *status_code) {
    if (!is_safe_path(name)) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid animation name");
    }
    
    if (!body || strlen(body) == 0) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Request body required");
    }
    
    /* Validate JSON */
    struct json_object *anim = json_tokener_parse(body);
    if (!anim) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid JSON");
    }
    
    /* Validate required fields */
    struct json_object *frames_obj;
    if (!json_object_object_get_ex(anim, "frames", &frames_obj) ||
        json_object_get_type(frames_obj) != json_type_array) {
        json_object_put(anim);
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Missing or invalid 'frames' array");
    }
    
    /* Build file path */
    size_t path_len = strlen(animations_dir) + strlen(name) + 10;
    char *path = malloc(path_len);
    snprintf(path, path_len, "%s/%s.json", animations_dir, name);
    
    /* Pretty print JSON for saving */
    const char *json_str = json_object_to_json_string_ext(anim, JSON_C_TO_STRING_PRETTY);
    
    if (write_file(path, json_str, strlen(json_str)) < 0) {
        free(path);
        json_object_put(anim);
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Failed to write file");
    }
    
    free(path);
    json_object_put(anim);
    
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "name", json_object_new_string(name));
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    *status_code = MHD_HTTP_OK;
    
    return response;
}

/* Handle POST /api/animations/:name/play - play animation on hardware */
static struct MHD_Response *handle_play_animation(const char *name, int *status_code) {
    if (!is_safe_path(name)) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid animation name");
    }
    
    /* Verify animation exists */
    size_t path_len = strlen(animations_dir) + strlen(name) + 10;
    char *path = malloc(path_len);
    snprintf(path, path_len, "%s/%s.json", animations_dir, name);
    
    struct stat st;
    if (stat(path, &st) < 0) {
        free(path);
        *status_code = MHD_HTTP_NOT_FOUND;
        return json_error_response("Animation not found");
    }
    free(path);
    
    /* Build command to run retropac */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
             "/usr/local/bin/retropac --quiet --custom %s --daemon default default default 2>&1",
             name);
    
    /* Execute command */
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Failed to execute retropac");
    }
    
    /* Read any output (for debugging) */
    char output[1024] = "";
    fread(output, 1, sizeof(output) - 1, fp);
    pclose(fp);
    
    /* Always report success - if animation exists and popen worked, it started */
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "animation", json_object_new_string(name));
    json_object_object_add(result, "message", json_object_new_string("Animation started on hardware"));
    *status_code = MHD_HTTP_OK;
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    
    return response;
}

/* Update a shell script to use a specific attract mode animation */
static int update_attract_script(const char *script_path, const char *anim_name, int add_ampersand) {
    FILE *fp = fopen(script_path, "r");
    if (!fp) {
        return -1;  /* File doesn't exist */
    }
    
    /* Read entire file */
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    if (!content) {
        fclose(fp);
        return -2;
    }
    
    size_t read_size = fread(content, 1, size, fp);
    content[read_size] = '\0';
    fclose(fp);
    
    /* Build new retropac command line */
    char new_cmd[512];
    if (add_ampersand) {
        snprintf(new_cmd, sizeof(new_cmd), 
                 "/usr/local/bin/retropac --quiet --custom %s --daemon default default default &",
                 anim_name);
    } else {
        snprintf(new_cmd, sizeof(new_cmd), 
                 "/usr/local/bin/retropac --quiet --custom %s --daemon default default default",
                 anim_name);
    }
    
    /* Find existing retropac line - try multiple patterns */
    char *pos = strstr(content, "/usr/local/bin/retropac");
    if (!pos) {
        pos = strstr(content, "retropac --");
    }
    
    /* Allocate space for new content */
    char *new_content = malloc(size + strlen(new_cmd) + 256);
    if (!new_content) {
        free(content);
        return -2;
    }
    
    if (pos) {
        /* Find end of line */
        char *eol = strchr(pos, '\n');
        if (!eol) eol = pos + strlen(pos);
        
        /* Build new content */
        size_t prefix_len = pos - content;
        strncpy(new_content, content, prefix_len);
        new_content[prefix_len] = '\0';
        strcat(new_content, new_cmd);
        strcat(new_content, eol);
    } else {
        /* No existing retropac line - file exists but no command found */
        free(content);
        free(new_content);
        return -3;
    }
    
    /* Write back */
    fp = fopen(script_path, "w");
    if (!fp) {
        free(content);
        free(new_content);
        return -4;
    }
    
    fputs(new_content, fp);
    fclose(fp);
    
    free(content);
    free(new_content);
    return 0;
}

/* Handle POST /api/animations/:name/set-attract - set as attract mode */
static struct MHD_Response *handle_set_attract_mode(const char *name, int *status_code) {
    if (!is_safe_path(name)) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid animation name");
    }
    
    /* Verify animation exists */
    size_t path_len = strlen(animations_dir) + strlen(name) + 10;
    char *path = malloc(path_len);
    snprintf(path, path_len, "%s/%s.json", animations_dir, name);
    
    struct stat st;
    if (stat(path, &st) < 0) {
        free(path);
        *status_code = MHD_HTTP_NOT_FOUND;
        return json_error_response("Animation not found");
    }
    free(path);
    
    /* Update scripts */
    const char *autostart_path = "/opt/retropie/configs/all/autostart.sh";
    const char *runcommand_path = "/opt/retropie/configs/all/runcommand-onend.sh";
    
    int autostart_result = update_attract_script(autostart_path, name, 1);  /* with & */
    int runcommand_result = update_attract_script(runcommand_path, name, 0); /* without & */
    
    struct json_object *result = json_object_new_object();
    
    if (autostart_result == 0 || runcommand_result == 0) {
        /* At least one script updated - now restart with the new animation */
        char cmd[512];
        snprintf(cmd, sizeof(cmd), 
                 "/usr/local/bin/retropac --quiet --custom %s --daemon default default default 2>&1",
                 name);
        
        /* Execute command to start the new attract animation immediately */
        FILE *fp = popen(cmd, "r");
        if (fp) {
            /* Read and discard output */
            char buf[256];
            while (fgets(buf, sizeof(buf), fp) != NULL) {}
            pclose(fp);
        }
        
        json_object_object_add(result, "success", json_object_new_boolean(1));
        json_object_object_add(result, "animation", json_object_new_string(name));
        
        char msg[256];
        snprintf(msg, sizeof(msg), "Updated attract mode to '%s' and started playback", name);
        if (autostart_result != 0) {
            strcat(msg, " (autostart.sh not found or no retropac line)");
        }
        if (runcommand_result != 0) {
            strcat(msg, " (runcommand-onend.sh not found or no retropac line)");
        }
        json_object_object_add(result, "message", json_object_new_string(msg));
        *status_code = MHD_HTTP_OK;
    } else {
        json_object_object_add(result, "success", json_object_new_boolean(0));
        json_object_object_add(result, "error", 
            json_object_new_string("Could not update scripts. Make sure retropac lines exist in autostart.sh and runcommand-onend.sh"));
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
    }
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    
    return response;
}

/* Handle POST /api/animations/stop - stop any running animation */
static struct MHD_Response *handle_stop_animation(int *status_code) {
    /* Build command to run retropac with default (kills daemon, sets default LEDs) */
    char cmd[256];
    snprintf(cmd, sizeof(cmd), 
             "/usr/local/bin/retropac --quiet default default default 2>&1");
    
    /* Execute command */
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Failed to execute retropac");
    }
    
    char output[1024] = "";
    fread(output, 1, sizeof(output) - 1, fp);
    pclose(fp);
    
    /* Always report success - if popen worked, the command ran */
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "message", json_object_new_string("Animation stopped"));
    *status_code = MHD_HTTP_OK;
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    
    return response;
}

/* Handle POST /api/animations/:name/duplicate - duplicate animation with timestamp suffix */
static struct MHD_Response *handle_duplicate_animation(const char *name, int *status_code) {
    if (!is_safe_path(name)) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid animation name");
    }
    
    /* Build source file path */
    size_t path_len = strlen(animations_dir) + strlen(name) + 10;
    char *src_path = malloc(path_len);
    snprintf(src_path, path_len, "%s/%s.json", animations_dir, name);
    
    /* Check source exists */
    struct stat st;
    if (stat(src_path, &st) < 0) {
        free(src_path);
        *status_code = MHD_HTTP_NOT_FOUND;
        return json_error_response("Animation not found");
    }
    
    /* Read source file */
    size_t content_size;
    char *content = read_file(src_path, &content_size);
    free(src_path);
    
    if (!content) {
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Failed to read animation file");
    }
    
    /* Generate new name with timestamp suffix (YYYYMMDD_HHMMSS) */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    
    char *new_name = malloc(strlen(name) + 20);
    sprintf(new_name, "%s_%s", name, timestamp);
    
    /* Parse JSON to update the friendly name */
    struct json_object *anim = json_tokener_parse(content);
    if (anim) {
        struct json_object *name_obj;
        if (json_object_object_get_ex(anim, "name", &name_obj)) {
            const char *old_friendly = json_object_get_string(name_obj);
            char *new_friendly = malloc(strlen(old_friendly) + 10);
            sprintf(new_friendly, "%s (copy)", old_friendly);
            json_object_object_add(anim, "name", json_object_new_string(new_friendly));
            free(new_friendly);
        }
        
        /* Write to new file */
        const char *json_str = json_object_to_json_string_ext(anim, JSON_C_TO_STRING_PRETTY);
        
        size_t new_path_len = strlen(animations_dir) + strlen(new_name) + 10;
        char *new_path = malloc(new_path_len);
        snprintf(new_path, new_path_len, "%s/%s.json", animations_dir, new_name);
        
        int write_result = write_file(new_path, json_str, strlen(json_str));
        free(new_path);
        json_object_put(anim);
        free(content);
        
        if (write_result < 0) {
            free(new_name);
            *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
            return json_error_response("Failed to write duplicate file");
        }
    } else {
        free(content);
        free(new_name);
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Failed to parse animation JSON");
    }
    
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "original", json_object_new_string(name));
    json_object_object_add(result, "duplicate", json_object_new_string(new_name));
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    free(new_name);
    *status_code = MHD_HTTP_OK;
    
    return response;
}

/* Handle POST /api/animations/:name/rename - rename animation file */
static struct MHD_Response *handle_rename_animation(const char *old_name, const char *body, int *status_code) {
    if (!is_safe_path(old_name)) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid animation name");
    }
    
    if (!body || strlen(body) == 0) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Request body required");
    }
    
    /* Parse request body to get new name */
    struct json_object *req = json_tokener_parse(body);
    if (!req) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid JSON");
    }
    
    struct json_object *new_name_obj;
    if (!json_object_object_get_ex(req, "newFilename", &new_name_obj)) {
        json_object_put(req);
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Missing 'newFilename' field");
    }
    
    const char *new_name = json_object_get_string(new_name_obj);
    if (!new_name || !is_safe_path(new_name)) {
        json_object_put(req);
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid new filename");
    }
    
    /* Build file paths */
    size_t old_path_len = strlen(animations_dir) + strlen(old_name) + 10;
    char *old_path = malloc(old_path_len);
    snprintf(old_path, old_path_len, "%s/%s.json", animations_dir, old_name);
    
    size_t new_path_len = strlen(animations_dir) + strlen(new_name) + 10;
    char *new_path = malloc(new_path_len);
    snprintf(new_path, new_path_len, "%s/%s.json", animations_dir, new_name);
    
    /* Check source exists */
    struct stat st;
    if (stat(old_path, &st) < 0) {
        free(old_path);
        free(new_path);
        json_object_put(req);
        *status_code = MHD_HTTP_NOT_FOUND;
        return json_error_response("Animation not found");
    }
    
    /* Check destination doesn't exist (unless same name) */
    if (strcmp(old_name, new_name) != 0 && stat(new_path, &st) == 0) {
        free(old_path);
        free(new_path);
        json_object_put(req);
        *status_code = MHD_HTTP_CONFLICT;
        return json_error_response("An animation with that filename already exists");
    }
    
    /* Rename file */
    if (strcmp(old_name, new_name) != 0) {
        if (rename(old_path, new_path) < 0) {
            free(old_path);
            free(new_path);
            json_object_put(req);
            *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
            return json_error_response("Failed to rename file");
        }
    }
    
    free(old_path);
    free(new_path);
    json_object_put(req);
    
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "oldFilename", json_object_new_string(old_name));
    json_object_object_add(result, "newFilename", json_object_new_string(new_name));
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    *status_code = MHD_HTTP_OK;
    
    return response;
}

/* Handle DELETE /api/animations/:name - delete animation */
static struct MHD_Response *handle_delete_animation(const char *name, int *status_code) {
    if (!is_safe_path(name)) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid animation name");
    }
    
    /* Build file path */
    size_t path_len = strlen(animations_dir) + strlen(name) + 10;
    char *path = malloc(path_len);
    snprintf(path, path_len, "%s/%s.json", animations_dir, name);
    
    if (unlink(path) < 0) {
        free(path);
        *status_code = MHD_HTTP_NOT_FOUND;
        return json_error_response("Animation not found");
    }
    
    free(path);
    
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "deleted", json_object_new_string(name));
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    *status_code = MHD_HTTP_OK;
    
    return response;
}

/* Handle GET /api/config - get full configuration */
static struct MHD_Response *handle_get_config(int *status_code) {
    size_t size;
    char *data = read_file(config_path, &size);
    
    if (!data) {
        *status_code = MHD_HTTP_NOT_FOUND;
        return json_error_response("Configuration file not found");
    }
    
    /* Parse and re-serialize to validate JSON */
    struct json_object *config = json_tokener_parse(data);
    free(data);
    
    if (!config) {
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Invalid configuration JSON");
    }
    
    struct MHD_Response *response = json_success_response(config);
    json_object_put(config);
    *status_code = MHD_HTTP_OK;
    
    return response;
}

/* Handle PUT /api/config - save full configuration */
static struct MHD_Response *handle_put_config(const char *body, int *status_code) {
    if (!body || strlen(body) == 0) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Request body required");
    }
    
    /* Validate JSON */
    struct json_object *config = json_tokener_parse(body);
    if (!config) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid JSON");
    }
    
    /* Validate required fields */
    struct json_object *ipac_obj;
    if (!json_object_object_get_ex(config, "ipac_controllers", &ipac_obj) ||
        json_object_get_type(ipac_obj) != json_type_array) {
        json_object_put(config);
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Missing or invalid 'ipac_controllers' array");
    }
    
    /* Pretty print JSON for saving */
    const char *json_str = json_object_to_json_string_ext(config, JSON_C_TO_STRING_PRETTY);
    
    if (write_file(config_path, json_str, strlen(json_str)) < 0) {
        json_object_put(config);
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Failed to write configuration file");
    }
    
    json_object_put(config);
    
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "message", json_object_new_string("Configuration saved"));
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    *status_code = MHD_HTTP_OK;
    
    return response;
}

/* Handle POST /api/config/backup - create backup of config file */
static struct MHD_Response *handle_backup_config(int *status_code) {
    /* Read current config */
    size_t size;
    char *data = read_file(config_path, &size);
    
    if (!data) {
        *status_code = MHD_HTTP_NOT_FOUND;
        return json_error_response("Configuration file not found");
    }
    
    /* Generate backup filename with timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);
    
    /* Build backup path - same directory as config */
    char backup_path[512];
    snprintf(backup_path, sizeof(backup_path), 
             "%s/config-bak-%s.json", DEFAULT_CONFIG_DIR, timestamp);
    
    /* Write backup file */
    if (write_file(backup_path, data, size) < 0) {
        free(data);
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Failed to write backup file");
    }
    
    free(data);
    
    /* Extract just the filename for the response */
    char backup_filename[64];
    snprintf(backup_filename, sizeof(backup_filename), "config-bak-%s.json", timestamp);
    
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "filename", json_object_new_string(backup_filename));
    json_object_object_add(result, "message", json_object_new_string("Configuration backup created"));
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    *status_code = MHD_HTTP_OK;
    
    return response;
}

/* Handle POST /api/config/test - test LED colors on hardware */
static struct MHD_Response *handle_test_leds(const char *body, int *status_code) {
    if (!body || strlen(body) == 0) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Request body required");
    }
    
    /* Parse JSON body - expects {"emulator": "mame", "rom": "sf2"} */
    struct json_object *req = json_tokener_parse(body);
    if (!req) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid JSON");
    }
    
    struct json_object *emulator_obj, *rom_obj;
    if (!json_object_object_get_ex(req, "emulator", &emulator_obj) ||
        !json_object_object_get_ex(req, "rom", &rom_obj)) {
        json_object_put(req);
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Missing 'emulator' or 'rom' field");
    }
    
    const char *emulator = json_object_get_string(emulator_obj);
    const char *rom = json_object_get_string(rom_obj);
    
    /* Build command to run retropac with the emulator and fake ROM path */
    /* Redirect stderr to /dev/null to prevent SIGPIPE from too many warnings */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
             "/usr/local/bin/retropac --quiet %s /roms/%s.zip 2>/dev/null",
             emulator, rom);
    
    /* Log the command for debugging */
    printf("Test LEDs command: %s\n", cmd);
    
    json_object_put(req);
    
    /* Execute command */
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Failed to execute retropac");
    }
    
    char buf[256];
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        /* Consume any output */
    }
    pclose(fp);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Testing %s/%s", emulator, rom);
    
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "message", json_object_new_string(msg));
    *status_code = MHD_HTTP_OK;
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    
    return response;
}

/* Handle POST /api/config/test-button - test a single button LED */
static struct MHD_Response *handle_test_button_led(const char *body, int *status_code) {
    if (!body || strlen(body) == 0) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Request body required");
    }
    
    /* Parse JSON body - expects {"button": "P1_BUTTON1", "color": "#FFFFFF"} */
    struct json_object *req = json_tokener_parse(body);
    if (!req) {
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Invalid JSON");
    }
    
    struct json_object *button_obj, *color_obj;
    if (!json_object_object_get_ex(req, "button", &button_obj)) {
        json_object_put(req);
        *status_code = MHD_HTTP_BAD_REQUEST;
        return json_error_response("Missing 'button' field");
    }
    
    const char *button = json_object_get_string(button_obj);
    const char *color = "#FFFFFF";  /* Default to white */
    
    if (json_object_object_get_ex(req, "color", &color_obj)) {
        color = json_object_get_string(color_obj);
    }
    
    /* Build command to run retropac with direct button color */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
             "/usr/local/bin/retropac --quiet --set-button %s %s 2>/dev/null",
             button, color);
    
    /* Log the command for debugging */
    printf("Test button LED command: %s\n", cmd);
    
    json_object_put(req);
    
    /* Execute command */
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Failed to execute retropac");
    }
    
    char buf[256];
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        /* Consume any output */
    }
    pclose(fp);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Testing %s with color %s", button, color);
    
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "message", json_object_new_string(msg));
    *status_code = MHD_HTTP_OK;
    
    struct MHD_Response *response = json_success_response(result);
    json_object_put(result);
    
    return response;
}

/* Serve static file */
static struct MHD_Response *serve_static_file(const char *url, int *status_code) {
    /* Build file path */
    const char *file_path = url;
    if (strcmp(url, "/") == 0) {
        file_path = "/index.html";
    }
    
    /* Skip leading slash */
    if (file_path[0] == '/') file_path++;
    
    if (!is_safe_path(file_path)) {
        *status_code = MHD_HTTP_FORBIDDEN;
        return json_error_response("Forbidden");
    }
    
    size_t path_len = strlen(web_dir) + strlen(file_path) + 2;
    char *full_path = malloc(path_len);
    snprintf(full_path, path_len, "%s/%s", web_dir, file_path);
    
    /* Check if file exists */
    struct stat st;
    if (stat(full_path, &st) < 0 || !S_ISREG(st.st_mode)) {
        /* Try index.html for SPA routing */
        free(full_path);
        path_len = strlen(web_dir) + 20;
        full_path = malloc(path_len);
        snprintf(full_path, path_len, "%s/index.html", web_dir);
        
        if (stat(full_path, &st) < 0) {
            free(full_path);
            *status_code = MHD_HTTP_NOT_FOUND;
            return json_error_response("Not found");
        }
    }
    
    /* Read file */
    size_t size;
    char *data = read_file(full_path, &size);
    
    if (!data) {
        free(full_path);
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return json_error_response("Failed to read file");
    }
    
    const char *mime = get_mime_type(full_path);
    free(full_path);
    
    struct MHD_Response *response = MHD_create_response_from_buffer(
        size, data, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", mime);
    
    *status_code = MHD_HTTP_OK;
    return response;
}

/* Connection data for POST/PUT body handling */
struct connection_info {
    char *data;
    size_t size;
};

/* Main request handler */
static enum MHD_Result request_handler(void *cls,
                                        struct MHD_Connection *connection,
                                        const char *url,
                                        const char *method,
                                        const char *version,
                                        const char *upload_data,
                                        size_t *upload_data_size,
                                        void **con_cls) {
    (void)cls; (void)version;
    
    struct MHD_Response *response = NULL;
    int status_code = MHD_HTTP_OK;
    
    /* First call - setup connection info */
    if (*con_cls == NULL) {
        struct connection_info *con_info = calloc(1, sizeof(struct connection_info));
        *con_cls = con_info;
        return MHD_YES;
    }
    
    struct connection_info *con_info = *con_cls;
    
    /* Handle upload data for POST/PUT */
    if (*upload_data_size > 0) {
        if (con_info->size + *upload_data_size > MAX_POST_SIZE) {
            *upload_data_size = 0;
            status_code = MHD_HTTP_CONTENT_TOO_LARGE;
            response = json_error_response("Request too large");
            goto send_response;
        }
        
        char *new_data = realloc(con_info->data, con_info->size + *upload_data_size + 1);
        if (new_data) {
            memcpy(new_data + con_info->size, upload_data, *upload_data_size);
            con_info->size += *upload_data_size;
            new_data[con_info->size] = '\0';
            con_info->data = new_data;
        }
        *upload_data_size = 0;
        return MHD_YES;
    }
    
    /* Handle CORS preflight */
    if (strcmp(method, "OPTIONS") == 0) {
        response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
        status_code = MHD_HTTP_NO_CONTENT;
        goto send_response;
    }
    
    /* API routes */
    if (strncmp(url, "/api/", 5) == 0) {
        const char *api_path = url + 5;
        
        /* GET /api/version - useful for debugging */
        if (strcmp(api_path, "version") == 0 && strcmp(method, "GET") == 0) {
            struct json_object *result = json_object_new_object();
            json_object_object_add(result, "version", json_object_new_string("2.0.0"));
            json_object_object_add(result, "features", json_object_new_string("config-editor"));
            response = json_success_response(result);
            json_object_put(result);
            goto send_response;
        }
        
        /* GET /api/config */
        if (strcmp(api_path, "config") == 0 && strcmp(method, "GET") == 0) {
            response = handle_get_config(&status_code);
            goto send_response;
        }
        
        /* PUT /api/config */
        if (strcmp(api_path, "config") == 0 && strcmp(method, "PUT") == 0) {
            response = handle_put_config(con_info->data, &status_code);
            goto send_response;
        }
        
        /* POST /api/config/backup */
        if (strcmp(api_path, "config/backup") == 0 && strcmp(method, "POST") == 0) {
            response = handle_backup_config(&status_code);
            goto send_response;
        }
        
        /* POST /api/config/test - test LED colors on hardware */
        if (strcmp(api_path, "config/test") == 0 && strcmp(method, "POST") == 0) {
            response = handle_test_leds(con_info->data, &status_code);
            goto send_response;
        }
        
        /* POST /api/config/test-button - test a single button LED */
        if (strcmp(api_path, "config/test-button") == 0 && strcmp(method, "POST") == 0) {
            response = handle_test_button_led(con_info->data, &status_code);
            goto send_response;
        }
        
        /* GET /api/buttons */
        if (strcmp(api_path, "buttons") == 0 && strcmp(method, "GET") == 0) {
            response = handle_get_buttons();
            goto send_response;
        }
        
        /* GET /api/animations */
        if (strcmp(api_path, "animations") == 0 && strcmp(method, "GET") == 0) {
            response = handle_list_animations();
            goto send_response;
        }
        
        /* POST /api/animations/stop - stop any running animation */
        if (strcmp(api_path, "animations/stop") == 0 && strcmp(method, "POST") == 0) {
            response = handle_stop_animation(&status_code);
            goto send_response;
        }
        
        /* /api/animations/:name */
        if (strncmp(api_path, "animations/", 11) == 0) {
            const char *name = api_path + 11;
            
            /* Check for /api/animations/:name/play */
            char *play_suffix = strstr(name, "/play");
            if (play_suffix && strcmp(method, "POST") == 0) {
                /* Extract animation name (everything before /play) */
                size_t name_len = play_suffix - name;
                char *anim_name = malloc(name_len + 1);
                strncpy(anim_name, name, name_len);
                anim_name[name_len] = '\0';
                
                response = handle_play_animation(anim_name, &status_code);
                free(anim_name);
                goto send_response;
            }
            
            /* Check for /api/animations/:name/set-attract */
            char *attract_suffix = strstr(name, "/set-attract");
            if (attract_suffix && strcmp(method, "POST") == 0) {
                /* Extract animation name (everything before /set-attract) */
                size_t name_len = attract_suffix - name;
                char *anim_name = malloc(name_len + 1);
                strncpy(anim_name, name, name_len);
                anim_name[name_len] = '\0';
                
                response = handle_set_attract_mode(anim_name, &status_code);
                free(anim_name);
                goto send_response;
            }
            
            /* Check for /api/animations/:name/duplicate */
            char *dup_suffix = strstr(name, "/duplicate");
            if (dup_suffix && strcmp(method, "POST") == 0) {
                /* Extract animation name (everything before /duplicate) */
                size_t name_len = dup_suffix - name;
                char *anim_name = malloc(name_len + 1);
                strncpy(anim_name, name, name_len);
                anim_name[name_len] = '\0';
                
                response = handle_duplicate_animation(anim_name, &status_code);
                free(anim_name);
                goto send_response;
            }
            
            /* Check for /api/animations/:name/rename */
            char *rename_suffix = strstr(name, "/rename");
            if (rename_suffix && strcmp(method, "POST") == 0) {
                /* Extract animation name (everything before /rename) */
                size_t name_len = rename_suffix - name;
                char *anim_name = malloc(name_len + 1);
                strncpy(anim_name, name, name_len);
                anim_name[name_len] = '\0';
                
                response = handle_rename_animation(anim_name, con_info->data, &status_code);
                free(anim_name);
                goto send_response;
            }
            
            if (strcmp(method, "GET") == 0) {
                response = handle_get_animation(name, &status_code);
                goto send_response;
            }
            
            if (strcmp(method, "PUT") == 0) {
                response = handle_put_animation(name, con_info->data, &status_code);
                goto send_response;
            }
            
            if (strcmp(method, "DELETE") == 0) {
                response = handle_delete_animation(name, &status_code);
                goto send_response;
            }
        }
        
        /* Unknown API route */
        status_code = MHD_HTTP_NOT_FOUND;
        response = json_error_response("API endpoint not found");
        goto send_response;
    }
    
    /* Serve static files */
    if (strcmp(method, "GET") == 0) {
        response = serve_static_file(url, &status_code);
        goto send_response;
    }
    
    status_code = MHD_HTTP_METHOD_NOT_ALLOWED;
    response = json_error_response("Method not allowed");

send_response:
    if (response) {
        enum MHD_Result ret = MHD_queue_response(connection, status_code, response);
        MHD_destroy_response(response);
        
        /* Cleanup connection info */
        if (con_info) {
            free(con_info->data);
            free(con_info);
            *con_cls = NULL;
        }
        
        return ret;
    }
    
    return MHD_NO;
}

/* Print usage */
static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --port <port>            HTTP port (default: %d)\n", DEFAULT_PORT);
    fprintf(stderr, "  --config <path>          Config file path (default: %s)\n", DEFAULT_CONFIG_PATH);
    fprintf(stderr, "  --animations-dir <path>  Animations directory (default: %s)\n", DEFAULT_ANIM_DIR);
    fprintf(stderr, "  --web-dir <path>         Web files directory (default: %s)\n", DEFAULT_WEB_DIR);
    fprintf(stderr, "  --pid-file <path>        PID file path (default: %s)\n", DEFAULT_PID_FILE);
    fprintf(stderr, "  --daemon, -d             Run as a background daemon\n");
    fprintf(stderr, "  --stop                   Stop a running daemon\n");
    fprintf(stderr, "  --help                   Show this help message\n");
}

int main(int argc, char *argv[]) {
    int daemon_mode = 0;
    int stop_mode = 0;
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            server_port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_path = argv[++i];
        } else if (strcmp(argv[i], "--animations-dir") == 0 && i + 1 < argc) {
            animations_dir = argv[++i];
        } else if (strcmp(argv[i], "--web-dir") == 0 && i + 1 < argc) {
            web_dir = argv[++i];
        } else if (strcmp(argv[i], "--pid-file") == 0 && i + 1 < argc) {
            pid_file = argv[++i];
        } else if (strcmp(argv[i], "--daemon") == 0 || strcmp(argv[i], "-d") == 0) {
            daemon_mode = 1;
        } else if (strcmp(argv[i], "--stop") == 0) {
            stop_mode = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    /* Set defaults if not specified */
    if (!animations_dir) animations_dir = DEFAULT_ANIM_DIR;
    if (!web_dir) web_dir = DEFAULT_WEB_DIR;
    if (!config_path) config_path = DEFAULT_CONFIG_PATH;
    if (!pid_file) pid_file = DEFAULT_PID_FILE;
    
    /* Convert relative paths to absolute paths (needed for daemon mode) */
    char *abs_animations_dir = realpath(animations_dir, NULL);
    char *abs_web_dir = realpath(web_dir, NULL);
    char *abs_config_path = realpath(config_path, NULL);
    
    if (abs_animations_dir) animations_dir = abs_animations_dir;
    if (abs_web_dir) web_dir = abs_web_dir;
    if (abs_config_path) config_path = abs_config_path;
    
    /* Handle stop command */
    if (stop_mode) {
        return stop_daemon(pid_file);
    }
    
    /* Verify directories exist */
    struct stat st;
    if (stat(animations_dir, &st) < 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: Animations directory '%s' not found\n", animations_dir);
        return 1;
    }
    
    if (stat(web_dir, &st) < 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Warning: Web directory '%s' not found\n", web_dir);
        fprintf(stderr, "Static file serving will not work until web app is built.\n");
        fprintf(stderr, "Run 'make web' to build the web application.\n\n");
    }
    
    if (stat(config_path, &st) < 0) {
        /* Config doesn't exist - try to copy from example */
        size_t example_size;
        char *example_data = read_file(CONFIG_EXAMPLE_PATH, &example_size);
        
        if (example_data) {
            /* Create config directory if it doesn't exist */
            mkdir(DEFAULT_CONFIG_DIR, 0755);
            
            if (write_file(config_path, example_data, example_size) == 0) {
                printf("Created config file from example: %s\n", config_path);
            } else {
                fprintf(stderr, "Warning: Could not create config file at '%s'\n", config_path);
                fprintf(stderr, "Config editor will not work until a config.json file exists.\n\n");
            }
            free(example_data);
        } else {
            fprintf(stderr, "Warning: Configuration file '%s' not found\n", config_path);
            fprintf(stderr, "Example config '%s' also not found.\n", CONFIG_EXAMPLE_PATH);
            fprintf(stderr, "Config editor will not work until a config.json file exists.\n\n");
        }
    }
    
    /* Setup signal handlers */
    setup_signals();
    
    /* Print server info before daemonizing (so user sees it) */
    printf("===========================================\n");
    printf("  RetroPac Animation Editor Server\n");
    printf("===========================================\n\n");
    printf("Server running at: http://%s:%d\n", get_local_ip(), server_port);
    printf("Animations directory: %s\n", animations_dir);
    printf("Configuration file: %s\n", config_path);
    printf("Web directory: %s\n", web_dir);
    if (daemon_mode) {
        printf("PID file: %s\n", pid_file);
        printf("Mode: daemon\n\n");
    } else {
        printf("\nPress Ctrl+C to stop the server.\n\n");
    }
    
    /* Daemonize if requested */
    if (daemon_mode) {
        if (daemonize() < 0) {
            return 1;
        }
        
        /* Write PID file */
        if (write_pid_file(pid_file) < 0) {
            return 1;
        }
        
        /* Register cleanup on exit */
        atexit(remove_pid_file);
    }
    
    /* Start HTTP server */
    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG,
        server_port,
        NULL, NULL,
        &request_handler, NULL,
        MHD_OPTION_END);
    
    if (!daemon) {
        fprintf(stderr, "Error: Failed to start HTTP server on port %d\n", server_port);
        return 1;
    }
    
    /* Wait for shutdown signal */
    while (running) {
        sleep(1);
    }
    
    printf("\nShutting down server...\n");
    MHD_stop_daemon(daemon);
    
    return 0;
}
