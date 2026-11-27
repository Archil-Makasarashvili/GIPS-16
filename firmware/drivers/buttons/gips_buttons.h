#pragma once


#include <stdint.h>

#include "esp_err.h"


enum
{
  GIPS_BUTTON_MENU = 1u << 0,
  GIPS_BUTTON_MODE = 1u << 1,
  GIPS_BUTTON_SET = 1u << 2,
};


esp_err_t gips_buttons_init(void);
// Returns a mask of pressed buttons once implemented.
esp_err_t gips_buttons_read(uint8_t *pressed);
