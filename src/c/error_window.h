#pragma once
#include <pebble.h>

void error_window_create();

void error_window_destroy();

Window *error_window_get_window();

void error_window_show();

void error_window_set_error(char *errorText);
