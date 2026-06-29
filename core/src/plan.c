#include "plan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *SCOPE_NAMES[SCOPE_COUNT] = {
    "Day",
    "Week",
    "Month",
    "Year"
};

const char SCOPE_PREFIXES[SCOPE_COUNT] = {
    'd', 'w', 'm', 'y'
};

void plan_clear(Plan *p) {
    if (!p) return;
    p->key.scope = SCOPE_DAY;
    p->key.anchor.year = 0;
    p->key.anchor.month = 0;
    p->key.anchor.day = 0;
    p->title[0] = '\0';
    p->content = NULL;
    p->content_len = 0;
}

void plan_free(Plan *p) {
    if (!p) return;
    free(p->content);
    p->content = NULL;
    p->content_len = 0;
}

PlanKey plan_key_normalize(PlanScope scope, Date d) {
    PlanKey key;
    key.scope = scope;
    switch (scope) {
        case SCOPE_DAY:
            key.anchor = d;
            break;
        case SCOPE_WEEK:
            key.anchor = week_start(d);
            break;
        case SCOPE_MONTH:
            key.anchor = (Date){d.year, d.month, 1};
            break;
        case SCOPE_YEAR:
            key.anchor = (Date){d.year, 1, 1};
            break;
        default:
            key.anchor = d;
            break;
    }
    return key;
}

bool plan_key_valid(PlanKey key) {
    if (key.scope < 0 || key.scope >= SCOPE_COUNT) return false;
    return date_valid(key.anchor);
}

int plan_format_label(PlanKey key, char *out, size_t out_len) {
    char date_str[MAX_DATE_STR_LEN];
    switch (key.scope) {
        case SCOPE_DAY:
            format_date("long", key.anchor, date_str, sizeof(date_str));
            return snprintf(out, out_len, "%s", date_str);
        case SCOPE_WEEK:
            format_date("long", key.anchor, date_str, sizeof(date_str));
            return snprintf(out, out_len, "Week of %s", date_str);
        case SCOPE_MONTH: {
            const char *mn = month_name(key.anchor.month);
            char cap[32];
            if (mn && mn[0]) {
                cap[0] = (char)(mn[0] - 32);
                size_t i = 1;
                while (mn[i] && i < sizeof(cap) - 1) {
                    cap[i] = mn[i];
                    i++;
                }
                cap[i] = '\0';
            } else {
                cap[0] = '\0';
            }
            return snprintf(out, out_len, "%s %d", cap, key.anchor.year);
        }
        case SCOPE_YEAR:
            return snprintf(out, out_len, "%d", key.anchor.year);
        default:
            return snprintf(out, out_len, "Unknown");
    }
}
