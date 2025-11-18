#ifndef CLIENT_H
#define CLIENT_H

#include <Arduino.h>

typedef struct {
  String host;
  uint16_t port;
  String path;
} url_t;

int postImage(char *UPLOAD_URL);

#endif