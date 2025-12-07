#include "stack_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

/* simple strdup replacement */
static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *copy = (char *)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len + 1);
    return copy;
}

/* read entire file into buffer, NUL-terminated */
static int read_file(const char *path, char **out_buf) {
    *out_buf = NULL;

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }

    long len = ftell(fp);
    if (len < 0) {
        fclose(fp);
        return -1;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    char *buf = (char *)malloc((size_t)len + 1);
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

int load_stack_from_file(const char *stack_id, Stack *out) {
    if (!stack_id || !out) {
        return -1;
    }

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

    out->packages = (Package *)calloc((size_t)pkg_count, sizeof(Package));
    if (!out->packages) {
        cJSON_Delete(root);
        return -1;
    }
    out->package_count = pkg_count;

    int idx = 0;
    cJSON *pkg_json = NULL;
    cJSON_ArrayForEach(pkg_json, packages) {
        if (!cJSON_IsObject(pkg_json)) {
            continue;
        }

        cJSON *pid          = cJSON_GetObjectItemCaseSensitive(pkg_json, "id");
        cJSON *display_name = cJSON_GetObjectItemCaseSensitive(pkg_json, "display_name");
        cJSON *windows_cmd  = cJSON_GetObjectItemCaseSensitive(pkg_json, "windows_cmd");
        cJSON *linux_cmd    = cJSON_GetObjectItemCaseSensitive(pkg_json, "linux_cmd");
        cJSON *verify_cmd   = cJSON_GetObjectItemCaseSensitive(pkg_json, "verify_cmd");

        Package *p = &out->packages[idx++];

        p->id           = (pid && cJSON_IsString(pid)) ? xstrdup(pid->valuestring) : NULL;
        p->display_name = (display_name && cJSON_IsString(display_name)) ? xstrdup(display_name->valuestring) : NULL;
        p->windows_cmd  = (windows_cmd && cJSON_IsString(windows_cmd)) ? xstrdup(windows_cmd->valuestring) : NULL;
        p->linux_cmd    = (linux_cmd && cJSON_IsString(linux_cmd)) ? xstrdup(linux_cmd->valuestring) : NULL;
        p->verify_cmd   = (verify_cmd && cJSON_IsString(verify_cmd)) ? xstrdup(verify_cmd->valuestring) : NULL;
    }

    cJSON_Delete(root);
    return 0;
}
