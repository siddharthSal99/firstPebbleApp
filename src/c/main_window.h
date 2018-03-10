#pragma once
#include <pebble.h>
#define MAX_AMOUNT_OF_CITIES 5;
typedef struct City{
	bool exists;
	int condition; //icon
	int temperature;
	int id;
	char name[1][30];
	char subtitle[1][30];
} City;

void main_window_create();

void main_window_destroy();

uint16_t get_amount_of_cities();

Window *main_window_get_window();

int main_window_save_cities();

int main_window_load_cities();
