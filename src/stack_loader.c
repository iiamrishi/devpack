#include "stack_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include "cJSON.h"

/* ---------------------------------------------------------
 * Utility: simple strdup replacement
 * --------------------------------------------------------- */
static char *xstrdup(const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len + 1);
    return copy;
}

/* ---------------------------------------------------------
 * Utility: read entire file into NUL-terminated buffer
 * --------------------------------------------------------- */
static int read_file(const char *path, char **out_buf)
{
    *out_buf = NULL;
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }

    long len = ftell(fp);
    if (len < 0) {
        fclose(fp);
        return -1;
    }

    rewind(fp);

    char *buf = malloc((size_t)len + 1);
    if (!buf) {
        fclose(fp);
        return -1;
    }

    size_t n = fread(buf, 1, (size_t)len, fp);
    fclose(fp);

    if (n != (size_t)len) {
        free(buf);
        return -1;
    }

    buf[len] = '\0';
    *out_buf = buf;
    return 0;
}

/* ---------------------------------------------------------
 * Load single stack from stacks/<id>.json
 * --------------------------------------------------------- */
int load_stack_from_file(const char *stack_id, Stack *out)
{
    if (!stack_id || !out) return -1;
    memset(out, 0, sizeof(*out));

    char path[256];
    snprintf(path, sizeof(path), "stacks/%s.json", stack_id);

    char *json_text = NULL;
    if (read_file(path, &json_text) != 0) {
        fprintf(stderr, "Could not read stack file: %s\n", path);
        return -1;
    }

    cJSON *root = cJSON_Parse(json_text);
    free(json_text);

    if (!root) {
        fprintf(stderr, "Invalid JSON in %s\n", path);
        return -1;
    }

    cJSON *id       = cJSON_GetObjectItemCaseSensitive(root, "id");
    cJSON *name     = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON *packages = cJSON_GetObjectItemCaseSensitive(root, "packages");

    if (!cJSON_IsString(id) || !cJSON_IsString(name) || !cJSON_IsArray(packages)) {
        fprintf(stderr, "Stack JSON missing required fields in %s\n", path);
        cJSON_Delete(root);
        return -1;
    }

    out->id   = xstrdup(id->valuestring);
    out->name = xstrdup(name->valuestring);

    int pkg_count = cJSON_GetArraySize(packages);
    if (pkg_count <= 0) {
        fprintf(stderr, "Stack '%s' has no packages\n", out->id);
        cJSON_Delete(root);
        return -1;
    }

    out->packages = calloc((size_t)pkg_count, sizeof(Package));
    if (!out->packages) {
        cJSON_Delete(root);
        return -1;
    }
    out->package_count = pkg_count;

    int idx = 0;
    cJSON *pkg_json = NULL;
    cJSON_ArrayForEach(pkg_json, packages) {
        if (!cJSON_IsObject(pkg_json)) continue;

        Package *p = &out->packages[idx++];

        cJSON *pid  = cJSON_GetObjectItemCaseSensitive(pkg_json, "id");
        cJSON *disp = cJSON_GetObjectItemCaseSensitive(pkg_json, "display_name");
        cJSON *win  = cJSON_GetObjectItemCaseSensitive(pkg_json, "windows_cmd");
        cJSON *lin  = cJSON_GetObjectItemCaseSensitive(pkg_json, "linux_cmd");
        cJSON *ver  = cJSON_GetObjectItemCaseSensitive(pkg_json, "verify_cmd");

        if (cJSON_IsString(pid))  p->id           = xstrdup(pid->valuestring);
        if (cJSON_IsString(disp)) p->display_name = xstrdup(disp->valuestring);
        if (cJSON_IsString(win))  p->windows_cmd  = xstrdup(win->valuestring);
        if (cJSON_IsString(lin))  p->linux_cmd    = xstrdup(lin->valuestring);
        if (cJSON_IsString(ver))  p->verify_cmd   = xstrdup(ver->valuestring);
    }

    /* Optional: depends_on array of stack IDs */
    cJSON *deps = cJSON_GetObjectItemCaseSensitive(root, "depends_on");
    if (cJSON_IsArray(deps)) {
        int dep_count = cJSON_GetArraySize(deps);
        if (dep_count > 0) {
            out->depends_on = calloc((size_t)dep_count, sizeof(char *));
            if (out->depends_on) {
                int dep_idx = 0;
                cJSON *dep = NULL;
                cJSON_ArrayForEach(dep, deps) {
                    if (cJSON_IsString(dep)) {
                        out->depends_on[dep_idx++] = xstrdup(dep->valuestring);
                    }
                }
                out->depends_count = dep_idx; /* in case some entries were invalid */
            }
        }
    }

    cJSON_Delete(root);
    return 0;
}

/* ---------------------------------------------------------
 * List available stacks (human-readable)
 * --------------------------------------------------------- */
int list_available_stacks(void)
{
    DIR *dir = opendir("stacks");
    if (!dir) {
        perror("opendir(stacks)");
        return 1;
    }

    printf("Available stacks:\n");

    struct dirent *ent;
    int found = 0;

    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;

        if (name[0] == '.') continue;

        size_t len = strlen(name);
        if (len <= 5 || strcmp(name + len - 5, ".json") != 0)
            continue;

        char stack_id[256];
        size_t id_len = len - 5;
        if (id_len >= sizeof(stack_id)) id_len = sizeof(stack_id) - 1;
        memcpy(stack_id, name, id_len);
        stack_id[id_len] = '\0';

        Stack s;
        if (load_stack_from_file(stack_id, &s) == 0) {
            printf(" - %s (%s)\n",
                   s.id ? s.id : stack_id,
                   s.name ? s.name : "(no name)");
            free_stack(&s);
        } else {
            printf(" - %s (invalid)\n", stack_id);
        }

        found++;
    }

    closedir(dir);

    if (!found) {
        printf(" (no stacks found)\n");
    }

    return 0;
}

/* ---------------------------------------------------------
 * List available stacks as JSON
 * --------------------------------------------------------- */
int list_available_stacks_json(void)
{
    DIR *dir = opendir("stacks");
    if (!dir) {
        perror("opendir(stacks)");
        return 1;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        closedir(dir);
        return 1;
    }

    cJSON *arr = cJSON_CreateArray();
    if (!arr) {
        cJSON_Delete(root);
        closedir(dir);
        return 1;
    }
    cJSON_AddItemToObject(root, "stacks", arr);

    struct dirent *ent;
    int found = 0;

    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;

        if (name[0] == '.') continue;

        size_t len = strlen(name);
        if (len <= 5 || strcmp(name + len - 5, ".json") != 0)
            continue;

        char stack_id[256];
        size_t id_len = len - 5;
        if (id_len >= sizeof(stack_id)) id_len = sizeof(stack_id) - 1;
        memcpy(stack_id, name, id_len);
        stack_id[id_len] = '\0';

        Stack s;
        if (load_stack_from_file(stack_id, &s) == 0) {
            cJSON *item = cJSON_CreateObject();
            if (item) {
                cJSON_AddStringToObject(
                    item, "id", s.id ? s.id : stack_id);
                if (s.name) {
                    cJSON_AddStringToObject(item, "name", s.name);
                }
                cJSON_AddStringToObject(item, "file", name);
                cJSON_AddNumberToObject(item, "package_count",
                                        s.package_count);

                /* Include depends_on if present */
                if (s.depends_count > 0 && s.depends_on) {
                    cJSON *deps_arr = cJSON_CreateArray();
                    if (deps_arr) {
                        for (int d = 0; d < s.depends_count; ++d) {
                            if (s.depends_on[d]) {
                                cJSON_AddItemToArray(
                                    deps_arr,
                                    cJSON_CreateString(s.depends_on[d]));
                            }
                        }
                        cJSON_AddItemToObject(item, "depends_on", deps_arr);
                    }
                }

                cJSON_AddItemToArray(arr, item);
            }
            free_stack(&s);
            found++;
        } else {
            /* skip invalid stacks in JSON mode */
        }
    }

    closedir(dir);

    /* Even if found==0, we still print: { "stacks": [] } */
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
