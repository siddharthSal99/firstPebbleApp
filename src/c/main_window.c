#include <pebble.h>
#include "main_window.h"
#include "src/c/error_window.h"


Window *mainWindow;
MenuLayer *mainMenuLayer;
City cities[5];
City currentCityToWrite;

const char *conditions[] = {
"CLEAR_DAY",
"CLEAR_NIGHT",
"WINDY",
"COLD",
"PARTLY_CLOUDY_DAY",
"PARTLY_CLOUDY_NIGHT",
"HAZE",
"CLOUD",
"RAIN",
"SNOW",
"HAIL",
"CLOUDY",
"STORM",
"NA"
};


uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
	return 2;
}
 
uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	switch(section_index){
		case 0:
			return get_amount_of_cities();
		case 1:
			return 1;
		default:
			return 0;
	}
 	
}

uint16_t get_amount_of_cities(){
	uint16_t amount = 0;
	for(int i = 0; i < 5; i++){
		if(cities[i].exists){
			amount++;
		} else {
			break;
		}
	}
	return amount;
}
 
int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}
 
void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
   switch(section_index){
		 case 0:
		 	menu_cell_basic_header_draw(ctx, cell_layer, "Cities");
		 	break;
		 case 1:
		 	menu_cell_basic_header_draw(ctx, cell_layer, "Other");
		 	break;
	 }
}
 
void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
   switch(cell_index->section){
		 case 0:
		 menu_cell_basic_draw(ctx, cell_layer, cities[cell_index->row].name[0], cities[cell_index->row].subtitle[0], NULL);
// 		 	switch(cell_index->row){
// 				case 0:
// 					menu_cell_basic_draw(ctx, cell_layer, "Atlanta", "47", NULL);
// 				break;
// 				case 1:
					
// 				break;
// 			}
		 break;
		 case 1:
		 	menu_cell_basic_draw(ctx, cell_layer, "Add City", NULL, NULL);
		 break;
	 }   
}

char *get_readable_dictation_status(DictationSessionStatus status){
    switch(status){
        case DictationSessionStatusSuccess:
            return "Success";
        case DictationSessionStatusFailureTranscriptionRejected:
            return "User rejected success";
        case DictationSessionStatusFailureTranscriptionRejectedWithError:
            return "User rejected error";
        case DictationSessionStatusFailureSystemAborted:
            return "Too many errors, UI gave up";
        case DictationSessionStatusFailureNoSpeechDetected:
            return "No speech, UI exited";
        case DictationSessionStatusFailureConnectivityError:
            return "No BT/internet connection";
        case DictationSessionStatusFailureDisabled:
            return "Voice dictation disabled";
        case DictationSessionStatusFailureInternalError:
            return "Internal error";
        case DictationSessionStatusFailureRecognizerError:
            return "Failed to transcribe speech";
    }
    return "Unknown";
}

void send_weather_request(char *city){
	DictionaryIterator *iter;
		app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		APP_LOG(APP_LOG_LEVEL_ERROR, "iter is null, refucing to send");
		return;
	}
	
	dict_write_cstring(iter, MESSAGE_KEY_getWeather, city);
	dict_write_end(iter);
	app_message_outbox_send();
}
 
void dictation_session_callback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context){
	switch(status){
		case DictationSessionStatusSuccess:
			send_weather_request(transcription);
		break;
		case DictationSessionStatusFailureTranscriptionRejected:
		break;
		default:
			error_window_set_error(get_readable_dictation_status(status));
			error_window_show();
		break;
	}
}

void launch_dictation(){
	static char lastText[40];
	DictationSession *session = dictation_session_create(sizeof(lastText), dictation_session_callback, NULL);
	if (session == NULL){
		APP_LOG(APP_LOG_LEVEL_ERROR, "Session is null");
		return;
	}
	dictation_session_start(session);
}

int main_window_save_cities(){
	int value = 0;
	for(int i = 0; i < 5; i++) {
		value += persist_write_data(i, &cities[i], sizeof(City));
	}
	return value;
}

int main_window_load_cities(){
	int value = 0;
	for(int i = 0; i < 5; i++) {
		value += persist_read_data(i, &cities[i], sizeof(City));
	}
	return value;
}

void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	switch(cell_index->section){
		case 0:
			for(int i = 0; i < 4; i++) {
				City nonExistingCity = {
					.exists = false
				};
				City tempCity = cities[i + 1];
				cities[i] = tempCity;
				cities[i + 1] = nonExistingCity;
			}
		menu_layer_reload_data(mainMenuLayer);
		break;
		case 1:
			launch_dictation();
		break;
	}
	
}

void process_tuple(Tuple *t){
    uint32_t key = t->key;
    int value = t->value->int32;
	
		if (key == MESSAGE_KEY_icon){
			currentCityToWrite.condition = value;
		}
		else if (key == MESSAGE_KEY_temperature) {
			currentCityToWrite.temperature = value;
		}
		else if (key == MESSAGE_KEY_cityid) {
			currentCityToWrite.id = value;
		}
		else if(key == MESSAGE_KEY_cityname){
			strncpy(currentCityToWrite.name[0], t->value->cstring, sizeof(currentCityToWrite.name[0]));
		}
    APP_LOG(APP_LOG_LEVEL_INFO, "Got key %d with value %d", (int) key, value);
}
 
void message_inbox(DictionaryIterator *iter, void *context){	
    Tuple *t = dict_read_first(iter);
    if(t){
        process_tuple(t);
    }
    while(t != NULL){
        t = dict_read_next(iter);
        if(t){
            process_tuple(t);
        }
    }
	snprintf(currentCityToWrite.subtitle[0], sizeof(currentCityToWrite.subtitle[0]), "%d°, %s", currentCityToWrite.temperature, conditions[currentCityToWrite.condition]);
	
		for(int i = 0; i < 5; i++){
			if(cities[i].id == currentCityToWrite.id) {
				currentCityToWrite.exists = true;
				cities[i] = currentCityToWrite;
				break;
			}
		}
		if(!currentCityToWrite.exists){
			for(int i = 0; i < 5; i++){
				if(!cities[i].exists) {
					currentCityToWrite.exists = true;
					cities[i] = currentCityToWrite;
					break;
				}
		}
		}
	
	
	
	menu_layer_reload_data(mainMenuLayer);
	vibes_double_pulse();		
	currentCityToWrite.exists = false;																																																																																
																																																																																		
}
 
void message_inbox_dropped(AppMessageResult reason, void *context){
    APP_LOG(APP_LOG_LEVEL_INFO, "Message dropped, reason %d.", reason);
}

int currentlyRefreshing = 0;
void refresh_weather(){
	if(currentlyRefreshing < 5){
		if(cities[currentlyRefreshing].exists) {
			send_weather_request(cities[currentlyRefreshing].name[0]);
			currentlyRefreshing++;
			app_timer_register(500, refresh_weather, NULL);
		} else {
			app_timer_register(900000, refresh_weather, NULL);
		}
	}
}
 
void setup_menu_layer(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
 
    mainMenuLayer = menu_layer_create(GRect(0, 0, 144, 168));
    menu_layer_set_callbacks(mainMenuLayer, NULL, (MenuLayerCallbacks){
        .get_num_sections = menu_get_num_sections_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .get_header_height = menu_get_header_height_callback,
        .draw_header = menu_draw_header_callback,
        .draw_row = menu_draw_row_callback,
        .select_click = menu_select_callback,
    });
 
    menu_layer_set_click_config_onto_window(mainMenuLayer, window);
	
		
 
    layer_add_child(window_layer, menu_layer_get_layer(mainMenuLayer));
}


void main_window_load(Window *window){
	setup_menu_layer(window);
	app_message_register_inbox_received(message_inbox);
	app_message_register_inbox_dropped(message_inbox_dropped);
	app_message_open(256,256);
	
	app_timer_register(500, refresh_weather, NULL);
}

void main_window_unload(Window *window) {
	menu_layer_destroy(mainMenuLayer);
}

void main_window_create() {
	mainWindow = window_create();
	window_set_window_handlers(mainWindow, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
}

void main_window_destroy() {
	window_destroy(mainWindow);
}

Window *main_window_get_window() {
	return mainWindow;
}

