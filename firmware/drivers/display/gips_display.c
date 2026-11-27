#include "gips_display.h"

#include <stddef.h>


esp_err_t gips_display_set_frame(const gips_display_frame_t *frame)
{
  if (frame == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }
  return ESP_ERR_NOT_SUPPORTED;
}
