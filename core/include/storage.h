#ifndef STORAGE_H
#define STORAGE_H

#include "plan.h"
#include <stdbool.h>
#include <stddef.h>

#define DATA_DIR "data"

bool storage_init(void);
bool save_plan(const Plan *plan);
bool load_plan(PlanKey key, Plan *out);
bool plan_key_exists(PlanKey key);
int list_day_plans_in_range(Date start, Date end, Date *out, size_t max_count);

#endif
