#pragma once

// The ESP8266 RTC memory is arranged into blocks of 4 bytes. The access methods read and write 4 bytes at a time,
// so the RTC data structure should be padded to a 4-byte multiple.

#include <Arduino.h>
#include <RTCMemory.h>

typedef struct _RTCDATA {
  uint8_t flags;          // 1 bytes
  uint8_t channel;        // 1 byte,   2 in total
  uint8_t ap_mac[6];      // 6 bytes,  8 in total
  // uint8_t ds1820addr[8];  // 8 bytes, 16 in total
  uint8_t loopsBeforeScan;  // 1 byte, 17 total
} RtcData;

#define DSVALID 0x01
#define APVALID 0x02
#define HADSENT 0x04
