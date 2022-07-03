

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include "fan_service.h"
#include "it8528.h"
#include "utils.h"

void set_fan_speed(double);
int8_t read_cpu_temp(double *);
double calculate_fan_speed(struct ServiceParameters *, double, double);
void print_startup_message(struct ServiceParameters *);
void assert_params(struct ServiceParameters *);

u_int32_t fan_diff;
double cpu_temp_diff;
double system_temp_diff;

bool running = false;

void service_start(struct ServiceParameters *params)
{
  if (running)
  {
    fprintf(stderr, "Already running!\n");
    return;
  }

  assert_params(params);

  fan_diff = params->fan_high_rpm - params->fan_low_rpm;
  cpu_temp_diff = params->cpu_temp_high - params->cpu_temp_low;
  system_temp_diff = params->temp_high - params->temp_low;

  print_startup_message(params);

  struct timespec sleep_ts = {.tv_sec = params->interval, .tv_nsec = 0};

  running = true;

  while (running)
  {
    if (!ensure_it8528())
    {
      fprintf(stderr, "Error checking it8528!\n");
      continue;
    }

    double temperature = 0;
    if (it8528_get_temperature(0, &temperature) != 0)
    {
      fprintf(stderr, "Can't get the temperature!\n");
      continue;
    }

    double cpu_temperature = 0;
    if (read_cpu_temp(&cpu_temperature) != 0)
    {
      fprintf(stderr, "Can't read cpu temperature!\n");
      continue;
    }

    double speed = calculate_fan_speed(params, temperature, cpu_temperature);

    set_fan_speed(speed);

    nanosleep(&sleep_ts, &sleep_ts);
  }
}

double calculate_fan_speed(struct ServiceParameters *params, double temperature, double cpu_temperature)
{
  double cpu_temp_ratio = (cpu_temperature - params->cpu_temp_low) / (cpu_temp_diff);
  double system_temp_ratio = (temperature - params->temp_low) / (system_temp_diff);

  double fan_speed_rpm;

  if (cpu_temp_ratio > system_temp_ratio)
  {
    fan_speed_rpm = params->fan_low_rpm + fan_diff * cpu_temp_ratio;
  }
  else
  {
    fan_speed_rpm = params->fan_low_rpm + fan_diff * system_temp_ratio;
  }

  if (fan_speed_rpm > params->fan_high_rpm)
  {
    return params->fan_high_rpm;
  }
  if (fan_speed_rpm < params->fan_low_rpm)
  {
    return params->fan_low_rpm;
  }

  return fan_speed_rpm;
}

void set_fan_speed(double speed_rpm)
{
  // Note: the formula to convert from fan speed to RPM is approximately:
  //       rpm = 7 * fan_speed - 17

  // Convert from fan speed percentage to fan speed
  double fan_speed = speed_rpm;
  fan_speed += 17;
  fan_speed /= 7;

  if (it8528_set_fan_speed(0, (u_int8_t)fan_speed) != 0)
  {
    fprintf(stderr, "Can't set fan speed!\n");
  }
}

int8_t read_cpu_temp(double *temperature_value)
{
  FILE *fp;
  fp = fopen("/sys/devices/virtual/thermal/thermal_zone0/temp", "r");
  int temperature;

  int ret = fscanf(fp, "%d", &temperature);
  if (ret == 0)
  {
    printf("Nothing read from temperature file\n");
    return ret;
  }
  ret = fclose(fp);

  *temperature_value = temperature / 1000.;

  return ret;
}

void assert_params(struct ServiceParameters *params)
{
  if (params->interval == 0)
  {
    printf("interval is 0\n");
    exit(EXIT_FAILURE);
  }
  if (params->fan_low_rpm >= params->fan_high_rpm)
  {
    printf("fan_low_rpm is higher or equal to fan_high_rpm\n");
    exit(EXIT_FAILURE);
  }
  if (params->temp_low >= params->temp_high)
  {
    printf("temp_low is higher or equal to temp_high\n");
    exit(EXIT_FAILURE);
  }
  if (params->cpu_temp_low >= params->cpu_temp_high)
  {
    printf("cpu_temp_low is higher or equal to cpu_temp_high\n");
    exit(EXIT_FAILURE);
  }
}

void print_startup_message(struct ServiceParameters *params)
{
  printf("Starting service with parameters:\n");
  printf("interval          -> %lf\n", params->interval);
  printf("fan_low_rpm       -> %d\n", params->fan_low_rpm);
  printf("fan_high_rpm      -> %d\n", params->fan_high_rpm);
  printf("temp_low          -> %lf\n", params->temp_low);
  printf("temp_high         -> %lf\n", params->temp_high);
  printf("cpu_temp_low      -> %lf\n", params->cpu_temp_low);
  printf("cpu_temp_high     -> %lf\n", params->cpu_temp_high);
}