#ifndef ANDROID_ADAPTER_H
#define ANDROID_ADAPTER_H

#include<string.h>

#define PROPERTY_VALUE_MAX 32

#define property_get(name, value_str, default_value) \
  do { \
    strncpy(value_str, default_value, PROPERTY_VALUE_MAX); \
  } while (0)

#endif
