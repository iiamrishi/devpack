#include "stack_list.h"
#include "stack.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "cJSON.h"

/* ---------------------------------------------------------
 * Helper: run a shell command and capture first line output
 * --------------------------------------------------------- */
static bool run_command_capture(const char *cmd,
                                char *buffer,
                                size_t buffer_size)
{
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return false;
    }

    if (!fgets(buffer, (int)buffer_size, fp)) {
        pclose(fp);
        return false;
    }

    int status = pclose(fp);

    /* Trim trailing newline */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    /* only succeed if command exited normally */
    if (status != 0) {
        return false;
    }

    return true;
}

/* ---------------------------------------------------------
 * C toolchain detection
 * --------------------------------------------------------- */
bool detect_c_toolchain(char *details, size_t details_size)
{
    char output[256];

#if defined(_WIN32)
    if (run_command_capture("gcc --version 2>&1", output, sizeof(output)) ||
        run_command_capture("clang --version 2>&1", output, sizeof(output))) {
#else
    if (run_command_capture("gcc --version 2>&1", output, sizeof(output)) ||
        run_command_capture("clang --version 2>&1", output, sizeof(output))) {
#endif
        snprintf(details, details_size, "%s", output);
        return true;
    }

    snprintf(details, details_size, "gcc/clang not found");
    return false;
}

/* ---------------------------------------------------------
 * Python detection
 * --------------------------------------------------------- */
bool detect_python(char *details, size_t details_size)
{
    char output[256];

#if defined(_WIN32)
    const char *cmds[] = {
        "py -V 2>&1",
        "python -V 2>&1",
        "python3 -V 2>&1"
    };
#else
    const char *cmds[] = {
        "python3 --version 2>&1",
        "python --version 2>&1"
    };
#endif

    size_t count = sizeof(cmds) / sizeof(cmds[0]);
    for (size_t i = 0; i < count; i++) {
        if (run_command_capture(cmds[i], output, sizeof(output))) {
            snprintf(details, details_size, "%s", output);
            return true;
        }
    }

    snprintf(details, details_size, "Python not found in PATH");
    return false;
}

/* ---------------------------------------------------------
 * Docker detection
 * --------------------------------------------------------- */
bool detect_docker(char *details, size_t details_size)
{
    char output[256];

    if (run_command_capture("docker --version 2>&1", output, sizeof(output))) {
        snprintf(details, details_size, "%s", output);
        return true;
    }

    snprintf(details, details_size, "Docker not found in PATH");
    return false;
}

/* ---------------------------------------------------------
 * Git detection
 * --------------------------------------------------------- */
bool detect_git(char *details, size_t details_size)
{
    char output[256];

    if (run_command_capture("git --version 2>&1", output, sizeof(output))) {
        snprintf(details, details_size, "%s", output);
        return true;
    }

    snprintf(details, details_size, "git not found in PATH");
    return false;
}

/* ---------------------------------------------------------
 * Node.js detection
 * --------------------------------------------------------- */
bool detect_nodejs(char *details, size_t details_size)
{
    char node_out[128] = {0};
    char npm_out[128]  = {0};
    char tmp[128];
    bool found_node = false;

#if defined(_WIN32)
    const char *node_cmds[] = {
        "node --version 2>&1"
    };
#else
    const char *node_cmds[] = {
        "node --version 2>&1",
        "nodejs --version 2>&1"
    };
#endif

    size_t count = sizeof(node_cmds) / sizeof(node_cmds[0]);
    for (size_t i = 0; i < count; i++) {
        if (run_command_capture(node_cmds[i], tmp, sizeof(tmp))) {
            snprintf(node_out, sizeof(node_out), "%s", tmp);
            found_node = true;
            break;
        }
    }

    if (!found_node) {
        snprintf(details, details_size, "Node.js not found in PATH");
        return false;
    }

    /* Try to get npm version too (optional) */
    if (run_command_capture("npm --version 2>&1", tmp, sizeof(tmp))) {
        snprintf(npm_out, sizeof(npm_out), "%s", tmp);
        snprintf(details, details_size, "%s / npm %s", node_out, npm_out);
    } else {
        snprintf(details, details_size, "%s (npm not found)", node_out);
    }

    return true;
}

/* ---------------------------------------------------------
 * Rust detection
 * --------------------------------------------------------- */
bool detect_rust(char *details, size_t details_size)
{
    char rustc_out[128] = {0};
    char cargo_out[128] = {0};
    char tmp[128];

    /* rustc is the main signal */
    if (!run_command_capture("rustc --version 2>&1", tmp, sizeof(tmp))) {
        snprintf(details, details_size, "rustc not found in PATH");
        return false;
    }

    snprintf(rustc_out, sizeof(rustc_out), "%s", tmp);

    /* cargo is optional but nice */
    if (run_command_capture("cargo --version 2>&1", tmp, sizeof(tmp))) {
        snprintf(cargo_out, sizeof(cargo_out), "%s", tmp);
        snprintf(details, details_size, "%s / %s", rustc_out, cargo_out);
    } else {
        snprintf(details, details_size, "%s (cargo not found)", rustc_out);
    }

    return true;
}

/* ---------------------------------------------------------
 * Stack registry
 * --------------------------------------------------------- */
static DevStack STACKS[] = {
    { "C toolchain", detect_c_toolchain },
    { "Python",      detect_python      },
    { "Git",         detect_git         },
    { "Node.js",     detect_nodejs      },
    { "Docker",      detect_docker      },
    { "Rust",        detect_rust        },
};

/* ---------------------------------------------------------
 * Public API: human-readable list
 * --------------------------------------------------------- */
int list_stacks(void)
{
    char details[256];

    size_t count = sizeof(STACKS) / sizeof(STACKS[0]);

    bool has_git    = false;
    bool has_node   = false;
    bool has_docker = false;

    for (size_t i = 0; i < count; i++) {
        memset(details, 0, sizeof(details));

        DevStack *s = &STACKS[i];
        bool ok = s->detect_fn(details, sizeof(details));

        if (strcmp(s->name, "Git") == 0 && ok) {
            has_git = true;
        } else if (strcmp(s->name, "Node.js") == 0 && ok) {
            has_node = true;
        } else if (strcmp(s->name, "Docker") == 0 && ok) {
            has_docker = true;
        }

        const char *tag_color = ok ? COLOR_GREEN : COLOR_RED;
        const char *tag_text  = ok ? "OK" : "MISSING";

        printf("[%s%s%s] %s\n", tag_color, tag_text, COLOR_RESET, s->name);
        if (details[0] != '\0') {
            printf("    -> %s\n", details);
        }
    }

    printf("\n");

    bool web_ok         = has_git && has_node;
    const char *web_c   = web_ok ? COLOR_GREEN : COLOR_RED;
    const char *web_tag = web_ok ? "OK" : "MISSING";

    printf("[%s%s%s] Web Dev\n", web_c, web_tag, COLOR_RESET);
    printf("    -> needs Git + Node.js%s\n",
           has_docker ? " (Docker available)" : " (Docker optional)");

    return 0;
}

/* ---------------------------------------------------------
 * Public API: JSON list
 * --------------------------------------------------------- */
int list_stacks_json(void)
{
    char details[256];
    size_t count = sizeof(STACKS) / sizeof(STACKS[0]);

    bool has_git    = false;
    bool has_node   = false;
    bool has_docker = false;

    cJSON *root = cJSON_CreateObject();
    if (!root) return 1;

    cJSON *arr = cJSON_CreateArray();
    if (!arr) {
        cJSON_Delete(root);
        return 1;
    }
    cJSON_AddItemToObject(root, "stacks", arr);

    for (size_t i = 0; i < count; i++) {
        memset(details, 0, sizeof(details));

        DevStack *s = &STACKS[i];
        bool ok = s->detect_fn(details, sizeof(details));

        if (strcmp(s->name, "Git") == 0 && ok) {
            has_git = true;
        } else if (strcmp(s->name, "Node.js") == 0 && ok) {
            has_node = true;
        } else if (strcmp(s->name, "Docker") == 0 && ok) {
            has_docker = true;
        }

        cJSON *item = cJSON_CreateObject();
        if (!item) continue;

        cJSON_AddStringToObject(item, "name", s->name);
        cJSON_AddStringToObject(item, "status", ok ? "OK" : "MISSING");
        if (details[0] != '\0') {
            cJSON_AddStringToObject(item, "details", details);
        }

        cJSON_AddItemToArray(arr, item);
    }

    /* Web Dev combined info */
    cJSON *web = cJSON_CreateObject();
    if (web) {
        bool web_ok = has_git && has_node;
        cJSON_AddStringToObject(web, "name", "Web Dev");
        cJSON_AddStringToObject(web, "status", web_ok ? "OK" : "MISSING");

        cJSON *req = cJSON_CreateArray();
        if (req) {
            cJSON_AddItemToArray(req, cJSON_CreateString("Git"));
            cJSON_AddItemToArray(req, cJSON_CreateString("Node.js"));
            cJSON_AddItemToObject(web, "requires", req);
        }

        cJSON_AddStringToObject(web, "docker",
                                has_docker ? "available" : "optional-or-missing");

        cJSON_AddItemToObject(root, "web_dev", web);
    }

    char *json = cJSON_Print(root);
    if (!json) {
        cJSON_Delete(root);
        return 1;
    }

    printf("%s\n", json);
    free(json);
    cJSON_Delete(root);
    return 0;
}

/* ---------------------------------------------------------
 * doctor: environment diagnostics
 * --------------------------------------------------------- */

static int command_exists_simple(const char *name)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1", name);
    return system(cmd) == 0;
}

int doctor(void)
{
    printf("devpack doctor\n\n");

    /* -------- OS + Arch -------- */
    struct utsname u;
    if (uname(&u) == 0) {
        printf("OS          : %s\n", u.sysname);
        printf("Kernel      : %s\n", u.release);
        printf("Arch        : %s\n", u.machine);
    } else {
        printf("OS          : unknown\n");
    }

#if defined(_WIN32)
    printf("Platform    : Windows\n");
#else
    /* -------- Distro -------- */
    FILE *fp = fopen("/etc/os-release", "r");
    char distro[128] = "unknown";

    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                char *val = strchr(line, '=');
                if (val) {
                    val++;
                    if (*val == '\"') val++;
                    strncpy(distro, val, sizeof(distro) - 1);
                    distro[strcspn(distro, "\"\n")] = 0;
                }
                break;
            }
        }
        fclose(fp);
    }

    printf("Distro      : %s\n", distro);

    /* -------- Package manager -------- */
    const char *pm = "unknown";
    if (command_exists_simple("pacman")) pm = "pacman";
    else if (command_exists_simple("apt")) pm = "apt";
    else if (command_exists_simple("dnf")) pm = "dnf";
    else if (command_exists_simple("yum")) pm = "yum";
    else if (command_exists_simple("zypper")) pm = "zypper";
    else if (command_exists_simple("brew")) pm = "brew";

    printf("Package mgr : %s\n", pm);

    /* -------- Shell -------- */
    const char *shell = getenv("SHELL");
    printf("Shell       : %s\n", shell ? shell : "unknown");

    /* -------- User / sudo -------- */
    if (geteuid() == 0) {
        printf("User        : root\n");
    } else {
        int has_sudo = command_exists_simple("sudo");
        printf("User        : regular (%s)\n",
               has_sudo ? "sudo available" : "no sudo");
    }
#endif

    return 0;
}
