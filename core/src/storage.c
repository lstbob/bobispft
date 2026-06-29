#include "storage.h"
#include "date_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAGIC 0x504C414E
#define VERSION 1

static uint8_t compute_checksum(const uint8_t *data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum ^= data[i];
    }
    return sum;
}

static const char *file_path(PlanKey key, char *buf, size_t buf_size) {
    switch (key.scope) {
        case SCOPE_DAY:
        case SCOPE_WEEK:
            snprintf(buf, buf_size, DATA_DIR "/%c-%04d-%02d-%02d.bobip",
                     SCOPE_PREFIXES[key.scope],
                     key.anchor.year, key.anchor.month, key.anchor.day);
            break;
        case SCOPE_MONTH:
            snprintf(buf, buf_size, DATA_DIR "/%c-%04d-%02d.bobip",
                     SCOPE_PREFIXES[key.scope],
                     key.anchor.year, key.anchor.month);
            break;
        case SCOPE_YEAR:
            snprintf(buf, buf_size, DATA_DIR "/%c-%04d.bobip",
                     SCOPE_PREFIXES[key.scope],
                     key.anchor.year);
            break;
        default:
            snprintf(buf, buf_size, DATA_DIR "/unknown.bobip");
            break;
    }
    return buf;
}

bool storage_init(void) {
    struct stat st = {0};
    if (stat(DATA_DIR, &st) == -1) {
        if (mkdir(DATA_DIR, 0700) == -1) {
            return false;
        }
    }
    return true;
}

bool save_plan(const Plan *plan) {
    char path[256];
    file_path(plan->key, path, sizeof(path));

    FILE *f = fopen(path, "w+b");
    if (!f) return false;

    uint32_t magic = MAGIC;
    uint8_t version = VERSION;
    uint8_t scope = (uint8_t)plan->key.scope;
    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&version, sizeof(version), 1, f);
    fputc(0, f);
    fwrite(&scope, sizeof(scope), 1, f);

    fwrite(&plan->key.anchor.year, sizeof(plan->key.anchor.year), 1, f);
    fwrite(&plan->key.anchor.month, sizeof(plan->key.anchor.month), 1, f);
    fwrite(&plan->key.anchor.day, sizeof(plan->key.anchor.day), 1, f);

    char title_buf[MAX_TITLE_LEN];
    memset(title_buf, 0, sizeof(title_buf));
    snprintf(title_buf, sizeof(title_buf), "%s", plan->title);
    fwrite(title_buf, 1, MAX_TITLE_LEN, f);

    uint32_t content_len = (uint32_t)plan->content_len;
    fwrite(&content_len, sizeof(content_len), 1, f);
    if (content_len > 0 && plan->content) {
        fwrite(plan->content, 1, content_len, f);
    }

    {
        long data_end = ftell(f);
        uint8_t *buf = (uint8_t *)malloc((size_t)data_end);
        if (buf) {
            fseek(f, 0, SEEK_SET);
            size_t nread = fread(buf, 1, (size_t)data_end, f);
            if (nread > 0) {
                uint8_t sum = compute_checksum(buf + 6, nread - 6);
                fseek(f, 5, SEEK_SET);
                fwrite(&sum, 1, 1, f);
            }
            free(buf);
        }
    }

    fclose(f);
    return true;
}

bool load_plan(PlanKey key, Plan *out) {
    char path[256];
    file_path(key, path, sizeof(path));

    FILE *f = fopen(path, "rb");
    if (!f) return false;

    uint32_t magic;
    uint8_t version;
    if (fread(&magic, sizeof(magic), 1, f) != 1 || magic != MAGIC) {
        fclose(f);
        return false;
    }
    if (fread(&version, sizeof(version), 1, f) != 1 || version != VERSION) {
        fclose(f);
        return false;
    }

    uint8_t stored_checksum;
    if (fread(&stored_checksum, 1, 1, f) != 1) {
        fclose(f);
        return false;
    }

    long data_start = ftell(f);
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, data_start, SEEK_SET);

    size_t data_len = (size_t)(file_size - data_start);
    uint8_t *data = (uint8_t *)malloc(data_len + 1);
    if (!data) {
        fclose(f);
        return false;
    }
    size_t nread = fread(data, 1, data_len, f);
    if (nread != data_len || compute_checksum(data, data_len) != stored_checksum) {
        free(data);
        fclose(f);
        return false;
    }

    fseek(f, (long)data_start, SEEK_SET);
    plan_clear(out);

    /* On-disk data is untrusted: the XOR-8 checksum only detects accidental
     * corruption, so check every read and validate before using anything. */
    bool ok = true;

    uint8_t stored_scope;
    ok = ok && fread(&stored_scope, sizeof(stored_scope), 1, f) == 1;
    if (ok && stored_scope >= SCOPE_COUNT) {
        free(data);
        fclose(f);
        return false;
    }

    ok = ok && fread(&out->key.anchor.year, sizeof(out->key.anchor.year), 1, f) == 1;
    ok = ok && fread(&out->key.anchor.month, sizeof(out->key.anchor.month), 1, f) == 1;
    ok = ok && fread(&out->key.anchor.day, sizeof(out->key.anchor.day), 1, f) == 1;

    char title_buf[MAX_TITLE_LEN];
    ok = ok && fread(title_buf, 1, MAX_TITLE_LEN, f) == MAX_TITLE_LEN;

    uint32_t content_len = 0;
    ok = ok && fread(&content_len, sizeof(content_len), 1, f) == 1;

    /* Reject short reads and out-of-range dates: out->key.anchor drives
     * month_name()[month-1] in the renderer. */
    if (!ok || !date_valid(out->key.anchor)) {
        free(data);
        fclose(f);
        return false;
    }

    /* Verify the stored scope matches the requested scope. A day file loaded
     * as a week plan (or vice versa) is a structural mismatch. */
    out->key.scope = (PlanScope)stored_scope;
    if (out->key.scope != key.scope) {
        free(data);
        fclose(f);
        return false;
    }

    title_buf[MAX_TITLE_LEN - 1] = '\0';
    snprintf(out->title, MAX_TITLE_LEN, "%s", title_buf);

    /* content_len is attacker-controlled. Require it to match exactly the
     * bytes still present in the file; otherwise a forged large value would
     * trigger an oversized malloc and a short fread would leave uninitialized
     * heap that later gets printed (information disclosure). */
    long content_pos = ftell(f);
    size_t remaining = (content_pos >= 0 && file_size >= content_pos)
                           ? (size_t)(file_size - content_pos)
                           : 0;
    if (content_len != remaining) {
        free(data);
        fclose(f);
        return false;
    }

    if (content_len > 0) {
        out->content = (char *)malloc((size_t)content_len + 1);
        if (!out->content) {
            free(data);
            fclose(f);
            return false;
        }
        if (fread(out->content, 1, content_len, f) != content_len) {
            free(out->content);
            out->content = NULL;
            free(data);
            fclose(f);
            return false;
        }
        out->content[content_len] = '\0';
        out->content_len = content_len;
    }

    free(data);
    fclose(f);
    return true;
}

bool plan_key_exists(PlanKey key) {
    char path[256];
    file_path(key, path, sizeof(path));
    return access(path, F_OK) == 0;
}

int list_day_plans_in_range(Date start, Date end, Date *out, size_t max_count) {
    int count = 0;
    Date current = start;

    while (date_cmp(current, end) <= 0 && (size_t)count < max_count) {
        PlanKey key = {SCOPE_DAY, current};
        if (plan_key_exists(key)) {
            out[count] = current;
            count++;
        }
        current = add_days(current, 1);
    }

    return count;
}
