#include "storage.h"
#include "plan.h"
#include "date_utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  %s ... ", name); tests_run++; } while(0)
#define PASS() printf("ok\n")
#define FAIL(msg) do { printf("FAIL (%s)\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_EQ(a, b, msg) do { if ((a) != (b)) { FAIL(msg); return; } } while(0)
#define ASSERT_STR_EQ(a, b, msg) do { if (strcmp(a, b) != 0) { FAIL(msg); return; } } while(0)

static void test_init(void) {
    TEST("storage_init");
    ASSERT(storage_init(), "init should succeed");
    PASS();
}

static void test_save_load_day(void) {
    TEST("save and load day plan roundtrip");
    Plan p;
    plan_clear(&p);
    p.key = plan_key_normalize(SCOPE_DAY, (Date){2026, 7, 4});
    snprintf(p.title, MAX_TITLE_LEN, "Independence Day");
    p.content = strdup("BBQ and fireworks.");
    p.content_len = strlen(p.content);

    ASSERT(save_plan(&p), "save");
    plan_free(&p);

    Plan loaded;
    plan_clear(&loaded);
    PlanKey key = plan_key_normalize(SCOPE_DAY, (Date){2026, 7, 4});
    ASSERT(load_plan(key, &loaded), "load");

    ASSERT_EQ(loaded.key.scope, SCOPE_DAY, "scope");
    ASSERT_EQ(loaded.key.anchor.year, 2026, "year");
    ASSERT_EQ(loaded.key.anchor.month, 7, "month");
    ASSERT_EQ(loaded.key.anchor.day, 4, "day");
    ASSERT_STR_EQ(loaded.title, "Independence Day", "title");
    ASSERT_STR_EQ(loaded.content, "BBQ and fireworks.", "content");

    plan_free(&loaded);
    PASS();
}

static void test_save_load_week(void) {
    TEST("save and load week plan roundtrip");
    Plan p;
    plan_clear(&p);
    p.key = plan_key_normalize(SCOPE_WEEK, (Date){2026, 6, 19});
    snprintf(p.title, MAX_TITLE_LEN, "Build Week");
    p.content = strdup("Ship the planner app.");
    p.content_len = strlen(p.content);

    ASSERT(save_plan(&p), "save");
    plan_free(&p);

    Plan loaded;
    plan_clear(&loaded);
    PlanKey key = plan_key_normalize(SCOPE_WEEK, (Date){2026, 6, 19});
    ASSERT(load_plan(key, &loaded), "load");

    ASSERT_EQ(loaded.key.scope, SCOPE_WEEK, "scope");
    ASSERT_EQ(loaded.key.anchor.day, 14, "sunday anchor");
    ASSERT_STR_EQ(loaded.title, "Build Week", "title");
    ASSERT_STR_EQ(loaded.content, "Ship the planner app.", "content");

    plan_free(&loaded);
    PASS();
}

static void test_save_load_month(void) {
    TEST("save and load month plan roundtrip");
    Plan p;
    plan_clear(&p);
    p.key = plan_key_normalize(SCOPE_MONTH, (Date){2026, 6, 15});
    snprintf(p.title, MAX_TITLE_LEN, "June Goals");
    p.content = strdup("Finish C course.");
    p.content_len = strlen(p.content);

    ASSERT(save_plan(&p), "save");
    plan_free(&p);

    Plan loaded;
    plan_clear(&loaded);
    PlanKey key = plan_key_normalize(SCOPE_MONTH, (Date){2026, 6, 15});
    ASSERT(load_plan(key, &loaded), "load");

    ASSERT_EQ(loaded.key.scope, SCOPE_MONTH, "scope");
    ASSERT_EQ(loaded.key.anchor.month, 6, "month");
    ASSERT_EQ(loaded.key.anchor.day, 1, "day normalized");
    ASSERT_STR_EQ(loaded.title, "June Goals", "title");
    ASSERT_STR_EQ(loaded.content, "Finish C course.", "content");

    plan_free(&loaded);
    PASS();
}

static void test_save_load_year(void) {
    TEST("save and load year plan roundtrip");
    Plan p;
    plan_clear(&p);
    p.key = plan_key_normalize(SCOPE_YEAR, (Date){2026, 6, 15});
    snprintf(p.title, MAX_TITLE_LEN, "2026 Vision");
    p.content = strdup("Learn Go and C.");
    p.content_len = strlen(p.content);

    ASSERT(save_plan(&p), "save");
    plan_free(&p);

    Plan loaded;
    plan_clear(&loaded);
    PlanKey key = plan_key_normalize(SCOPE_YEAR, (Date){2026, 6, 15});
    ASSERT(load_plan(key, &loaded), "load");

    ASSERT_EQ(loaded.key.scope, SCOPE_YEAR, "scope");
    ASSERT_EQ(loaded.key.anchor.year, 2026, "year");
    ASSERT_EQ(loaded.key.anchor.month, 1, "month normalized");
    ASSERT_STR_EQ(loaded.title, "2026 Vision", "title");
    ASSERT_STR_EQ(loaded.content, "Learn Go and C.", "content");

    plan_free(&loaded);
    PASS();
}

static void test_plan_exists(void) {
    TEST("plan_key_exists");
    PlanKey day_key = plan_key_normalize(SCOPE_DAY, (Date){2026, 7, 4});
    ASSERT(plan_key_exists(day_key), "day exists");
    PlanKey missing = plan_key_normalize(SCOPE_DAY, (Date){2026, 1, 1});
    ASSERT(!plan_key_exists(missing), "not exists");
    PlanKey week_key = plan_key_normalize(SCOPE_WEEK, (Date){2026, 6, 19});
    ASSERT(plan_key_exists(week_key), "week exists");
    PASS();
}

static void test_list_day_plans(void) {
    TEST("list_day_plans_in_range");
    Date start = {2026, 7, 1};
    Date end = {2026, 7, 31};
    Date dates[31];
    int count = list_day_plans_in_range(start, end, dates, 31);
    ASSERT(count >= 1, "should find at least 1");
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (dates[i].day == 4) found = 1;
    }
    ASSERT(found, "should find July 4");
    PASS();
}

static void test_checksum_corruption(void) {
    TEST("checksum rejects corruption");
    FILE *f = fopen("data/d-2026-07-04.bobip", "r+b");
    ASSERT(f, "open");
    fseek(f, 100, SEEK_SET);
    fputc(0xFF, f);
    fclose(f);

    Plan loaded;
    plan_clear(&loaded);
    PlanKey key = plan_key_normalize(SCOPE_DAY, (Date){2026, 7, 4});
    ASSERT(!load_plan(key, &loaded), "rejected");
    PASS();
}

static void write_raw_plan(const char *path, PlanScope scope, Date date,
                           const char *title, uint32_t content_len,
                           const void *content, size_t content_nbytes) {
    uint8_t data[1 + 12 + MAX_TITLE_LEN + 4 + 4096];
    size_t pos = 0;
    data[pos++] = (uint8_t)scope;
    memcpy(data + pos, &date.year, sizeof(int));  pos += sizeof(int);
    memcpy(data + pos, &date.month, sizeof(int)); pos += sizeof(int);
    memcpy(data + pos, &date.day, sizeof(int));   pos += sizeof(int);

    char title_buf[MAX_TITLE_LEN];
    memset(title_buf, 0, sizeof(title_buf));
    snprintf(title_buf, sizeof(title_buf), "%s", title ? title : "");
    memcpy(data + pos, title_buf, MAX_TITLE_LEN); pos += MAX_TITLE_LEN;

    memcpy(data + pos, &content_len, sizeof(content_len)); pos += sizeof(content_len);
    if (content && content_nbytes > 0) {
        memcpy(data + pos, content, content_nbytes);
        pos += content_nbytes;
    }

    uint8_t sum = 0;
    for (size_t k = 0; k < pos; k++) sum ^= data[k];

    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint32_t magic = 0x504C414E;
    uint8_t version = 1;
    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&version, sizeof(version), 1, f);
    fputc((int)sum, f);
    fwrite(data, 1, pos, f);
    fclose(f);
}

static void test_forged_content_len(void) {
    TEST("rejects forged content_len");
    Date d = {2026, 8, 15};
    const char actual[] = "hi";
    write_raw_plan("data/d-2026-08-15.bobip", SCOPE_DAY, d, "Title",
                   0x10000u, actual, sizeof(actual) - 1);

    Plan loaded;
    plan_clear(&loaded);
    ASSERT(!load_plan(plan_key_normalize(SCOPE_DAY, d), &loaded),
           "forged content_len must be rejected");
    plan_free(&loaded);
    PASS();
}

static void test_wrong_scope_rejected(void) {
    TEST("rejects wrong scope");
    Date d = {2026, 8, 16};
    const char body[] = "day content";
    write_raw_plan("data/d-2026-08-16.bobip", SCOPE_DAY, d, "Day",
                   (uint32_t)(sizeof(body) - 1), body, sizeof(body) - 1);

    Plan loaded;
    plan_clear(&loaded);
    PlanKey wrong = plan_key_normalize(SCOPE_WEEK, d);
    ASSERT(!load_plan(wrong, &loaded), "wrong scope must be rejected");
    plan_free(&loaded);
    PASS();
}

static void test_bogus_date_rejected(void) {
    TEST("rejects out-of-range date");
    Date stored = {2026, 0, 15};
    write_raw_plan("data/d-2026-08-17.bobip", SCOPE_DAY, stored, "T", 0u, NULL, 0);

    Date lookup = {2026, 8, 17};
    Plan loaded;
    plan_clear(&loaded);
    ASSERT(!load_plan(plan_key_normalize(SCOPE_DAY, lookup), &loaded),
           "bogus stored date must be rejected");
    plan_free(&loaded);
    PASS();
}

static void test_raw_roundtrip(void) {
    TEST("hand-forged honest record loads");
    Date d = {2026, 8, 18};
    const char body[] = "valid body";
    write_raw_plan("data/d-2026-08-18.bobip", SCOPE_DAY, d, "Hello",
                   (uint32_t)(sizeof(body) - 1), body, sizeof(body) - 1);

    Plan loaded;
    plan_clear(&loaded);
    ASSERT(load_plan(plan_key_normalize(SCOPE_DAY, d), &loaded),
           "consistent record should load");
    ASSERT_STR_EQ(loaded.title, "Hello", "title");
    ASSERT_STR_EQ(loaded.content, "valid body", "content");
    plan_free(&loaded);
    PASS();
}

static void test_cleanup(void) {
    TEST("cleanup");
    unlink("data/d-2026-07-04.bobip");
    unlink("data/w-2026-06-14.bobip");
    unlink("data/m-2026-06.bobip");
    unlink("data/y-2026.bobip");
    unlink("data/d-2026-08-15.bobip");
    unlink("data/d-2026-08-16.bobip");
    unlink("data/d-2026-08-17.bobip");
    unlink("data/d-2026-08-18.bobip");
    PASS();
}

int main(void) {
    printf("storage tests:\n");
    test_init();
    test_save_load_day();
    test_save_load_week();
    test_save_load_month();
    test_save_load_year();
    test_plan_exists();
    test_list_day_plans();
    test_checksum_corruption();
    test_forged_content_len();
    test_wrong_scope_rejected();
    test_bogus_date_rejected();
    test_raw_roundtrip();
    test_cleanup();

    printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
