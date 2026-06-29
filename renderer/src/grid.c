#include "grid.h"
#include "tui.h"
#include "date_utils.h"
#include "plan.h"

#include <stdio.h>
#include <string.h>

const char *VIEW_NAMES[VIEW_COUNT] = {
    "Daily",
    "Weekly",
    "Monthly",
    "Yearly"
};

static const char *DOW_SHORT[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static const char *MONTH_ABBR[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void grid_clear(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

static void print_trunc(const char *s, int max_width) {
    int len = (int)strlen(s);
    if (len <= max_width) {
        printf("%s", s);
        for (int i = len; i < max_width; i++) putchar(' ');
    } else {
        printf("%.*s", max_width - 1, s);
        putchar('>');
    }
}

int days_in_month(int year, int month) {
    int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
        return 29;
    }
    return dim[month - 1];
}

void grid_draw_day(const Plan *plan, TermSize term) {
    char label[MAX_DATE_STR_LEN * 2];
    plan_format_label(plan->key, label, sizeof(label));
    (void)term;

    printf("\n  " ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", label);
    printf("  " ANSI_BOLD ANSI_YELLOW "%s" ANSI_RESET "\n", plan->title);

    if (plan->content && plan->content_len > 0) {
        printf("\n%s\n", plan->content);
    } else {
        printf("\n" ANSI_DIM "(empty plan)" ANSI_RESET "\n");
    }
}

int col_width(TermSize term) {
    int w = (term.cols - 8) / 7;
    if (w < 8) w = 8;
    return w;
}

void grid_draw_week(Plan day_plans[7], int count, const Plan *week_plan, TermSize term) {
    int cw = col_width(term);

    if (week_plan) {
        char label[MAX_DATE_STR_LEN * 2];
        plan_format_label(week_plan->key, label, sizeof(label));
        printf("\n  " ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n", label);
        printf("  " ANSI_BOLD ANSI_YELLOW "%s" ANSI_RESET "\n\n", week_plan->title);
    } else {
        printf("\n  " ANSI_BOLD ANSI_CYAN "Weekly View" ANSI_RESET "\n\n");
    }

    for (int d = 0; d < count; d++) {
        if (d > 0) printf(" ");
        char buf[32];
        int dow = day_of_week(day_plans[d].key.anchor);
        snprintf(buf, sizeof(buf), "%s %02d", DOW_SHORT[dow], day_plans[d].key.anchor.day);
        printf(ANSI_BOLD ANSI_YELLOW "%-*s" ANSI_RESET, cw, buf);
    }
    printf("\n");

    for (int d = 0; d < count; d++) {
        if (d > 0) printf(" ");
        if (day_plans[d].title[0]) {
            printf(ANSI_GREEN);
            print_trunc(day_plans[d].title, cw);
            printf(ANSI_RESET);
        } else {
            printf(ANSI_DIM);
            print_trunc("", cw);
            printf(ANSI_RESET);
        }
    }
    printf("\n");
}

void grid_draw_month(int year, int month, const Date *days, int day_count,
                     const Plan *month_plan, TermSize term) {
    (void)term;

    if (month_plan) {
        char label[MAX_DATE_STR_LEN * 2];
        plan_format_label(month_plan->key, label, sizeof(label));
        printf("\n  " ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n", label);
        printf("  " ANSI_BOLD ANSI_YELLOW "%s" ANSI_RESET "\n\n", month_plan->title);
    } else {
        PlanKey mk = {SCOPE_MONTH, {year, month, 1}};
        char label[MAX_DATE_STR_LEN * 2];
        plan_format_label(mk, label, sizeof(label));
        printf("\n  " ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", label);
    }

    for (int i = 0; i < 7; i++) {
        if (i > 0) printf(" ");
        printf(ANSI_BOLD ANSI_YELLOW "%3s" ANSI_RESET, DOW_SHORT[i]);
    }
    printf("\n");

    Date first = {year, month, 1};
    int start_dow = day_of_week(first);
    int dim = days_in_month(year, month);

    for (int i = 0; i < start_dow; i++) {
        if (i > 0) printf(" ");
        printf("    ");
    }

    for (int day = 1; day <= dim; day++) {
        if ((day + start_dow - 1) % 7 == 0 && day > 1) {
            printf("\n");
        } else if (day > 1) {
            printf(" ");
        }

        int has_plan = 0;
        for (int i = 0; i < day_count; i++) {
            if (days[i].year == year && days[i].month == month && days[i].day == day) {
                has_plan = 1;
                break;
            }
        }

        if (has_plan) {
            printf(ANSI_REVERSE ANSI_GREEN "%3d" ANSI_RESET, day);
        } else {
            printf("%3d", day);
        }
    }
    printf("\n");
}

void grid_draw_year(int year, const bool *months_with_plan,
                    const Plan *year_plan, TermSize term) {
    (void)term;

    if (year_plan) {
        printf("\n  " ANSI_BOLD ANSI_CYAN "%d" ANSI_RESET "\n", year);
        printf("  " ANSI_BOLD ANSI_YELLOW "%s" ANSI_RESET "\n\n", year_plan->title);
    } else {
        printf("\n  " ANSI_BOLD ANSI_CYAN "%d" ANSI_RESET "\n\n", year);
    }

    for (int row = 0; row < 3; row++) {
        printf("  ");
        for (int col = 0; col < 4; col++) {
            int m = row * 4 + col;
            if (col > 0) printf("  ");
            if (months_with_plan && months_with_plan[m]) {
                printf(ANSI_REVERSE ANSI_GREEN "%-4s" ANSI_RESET, MONTH_ABBR[m]);
            } else {
                printf(ANSI_DIM "%-4s" ANSI_RESET, MONTH_ABBR[m]);
            }
        }
        printf("\n");
    }
}
