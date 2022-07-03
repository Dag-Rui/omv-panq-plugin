struct ServiceParameters {
  double interval;
  
  u_int32_t fan_low_rpm;
  u_int32_t fan_high_rpm;

  double temp_low;
  double temp_high;
  double cpu_temp_low;
  double cpu_temp_high;
};

void service_start(struct ServiceParameters *params);
