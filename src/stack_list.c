#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "stack_list.h"
#include "log.h"
#include "cJSON.h"

#define STACKS_DIR "stacks"
#define STACK_PATH_MAX 512

/* Read entire file into a null-terminated buffer.
 * Caller must free() the returned buffer.
 */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }

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

int list_stacks(void) {
    DIR *dir = opendir(STACKS_DIR);
    if (!dir) {
        log_error("Failed to open stacks/ directory.");
        return 1;
    }

    printf("Available stacks:\n");

    struct dirent *ent;
    int found = 0;

    while ((ent = readdir(dir)) != NULL) {
        /* Only care about .json files */
        if (!strstr(ent->d_name, ".json")) {
            continue;
        }

        char path[STACK_PATH_MAX];
        int written = snprintf(
            path,
            sizeof(path),
            "%s/%s",
            STACKS_DIR,
            ent->d_name
        );

        if (written < 0 || written >= (int)sizeof(path)) {
            log_error("Stack filename too long, skipping.");
            continue;
        }

        char *json_text = read_file(path);
        if (!json_text) {
            log_error("Failed to read stack file.");
            continue;
        }

        cJSON *root = cJSON_Parse(json_text);
        free(json_text);

        if (!root) {
            log_error("Invalid JSON in stack file.");
            continue;
        }

        cJSON *id   = cJSON_GetObjectItemCaseSensitive(root, "id");
        cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");

        if (cJSON_IsString(id) && cJSON_IsString(name)) {
            printf("  - %-12s : %s\n", id->valuestring, name->valuestring);
            found = 1;
        }

        cJSON_Delete(root);
    }

    closedir(dir);

    if (!found) {
        printf("  (no stacks found)\n");
    }

    return 0;
}
