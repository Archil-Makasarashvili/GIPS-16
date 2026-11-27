#pragma once


#include <stdint.h>

#include "esp_err.h"


#define GIPS_DISPLAY_COLUMNS 111U
#define GIPS_DISPLAY_ROWS 7U


// Each column uses bits 0 through 6 for rows 1 through 7.
typedef struct
{
  uint8_t columns[GIPS_DISPLAY_COLUMNS];
} gips_display_frame_t;


// Display output is disabled until the routed PCB and drive sequence are verified.
esp_err_t gips_display_set_frame(const gips_display_frame_t *frame);
