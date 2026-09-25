#pragma once

// Runtime memory guardrails (the RAM counterpart of the boot-time PWR check):
// logs free heap and every task's stack headroom, and warns when either gets
// low. Called periodically from the main loop.
void health_log(void);
