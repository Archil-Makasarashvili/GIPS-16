#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "gips_display.h"
#include "gips_rtc.h"


static const char *const k_tag = "gips_checkout";


// Keep requested pixels between CLI calls.
static gips_display_frame_t _frame;
static unsigned _columns;


static int cmd_dsp(int argc, char **argv);
static bool parse_digits(const char *text, size_t length, unsigned *value);
static int cmd_rtc(int argc, char **argv);
static void register_commands(void);


static int cmd_dsp(int argc, char **argv)
{
  if (argc == 2 && strcmp(argv[1], "off") == 0)
  {
    memset(&_frame, 0, sizeof(_frame));
    _columns = 0;
  }
  else if (argc == 1)
  {
    if (_columns < GIPS_DISPLAY_COLUMNS)
    {
      _frame.columns[_columns++] = 1U << (4 - 1);
    }
  }
  else
  {
    printf("Usage: dsp [off]\n");
    return 1;
  }

  esp_err_t err = gips_display_set_frame(&_frame);
  if (err != ESP_OK)
  {
    printf("Display unavailable: %s (requested row 4: %u/%u columns)\n",
           esp_err_to_name(err), _columns, GIPS_DISPLAY_COLUMNS);
    return 1;
  }
  printf("Display row 4: %u/%u columns\n", _columns, GIPS_DISPLAY_COLUMNS);
  return 0;
}


static bool parse_digits(const char *text, size_t length, unsigned *value)
{
  *value = 0;
  for (size_t i = 0; i < length; ++i)
  {
    if (text[i] < '0' || text[i] > '9')
    {
      return false;
    }
    *value = *value * 10 + (unsigned)(text[i] - '0');
  }
  return true;
}


static int cmd_rtc(int argc, char **argv)
{
  struct tm time = {0};
  bool write = argc == 4 && strcmp(argv[1], "set") == 0;
  if (argc != 1 && !write)
  {
    printf("Usage: rtc [set YYYY-MM-DD HH:MM:SS]\n");
    return 1;
  }
  if (write)
  {
    unsigned year, month, day, hour, minute, second;
    const char *date = argv[2];
    const char *clock = argv[3];
    // Fixed-width parsing rejects trailing characters and malformed input.
    if (strlen(date) != 10 || strlen(clock) != 8 || date[4] != '-' ||
        date[7] != '-' || clock[2] != ':' || clock[5] != ':' ||
        !parse_digits(date, 4, &year) || !parse_digits(date + 5, 2, &month) ||
        !parse_digits(date + 8, 2, &day) || !parse_digits(clock, 2, &hour) ||
        !parse_digits(clock + 3, 2, &minute) ||
        !parse_digits(clock + 6, 2, &second))
    {
      printf("Usage: rtc set YYYY-MM-DD HH:MM:SS\n");
      return 1;
    }
    // struct tm uses years since 1900 and zero-based months.
    time.tm_year = (int)year - 1900;
    time.tm_mon = (int)month - 1;
    time.tm_mday = day;
    time.tm_hour = hour;
    time.tm_min = minute;
    time.tm_sec = second;
  }
  esp_err_t err = gips_rtc_init();
  if (err != ESP_OK)
  {
    printf("RTC init failed: %s\n", esp_err_to_name(err));
    return 1;
  }
  err = write ? gips_rtc_write(&time) : gips_rtc_read(&time);
  if (err != ESP_OK)
  {
    if (!write && err == ESP_ERR_INVALID_STATE)
    {
      printf("RTC time is invalid (oscillator stopped); set the time.\n");
    }
    else
    {
      printf("RTC %s failed: %s\n", write ? "set" : "read",
             esp_err_to_name(err));
    }
    return 1;
  }
  printf("RTC: %04d-%02d-%02d %02d:%02d:%02d\n",
         time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
         time.tm_hour, time.tm_min, time.tm_sec);
  return 0;
}


static void register_commands(void)
{
  ESP_ERROR_CHECK(esp_console_register_help_command());
  ESP_ERROR_CHECK(esp_console_cmd_register(&(esp_console_cmd_t){
    .command = "dsp",
    .help = "Add a pixel on row 4; dsp off clears and resets",
    .func = cmd_dsp,
  }));
  ESP_ERROR_CHECK(esp_console_cmd_register(&(esp_console_cmd_t){
    .command = "rtc",
    .help = "Read RTC or set it: rtc set YYYY-MM-DD HH:MM:SS",
    .func = cmd_rtc,
  }));
}


void app_main(void)
{
  esp_console_repl_t *repl = NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  esp_console_dev_usb_serial_jtag_config_t dev_config =
      ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

  repl_config.prompt = "gips> ";
  register_commands();
  ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(
      &dev_config, &repl_config, &repl));
  ESP_LOGI(k_tag, "Type help for commands; display GPIOs are untouched");
  ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
