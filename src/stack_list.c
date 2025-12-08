#include "stack_list.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define _POSIX_C_SOURCE 200809L


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

    /* ✅ only succeed if command exited normally */
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
 * Public API
 * --------------------------------------------------------- */
int list_stacks(void)
{
    char details[256];

    size_t count = sizeof(STACKS) / sizeof(STACKS[0]);

    bool has_git = false;
    bool has_node = false;
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

        printf("[%s] %s\n", ok ? "OK" : "MISSING", s->name);
        if (details[0] != '\0') {
            printf("    -> %s\n", details);
        }
    }

    printf("\n");

    bool web_ok = has_git && has_node;
    printf("[%s] Web Dev\n", web_ok ? "OK" : "MISSING");
    printf("    -> needs Git + Node.js%s\n",
           has_docker ? " (Docker available)" : " (Docker optional)");

    return 0;
}