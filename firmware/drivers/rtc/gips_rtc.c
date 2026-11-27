#include "gips_rtc.h"

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "gips_board.h"


static const int k_timeout_ms = 100;
static const uint8_t k_status_register = 0x0f;
static const uint8_t k_osf = 0x80;
static const uint8_t k_days[] = {
  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
};


static i2c_master_bus_handle_t _bus;
static i2c_master_dev_handle_t _device;


static bool leap_year(int year);
static bool valid_time(const struct tm *time);
static int weekday(const struct tm *time);
static int yearday(const struct tm *time);
static uint8_t to_bcd(unsigned value);
static bool from_bcd(uint8_t value, uint8_t *result);
static esp_err_t read_registers(uint8_t address, uint8_t *data, size_t size);


static bool leap_year(int year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}


static bool valid_time(const struct tm *time)
{
  // struct tm counts years from 1900 and months from zero.
  if (time->tm_year < 100 || time->tm_year > 199 ||
      time->tm_mon < 0 || time->tm_mon > 11 ||
      time->tm_hour < 0 || time->tm_hour > 23 ||
      time->tm_min < 0 || time->tm_min > 59 ||
      time->tm_sec < 0 || time->tm_sec > 59)
  {
    return false;
  }
  int days = k_days[time->tm_mon];
  if (time->tm_mon == 1 && leap_year(time->tm_year + 1900))
  {
    ++days;
  }
  return time->tm_mday >= 1 && time->tm_mday <= days;
}


static int weekday(const struct tm *time)
{
  // Gregorian weekday calculation treats January and February as prior year.
  static const int k_month_offsets[] = {
    0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4,
  };
  int year = time->tm_year + 1900 - (time->tm_mon < 2);
  return (year + year / 4 - year / 100 + year / 400 +
          k_month_offsets[time->tm_mon] + time->tm_mday) % 7;
}


static int yearday(const struct tm *time)
{
  int day = time->tm_mday - 1;
  for (int month = 0; month < time->tm_mon; ++month)
  {
    day += k_days[month];
  }
  // Leap day contributes only after February.
  return day + (time->tm_mon > 1 && leap_year(time->tm_year + 1900));
}


static uint8_t to_bcd(unsigned value)
{
  return (uint8_t)((value / 10) * 16 + value % 10);
}


static bool from_bcd(uint8_t value, uint8_t *result)
{
  if ((value & 0x0f) > 9 || (value >> 4) > 9)
  {
    return false;
  }
  *result = (value >> 4) * 10 + (value & 0x0f);
  return true;
}


static esp_err_t read_registers(uint8_t address, uint8_t *data, size_t size)
{
  return i2c_master_transmit_receive(_device, &address, 1, data, size,
                                     k_timeout_ms);
}


esp_err_t gips_rtc_init(void)
{
  if (_device != NULL)
  {
    return ESP_OK;
  }
  const i2c_master_bus_config_t bus_config = {
    .i2c_port = -1,
    .sda_io_num = GIPS_PIN_RTC_SDA,
    .scl_io_num = GIPS_PIN_RTC_SCL,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
  };
  esp_err_t err = i2c_new_master_bus(&bus_config, &_bus);
  if (err != ESP_OK)
  {
    return err;
  }
  const i2c_device_config_t device_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x68,
    .scl_speed_hz = 100000,
  };
  err = i2c_master_bus_add_device(_bus, &device_config, &_device);
  if (err != ESP_OK)
  {
    i2c_del_master_bus(_bus);
    _bus = NULL;
  }
  return err;
}


esp_err_t gips_rtc_read(struct tm *time)
{
  if (time == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }
  if (_device == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }
  uint8_t status;
  esp_err_t err = read_registers(k_status_register, &status, 1);
  if (err != ESP_OK)
  {
    return err;
  }
  // OSF means the oscillator stopped and the stored time is unreliable.
  if (status & k_osf)
  {
    return ESP_ERR_INVALID_STATE;
  }
  uint8_t data[7];
  err = read_registers(0, data, sizeof(data));
  if (err != ESP_OK)
  {
    return err;
  }
  // Mask control and reserved bits before decoding the BCD fields.
  if (data[5] & 0x80)
  {
    return ESP_ERR_INVALID_RESPONSE;  // Century bit is outside 2000-2099.
  }
  struct tm result = {0};
  uint8_t second, minute, hour, day, month, year;
  if (!from_bcd(data[0] & 0x7f, &second) ||
      !from_bcd(data[1] & 0x7f, &minute) ||
      !from_bcd(data[2] & ((data[2] & 0x40) ? 0x1f : 0x3f), &hour) ||
      !from_bcd(data[4] & 0x3f, &day) ||
      !from_bcd(data[5] & 0x1f, &month) ||
      !from_bcd(data[6], &year))
  {
    return ESP_ERR_INVALID_RESPONSE;
  }
  if (data[2] & 0x40)
  {
    if (hour < 1 || hour > 12)
    {
      return ESP_ERR_INVALID_RESPONSE;
    }
    // In 12-hour mode, 12 AM is 0 and 12 PM is 12.
    hour = hour % 12 + ((data[2] & 0x20) ? 12 : 0);
  }
  result.tm_sec = second;
  result.tm_min = minute;
  result.tm_hour = hour;
  result.tm_mday = day;
  result.tm_mon = (int)month - 1;
  result.tm_year = 100 + year;
  if (!valid_time(&result))
  {
    return ESP_ERR_INVALID_RESPONSE;
  }
  result.tm_wday = weekday(&result);
  result.tm_yday = yearday(&result);
  result.tm_isdst = -1;
  *time = result;
  return ESP_OK;
}


esp_err_t gips_rtc_write(const struct tm *time)
{
  if (time == NULL || !valid_time(time))
  {
    return ESP_ERR_INVALID_ARG;
  }
  if (_device == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }
  // DS3231M weekday is 1-7; calculate it from the validated date.
  uint8_t data[] = {0, to_bcd(time->tm_sec), to_bcd(time->tm_min),
                    to_bcd(time->tm_hour), weekday(time) + 1,
                    to_bcd(time->tm_mday), to_bcd(time->tm_mon + 1),
                    to_bcd(time->tm_year - 100)};
  esp_err_t err = i2c_master_transmit(_device, data, sizeof(data),
                                      k_timeout_ms);
  if (err != ESP_OK)
  {
    return err;
  }
  // Enable the oscillator on battery power without changing other controls.
  uint8_t control;
  err = read_registers(0x0e, &control, 1);
  if (err != ESP_OK)
  {
    return err;
  }
  uint8_t enable_oscillator[] = {0x0e, control & 0x7f};
  err = i2c_master_transmit(_device, enable_oscillator,
                            sizeof(enable_oscillator), k_timeout_ms);
  if (err != ESP_OK)
  {
    return err;
  }
  uint8_t status;
  err = read_registers(k_status_register, &status, 1);
  if (err != ESP_OK)
  {
    return err;
  }
  // Alarm flags clear on writing zero; writing one preserves flags that
  // become set between the read and write.
  uint8_t clear_osf[] = {k_status_register, (status & (uint8_t)~k_osf) | 0x03};
  return i2c_master_transmit(_device, clear_osf, sizeof(clear_osf),
                             k_timeout_ms);
}
