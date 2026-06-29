#ifndef GRID_H
#define GRID_H

#include "tui.h"
#include "plan.h"

typedef enum {
    VIEW_DAILY,
    VIEW_WEEKLY,
    VIEW_MONTHLY,
    VIEW_YEARLY,
    VIEW_COUNT
} ViewMode;

extern const char *VIEW_NAMES[VIEW_COUNT];

void grid_clear(void);
void grid_draw_day(const Plan *plan, TermSize term);
void grid_draw_week(Plan day_plans[7], int count, const Plan *week_plan, TermSize term);
void grid_draw_month(int year, int month, const Date *days, int day_count,
                     const Plan *month_plan, TermSize term);
void grid_draw_year(int year, const bool *months_with_plan,
                    const Plan *year_plan, TermSize term);

int col_width(TermSize term);
int days_in_month(int year, int month);

#endif
