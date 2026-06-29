#ifndef PLAN_H
#define PLAN_H

#include "date_utils.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_TITLE_LEN 128

typedef enum {
    SCOPE_DAY,
    SCOPE_WEEK,
    SCOPE_MONTH,
    SCOPE_YEAR,
    SCOPE_COUNT
} PlanScope;

extern const char *SCOPE_NAMES[SCOPE_COUNT];
extern const char SCOPE_PREFIXES[SCOPE_COUNT];

typedef struct {
    PlanScope scope;
    Date anchor;
} PlanKey;

typedef struct {
    PlanKey key;
    char title[MAX_TITLE_LEN];
    char *content;
    size_t content_len;
} Plan;

void plan_clear(Plan *p);
void plan_free(Plan *p);
PlanKey plan_key_normalize(PlanScope scope, Date d);
bool plan_key_valid(PlanKey key);
int plan_format_label(PlanKey key, char *out, size_t out_len);

#endif
