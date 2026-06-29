#include "commands.h"
#include "plan.h"
#include "date_utils.h"
#include "storage.h"
#include "input.h"
#include "sanitize.h"
#include "editor.h"
#include "tui.h"
#include "grid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_plan(const Plan *plan, const char *label) {
    char plan_label[MAX_DATE_STR_LEN * 2];
    plan_format_label(plan->key, plan_label, sizeof(plan_label));
    printf("\n" ANSI_BOLD "%s:" ANSI_RESET " %s\n", label, plan_label);
    printf("  " ANSI_BOLD ANSI_YELLOW "Title:" ANSI_RESET " %s\n", plan->title);
    printf("  " ANSI_BOLD ANSI_CYAN "Content:" ANSI_RESET "\n");
    if (plan->content && plan->content_len > 0) {
        printf("%s\n", plan->content);
    } else {
        printf("  " ANSI_DIM "(empty)" ANSI_RESET "\n");
    }
}

static bool plan_interactive(PlanKey key, bool *skipped) {
    char label[MAX_DATE_STR_LEN * 2];
    plan_format_label(key, label, sizeof(label));
    printf("\n--- Planning %s ---\n", label);

    if (plan_key_exists(key)) {
        Plan existing;
        plan_clear(&existing);
        load_plan(key, &existing);
        print_plan(&existing, "Existing plan");
        plan_free(&existing);

        if (!confirm("Overwrite existing plan")) {
            if (skipped) *skipped = true;
            return false;
        }
    }

    printf("Title: ");
    fflush(stdout);
    char title[MAX_TITLE_LEN];
    if (!read_line(title, sizeof(title)) || title[0] == '\0') {
        printf("Cancelled.\n");
        return false;
    }

    printf("Opening editor to write plan...\n");
    char *content = NULL;
    size_t content_len = 0;

    if (!open_editor("", 0, &content, &content_len)) {
        fprintf(stderr, "error: editor failed\n");
        return false;
    }

    Plan plan;
    plan_clear(&plan);
    plan.key = key;
    snprintf(plan.title, MAX_TITLE_LEN, "%s", title);
    plan.content = content;
    plan.content_len = content_len;

    if (!save_plan(&plan)) {
        fprintf(stderr, "error: could not save plan for %s\n", label);
        plan_free(&plan);
        return false;
    }

    printf("Saved plan for %s\n", label);
    plan_free(&plan);
    if (skipped) *skipped = false;
    return true;
}

static bool edit_plan_interactive(PlanKey key) {
    char label[MAX_DATE_STR_LEN * 2];
    plan_format_label(key, label, sizeof(label));

    if (!plan_key_exists(key)) {
        printf("No plan for %s\n", label);
        return false;
    }

    Plan existing;
    plan_clear(&existing);
    if (!load_plan(key, &existing)) {
        printf("Could not load plan for %s\n", label);
        return false;
    }

    printf("Editing plan for %s\n", label);
    printf("Title: %s\n", existing.title);
    printf("Opening editor...\n");

    char *new_content = NULL;
    size_t new_len = 0;
    if (!open_editor(existing.content, existing.content_len,
                     &new_content, &new_len)) {
        fprintf(stderr, "error: editor failed\n");
        plan_free(&existing);
        return false;
    }

    existing.content_len = new_len;
    free(existing.content);
    existing.content = new_content;

    if (!save_plan(&existing)) {
        fprintf(stderr, "error: could not save plan\n");
        plan_free(&existing);
        return false;
    }

    printf("Saved plan for %s\n", label);
    plan_free(&existing);
    return true;
}

static Date prompt_date_default_scope(PlanScope scope, Date def) {
    PlanKey def_key = plan_key_normalize(scope, def);
    char label[MAX_DATE_STR_LEN * 2];
    plan_format_label(def_key, label, sizeof(label));
    printf("Plan for (Enter for %s, or type a date): ", label);
    fflush(stdout);

    char input[128];
    if (!read_line(input, sizeof(input)) || input[0] == '\0') {
        return def;
    }

    Date d;
    if (parse_date(input, &d)) return d;

    printf("Could not parse date. Using %s.\n", label);
    return def;
}

static PlanScope parse_scope_flag(int argc, char *argv[], int start) {
    for (int i = start; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0) return SCOPE_WEEK;
        if (strcmp(argv[i], "-m") == 0) return SCOPE_MONTH;
        if (strcmp(argv[i], "-y") == 0) return SCOPE_YEAR;
    }
    return SCOPE_DAY;
}

static bool parse_date_flag(int argc, char *argv[], int start, Date *out) {
    for (int i = start; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            if (parse_date(argv[i + 1], out)) return true;
        }
    }
    return false;
}

int cmd_np(int argc, char *argv[]) {
    PlanScope scope = parse_scope_flag(argc, argv, 2);

    Date specified;
    bool has_date = parse_date_flag(argc, argv, 2, &specified);

    Date def = (scope == SCOPE_DAY) ? date_tomorrow() : date_today();
    Date target = has_date ? specified : prompt_date_default_scope(scope, def);

    PlanKey key = plan_key_normalize(scope, target);
    plan_interactive(key, NULL);
    return 0;
}

int cmd_ep(int argc, char *argv[]) {
    PlanScope scope = parse_scope_flag(argc, argv, 2);

    Date specified;
    bool has_date = parse_date_flag(argc, argv, 2, &specified);

    Date def = (scope == SCOPE_DAY) ? date_tomorrow() : date_today();
    Date target = has_date ? specified : prompt_date_default_scope(scope, def);

    PlanKey key = plan_key_normalize(scope, target);

    if (!plan_key_exists(key)) {
        char label[MAX_DATE_STR_LEN * 2];
        plan_format_label(key, label, sizeof(label));
        printf("No plan for %s\n", label);
        return 1;
    }

    edit_plan_interactive(key);
    return 0;
}

int cmd_sp(int argc, char *argv[]) {
    Date cursor = date_today();
    ViewMode view = VIEW_DAILY;

    if (argc >= 4 && strcmp(argv[2], "-d") == 0) {
        Date d;
        if (parse_date(argv[3], &d)) {
            cursor = d;
        }
    }

    raw_mode_enable();
    atexit(raw_mode_disable);

    int running = 1;
    while (running) {
        TermSize term = get_term_size();
        grid_clear();

        switch (view) {
            case VIEW_DAILY: {
                Plan plan;
                plan_clear(&plan);
                PlanKey key = plan_key_normalize(SCOPE_DAY, cursor);
                if (load_plan(key, &plan)) {
                    grid_draw_day(&plan, term);
                    plan_free(&plan);
                } else {
                    char label[MAX_DATE_STR_LEN * 2];
                    plan_format_label(key, label, sizeof(label));
                    printf("\n  " ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", label);
                    printf("  " ANSI_DIM "(no plan for this day)" ANSI_RESET "\n");
                }
                break;
            }
            case VIEW_WEEKLY: {
                Date start = week_start(cursor);
                Plan day_plans[7];
                int count = 0;
                for (int i = 0; i < 7; i++) {
                    Date d = add_days(start, i);
                    plan_clear(&day_plans[i]);
                    day_plans[i].key = plan_key_normalize(SCOPE_DAY, d);
                    if (load_plan(day_plans[i].key, &day_plans[i])) {
                        count++;
                    }
                }
                Plan week_plan;
                plan_clear(&week_plan);
                PlanKey week_key = plan_key_normalize(SCOPE_WEEK, cursor);
                bool has_week = load_plan(week_key, &week_plan);

                if (count == 0 && !has_week) {
                    printf("\n  " ANSI_BOLD ANSI_CYAN "Weekly View" ANSI_RESET "\n\n");
                    printf("  " ANSI_DIM "(no plans this week)" ANSI_RESET "\n");
                } else {
                    grid_draw_week(day_plans, 7,
                                   has_week ? &week_plan : NULL, term);
                }
                for (int i = 0; i < 7; i++) plan_free(&day_plans[i]);
                if (has_week) plan_free(&week_plan);
                break;
            }
            case VIEW_MONTHLY: {
                Plan month_plan;
                plan_clear(&month_plan);
                PlanKey month_key = plan_key_normalize(SCOPE_MONTH, cursor);
                bool has_month = load_plan(month_key, &month_plan);

                int dim = days_in_month(cursor.year, cursor.month);
                Date mstart = {cursor.year, cursor.month, 1};
                Date mend = {cursor.year, cursor.month, dim};
                Date plan_dates[31];
                int found = list_day_plans_in_range(mstart, mend, plan_dates, 31);

                grid_draw_month(cursor.year, cursor.month, plan_dates, found,
                                has_month ? &month_plan : NULL, term);
                if (has_month) plan_free(&month_plan);
                break;
            }
            case VIEW_YEARLY: {
                Plan year_plan;
                plan_clear(&year_plan);
                PlanKey year_key = plan_key_normalize(SCOPE_YEAR, cursor);
                bool has_year = load_plan(year_key, &year_plan);

                bool months_with_plan[12] = {false};
                for (int m = 1; m <= 12; m++) {
                    PlanKey mk = plan_key_normalize(SCOPE_MONTH,
                                                    (Date){cursor.year, m, 1});
                    months_with_plan[m - 1] = plan_key_exists(mk);
                }

                grid_draw_year(cursor.year, months_with_plan,
                               has_year ? &year_plan : NULL, term);
                if (has_year) plan_free(&year_plan);
                break;
            }
            default:
                break;
        }

        const char *next_view =
            (view == VIEW_DAILY)   ? " weekly"  :
            (view == VIEW_WEEKLY)  ? " monthly" :
            (view == VIEW_MONTHLY) ? " yearly"  : " daily";

        printf("\n"
               "  " ANSI_DIM "[" ANSI_RESET ANSI_BOLD ANSI_GREEN "i" ANSI_RESET ANSI_DIM "]" ANSI_RESET "%s"
               "  " ANSI_DIM "[" ANSI_RESET ANSI_BOLD ANSI_GREEN "n" ANSI_RESET ANSI_DIM "]next" ANSI_RESET
               "  " ANSI_DIM "[" ANSI_RESET ANSI_BOLD ANSI_GREEN "N" ANSI_RESET ANSI_DIM "]prev" ANSI_RESET
               "  " ANSI_DIM "[" ANSI_RESET ANSI_BOLD ANSI_GREEN "e" ANSI_RESET ANSI_DIM "]edit" ANSI_RESET
               "  " ANSI_DIM "[" ANSI_RESET ANSI_BOLD ANSI_RED "q" ANSI_RESET ANSI_DIM "]quit" ANSI_RESET,
               next_view);
        printf("\n");
        fflush(stdout);

        int key = read_key();
        switch (key) {
            case 'q':
            case KEY_ESC:
                running = 0;
                break;
            case 'i':
                view = (ViewMode)((view + 1) % VIEW_COUNT);
                break;
            case 'n':
                switch (view) {
                    case VIEW_DAILY:   cursor = add_days(cursor, 1); break;
                    case VIEW_WEEKLY:  cursor = add_days(cursor, 7); break;
                    case VIEW_MONTHLY:
                        if (cursor.month == 12) { cursor.year++; cursor.month = 1; }
                        else { cursor.month++; }
                        break;
                    case VIEW_YEARLY:  cursor.year++; break;
                    default: break;
                }
                break;
            case 'N':
                switch (view) {
                    case VIEW_DAILY:   cursor = add_days(cursor, -1); break;
                    case VIEW_WEEKLY:  cursor = add_days(cursor, -7); break;
                    case VIEW_MONTHLY:
                        if (cursor.month == 1) { cursor.year--; cursor.month = 12; }
                        else { cursor.month--; }
                        break;
                    case VIEW_YEARLY:  cursor.year--; break;
                    default: break;
                }
                break;
            case 'e': {
                raw_mode_disable();
                printf("\n");
                PlanScope edit_scope;
                switch (view) {
                    case VIEW_DAILY:   edit_scope = SCOPE_DAY;   break;
                    case VIEW_WEEKLY:  edit_scope = SCOPE_WEEK;  break;
                    case VIEW_MONTHLY: edit_scope = SCOPE_MONTH; break;
                    case VIEW_YEARLY:  edit_scope = SCOPE_YEAR;  break;
                    default:           edit_scope = SCOPE_DAY;   break;
                }
                Date d = prompt_date_default_scope(edit_scope, cursor);
                PlanKey pkey = plan_key_normalize(edit_scope, d);
                if (plan_key_exists(pkey)) edit_plan_interactive(pkey);
                else                        plan_interactive(pkey, NULL);
                raw_mode_enable();
                break;
            }
            case KEY_RIGHT:
                if (view == VIEW_DAILY) cursor = add_days(cursor, 1);
                else if (view == VIEW_WEEKLY) cursor = add_days(cursor, 7);
                break;
            case KEY_LEFT:
                if (view == VIEW_DAILY) cursor = add_days(cursor, -1);
                else if (view == VIEW_WEEKLY) cursor = add_days(cursor, -7);
                break;
            default:
                break;
        }
    }

    raw_mode_disable();
    printf("\n");
    return 0;
}
