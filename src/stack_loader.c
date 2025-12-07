#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack_loader.h"
#include "log.h"
#include "cJSON.h"

/* Read entire file into a null-terminated buffer. Caller must free(). */
static char *read_file_to_buffer(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }

    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);

    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);

    if (read != (size_t)size) {
        free(buf);
        return NULL;
    }

    buf[size] = '\0';
    return buf;
}

int load_stack_from_file(const char *stack_id, Stack *out) {
    if (!stack_id || !out) return -1;

    char path[256];
    snprintf(path, sizeof(path), "stacks/%s.json", stack_id);

    char *json_text = read_file_to_buffer(path);
    if (!json_text) {
        log_error("Failed to read stack file.");
        return -1;
    }

    cJSON *root = cJSON_Parse(json_text);
    free(json_text);

    if (!root) {
        log_error("Failed to parse JSON.");
        return -1;
    }

    memset(out, 0, sizeof(*out));

    cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON *packages = cJSON_GetObjectItemCaseSensitive(root, "packages");

    if (!cJSON_IsString(id) || !cJSON_IsString(name) || !cJSON_IsArray(packages)) {
        log_error("Invalid stack JSON structure.");
        cJSON_Delete(root);
        return -1;
    }

    out->id = strdup(id->valuestring);
    out->name = strdup(name->valuestring);

    int count = cJSON_GetArraySize(packages);
    if (count <= 0) {
        log_error("Stack has no packages.");
        cJSON_Delete(root);
        return -1;
    }

    out->package_count = count;
    out->packages = (Package *)calloc((size_t)count, sizeof(Package));
    if (!out->packages) {
        log_error("Out of memory allocating packages.");
        cJSON_Delete(root);
        return -1;
    }

    int i = 0;
    cJSON *pkg_json = NULL;
    cJSON_ArrayForEach(pkg_json, packages) {
        if (!cJSON_IsObject(pkg_json) || i >= count) continue;

        cJSON *pid      = cJSON_GetObjectItemCaseSensitive(pkg_json, "id");
        cJSON *display  = cJSON_GetObjectItemCaseSensitive(pkg_json, "display_name");
        cJSON *win_cmd  = cJSON_GetObjectItemCaseSensitive(pkg_json, "windows_cmd");
        cJSON *lin_cmd  = cJSON_GetObjectItemCaseSensitive(pkg_json, "linux_cmd");
        cJSON *verify   = cJSON_GetObjectItemCaseSensitive(pkg_json, "verify_cmd");

        if (!cJSON_IsString(pid) || !cJSON_IsString(display)) {
            log_error("Package missing id or display_name.");
            continue;
        }

        Package *p = &out->packages[i];

        p->id = strdup(pid->valuestring);
        p->display_name = strdup(display->valuestring);

        if (cJSON_IsString(win_cmd))  p->windows_cmd = strdup(win_cmd->valuestring);
        if (cJSON_IsString(lin_cmd))  p->linux_cmd   = strdup(lin_cmd->valuestring);
        if (cJSON_IsString(verify))   p->verify_cmd  = strdup(verify->valuestring);

        i++;
    }

    out->package_count = i;

    cJSON_Delete(root);

    if (out->package_count == 0) {
        log_error("No valid packages parsed.");
        free_stack(out);
        return -1;
    }

    return 0;
}

void free_stack(Stack *stack) {
    if (!stack) return;

    if (stack->id)   free(stack->id);
    if (stack->name) free(stack->name);

    if (stack->packages) {
        for (int i = 0; i < stack->package_count; ++i) {
            Package *p = &stack->packages[i];
            if (p->id)           free(p->id);
            if (p->display_name) free(p->display_name);
            if (p->windows_cmd)  free(p->windows_cmd);
            if (p->linux_cmd)    free(p->linux_cmd);
            if (p->verify_cmd)   free(p->verify_cmd);
        }
        free(stack->packages);
    }

    memset(stack, 0, sizeof(*stack));
}
