#pragma once


#include <time.h>

#include "esp_err.h"


esp_err_t gips_rtc_init(void);
esp_err_t gips_rtc_read(struct tm *time);
esp_err_t gips_rtc_write(const struct tm *time);
