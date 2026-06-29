#include "plan.h"
#include "date_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  %s ... ", name); tests_run++; } while(0)
#define PASS() printf("ok\n")
#define FAIL(msg) do { printf("FAIL (%s)\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_EQ(a, b, msg) do { if ((a) != (b)) { FAIL(msg); return; } } while(0)

static void test_plan_clear(void) {
    TEST("plan_clear");
    Plan p;
    p.key.scope = SCOPE_YEAR;
    p.key.anchor.year = 2026;
    p.title[0] = 'x';
    p.content = (char *)0x1234;
    p.content_len = 99;
    plan_clear(&p);
    ASSERT_EQ(p.key.scope, SCOPE_DAY, "scope reset");
    ASSERT_EQ(p.key.anchor.year, 0, "year zeroed");
    ASSERT_EQ(p.title[0], '\0', "title cleared");
    ASSERT(p.content == NULL, "content null");
    ASSERT_EQ(p.content_len, (size_t)0, "len zero");
    PASS();
}

static void test_plan_free_null(void) {
    TEST("plan_free null content");
    Plan p;
    plan_clear(&p);
    plan_free(&p);
    ASSERT(p.content == NULL, "content null after free");
    PASS();
}

static void test_plan_free_allocated(void) {
    TEST("plan_free allocated");
    Plan p;
    plan_clear(&p);
    p.content = strdup("hello");
    p.content_len = 5;
    plan_free(&p);
    ASSERT(p.content == NULL, "content freed");
    ASSERT_EQ(p.content_len, (size_t)0, "len zero");
    PASS();
}

static void test_normalize_day(void) {
    TEST("normalize day scope");
    Date d = {2026, 6, 28};
    PlanKey key = plan_key_normalize(SCOPE_DAY, d);
    ASSERT_EQ(key.scope, SCOPE_DAY, "scope");
    ASSERT_EQ(key.anchor.year, 2026, "year");
    ASSERT_EQ(key.anchor.month, 6, "month");
    ASSERT_EQ(key.anchor.day, 28, "day");
    PASS();
}

static void test_normalize_week(void) {
    TEST("normalize week scope");
    Date d = {2026, 6, 19};
    PlanKey key = plan_key_normalize(SCOPE_WEEK, d);
    ASSERT_EQ(key.scope, SCOPE_WEEK, "scope");
    ASSERT_EQ(key.anchor.day, 14, "sunday start");
    PASS();
}

static void test_normalize_month(void) {
    TEST("normalize month scope");
    Date d = {2026, 6, 28};
    PlanKey key = plan_key_normalize(SCOPE_MONTH, d);
    ASSERT_EQ(key.scope, SCOPE_MONTH, "scope");
    ASSERT_EQ(key.anchor.year, 2026, "year");
    ASSERT_EQ(key.anchor.month, 6, "month");
    ASSERT_EQ(key.anchor.day, 1, "day normalized to 1");
    PASS();
}

static void test_normalize_year(void) {
    TEST("normalize year scope");
    Date d = {2026, 6, 28};
    PlanKey key = plan_key_normalize(SCOPE_YEAR, d);
    ASSERT_EQ(key.scope, SCOPE_YEAR, "scope");
    ASSERT_EQ(key.anchor.year, 2026, "year");
    ASSERT_EQ(key.anchor.month, 1, "month normalized to 1");
    ASSERT_EQ(key.anchor.day, 1, "day normalized to 1");
    PASS();
}

static void test_key_valid(void) {
    TEST("plan_key_valid");
    PlanKey key = {SCOPE_DAY, {2026, 6, 28}};
    ASSERT(plan_key_valid(key), "valid key");
    PlanKey bad_scope = {SCOPE_COUNT, {2026, 6, 28}};
    ASSERT(!plan_key_valid(bad_scope), "bad scope");
    PlanKey bad_date = {SCOPE_DAY, {2026, 13, 1}};
    ASSERT(!plan_key_valid(bad_date), "bad date");
    PASS();
}

static void test_format_label_day(void) {
    TEST("format label day");
    PlanKey key = {SCOPE_DAY, {2026, 6, 28}};
    char buf[128];
    plan_format_label(key, buf, sizeof(buf));
    ASSERT(strcmp(buf, "June 28, 2026") == 0, "day label");
    PASS();
}

static void test_format_label_week(void) {
    TEST("format label week");
    PlanKey key = {SCOPE_WEEK, {2026, 6, 14}};
    char buf[128];
    plan_format_label(key, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Week of June 14, 2026") == 0, "week label");
    PASS();
}

static void test_format_label_month(void) {
    TEST("format label month");
    PlanKey key = {SCOPE_MONTH, {2026, 6, 1}};
    char buf[128];
    plan_format_label(key, buf, sizeof(buf));
    ASSERT(strcmp(buf, "June 2026") == 0, "month label");
    PASS();
}

static void test_format_label_year(void) {
    TEST("format label year");
    PlanKey key = {SCOPE_YEAR, {2026, 1, 1}};
    char buf[128];
    plan_format_label(key, buf, sizeof(buf));
    ASSERT(strcmp(buf, "2026") == 0, "year label");
    PASS();
}

int main(void) {
    printf("plan tests:\n");
    test_plan_clear();
    test_plan_free_null();
    test_plan_free_allocated();
    test_normalize_day();
    test_normalize_week();
    test_normalize_month();
    test_normalize_year();
    test_key_valid();
    test_format_label_day();
    test_format_label_week();
    test_format_label_month();
    test_format_label_year();

    printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
