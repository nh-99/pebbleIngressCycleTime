#include <pebble.h>

Window *my_window;
TextLayer *tl_cycle;
TextLayer *tl_cp;
TextLayer *tl_countdown;
TextLayer *tl_list;
TextLayer *tl_conn_layer;
TextLayer *tl_batt_layer;
TextLayer *tl_realtime;
TextLayer *tl_region_layer;
GBitmap *img_res;
BitmapLayer *bl_res;

static char region[] = "-REGION-";
static uint32_t mydata = 0; //you want ants? That's how you get ants.


#define START_TIME_SEC 0  //checkpoints actually can be done without special modulus - unix epoch is okay
#define CYCLE_SEC 630000
#define CP_SEC 18000
#define BUF_SIZE 30
#define SHOW_CP_NUM 3 // number of checkpoints displayed
#define CP_DATA_SIZE 6 // number of characters per checkpoint

char buffer[6][BUF_SIZE];

void handle_conn(bool connected) {
	if (connected) {
			text_layer_set_text(tl_conn_layer, "BT: OK");
      vibes_short_pulse();
	} else {
			text_layer_set_text(tl_conn_layer, "BT: LOST");
      vibes_long_pulse();
	}
}

void handle_batt(BatteryChargeState charge) {
  static char battstate[BUF_SIZE];
  snprintf(battstate, BUF_SIZE, "%s: %d%%", charge.is_charging?"CHG":"BATT",charge.charge_percent);
  text_layer_set_text(tl_batt_layer, battstate);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

static void update_time(bool fullupdate) {
  int32_t utcoffset;
  utcoffset =  mydata - 2400;
   APP_LOG( APP_LOG_LEVEL_ERROR , "%ld: utcoffset tick",utcoffset*60);
	time_t rt = time(NULL);
  uint32_t t = rt;
	uint32_t countdown = CP_SEC - (t + utcoffset*60)  % CP_SEC ;
	struct tm *tms;
  struct tm *acttime;
  struct tm *curcyclestart;
  
	if(!fullupdate){
		fullupdate = (rt % 3600 == 0);
    //if (utcoffset != -1) { app_message_outbox_send(); };
	}
	
	if(fullupdate){

    uint32_t next = rt + countdown + utcoffset*60;
    uint32_t last = rt - (CYCLE_SEC-countdown)+utcoffset*60;
    curcyclestart = localtime((time_t*) &last);
		uint32_t year = curcyclestart->tm_year+1900; //we have our year
    uint32_t offset = last-(curcyclestart->tm_yday*86400)-(curcyclestart->tm_hour*60*60)-(curcyclestart->tm_min*60)-(curcyclestart->tm_sec);
		uint32_t cycle = (t - offset) / CYCLE_SEC;
    uint32_t cp = (t % CYCLE_SEC) / CP_SEC + 1;
    //APP_LOG( APP_LOG_LEVEL_ERROR , "%ld: utcoffset, %lu: offset, %lu: cycle, %lu: checkpoint, %lu: year", utcoffset, offset, cycle, cp, year);

		snprintf(buffer[0], BUF_SIZE, "%lu.%02lu", year, cycle);
		snprintf(buffer[1], BUF_SIZE, "%02lu/35", cp);
		text_layer_set_text(tl_cycle, buffer[0]);
		text_layer_set_text(tl_cp, buffer[1]);
	  next = next - utcoffset*60;
		tms = localtime((time_t*) &next);
		int nc = tms->tm_hour;
    int ncm = tms->tm_min;
		for(int i=0; i<SHOW_CP_NUM; ++i, nc = (nc+5) % 24){
			snprintf(buffer[3] + CP_DATA_SIZE*i, BUF_SIZE, "%02d:%02d,", nc,ncm); 
		}
		buffer[3][SHOW_CP_NUM * CP_DATA_SIZE - 1] = '\0';
		text_layer_set_text(tl_list, buffer[3]);
    
	}
	tms = localtime((time_t*)&countdown);
  if (countdown <= 300) {
    strftime(buffer[2], BUF_SIZE, "%H:%M:%S", tms);
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(SECOND_UNIT, &handle_tick);
  } else {
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_tick);
	  strftime(buffer[2], BUF_SIZE, "%H:%M", tms);
  };
    text_layer_set_text(tl_countdown, buffer[2]);
  acttime = localtime((time_t*) &rt);
  strftime(buffer[5], BUF_SIZE, "%m/%d %H:%M", acttime);
  text_layer_set_text(tl_realtime, buffer[5]);

}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	update_time(false);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  
	Tuple *ofs = dict_find(iter, 0);
	Tuple *rgn = dict_find(iter, 1);
	memcpy(&mydata,&ofs->value->uint32,sizeof(uint32_t));
  
  //APP_LOG( APP_LOG_LEVEL_ERROR , "%lu: utcoffset received",mydata);
	strncpy(region, rgn->value->cstring, rgn->length);
  text_layer_set_text(tl_region_layer, region);
  update_time(true);
}

void appmessage_init(void) {
	app_message_register_inbox_received(in_received_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void handle_init(void) {
	my_window = window_create();
	window_stack_push(my_window, true);
	
	GFont font_s = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
	GFont font_m = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
	GFont font_b = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
	Layer *root_layer = window_get_root_layer(my_window);	
	GRect frame = layer_get_frame(root_layer);
	
	window_set_background_color(my_window, GColorBlack);
	
	tl_cycle = text_layer_create(GRect(0, 0, 70, 26));
	text_layer_set_font(tl_cycle, font_m);	
	text_layer_set_text_alignment(tl_cycle, GTextAlignmentLeft);
	text_layer_set_background_color(tl_cycle, GColorBlack);
	text_layer_set_text_color(tl_cycle, GColorWhite);
	
	tl_cp = text_layer_create(GRect(70, 0, frame.size.w-70, 26));
	text_layer_set_font(tl_cp, font_m);
	text_layer_set_text_alignment(tl_cp, GTextAlignmentRight);
	text_layer_set_background_color(tl_cp, GColorBlack);
	text_layer_set_text_color(tl_cp, GColorWhite);
	

	tl_countdown = text_layer_create(GRect(0, 25, frame.size.w, 30));
	text_layer_set_font(tl_countdown, font_b);
	text_layer_set_text_alignment(tl_countdown, GTextAlignmentCenter);
	text_layer_set_background_color(tl_countdown, GColorBlack);
	text_layer_set_text_color(tl_countdown, GColorWhite);
	
	tl_list = text_layer_create(GRect(0, frame.size.h-25, frame.size.w, 20));
	text_layer_set_font(tl_list, font_s);
	text_layer_set_text_alignment(tl_list, GTextAlignmentCenter);
	text_layer_set_background_color(tl_list, GColorBlack);
	text_layer_set_text_color(tl_list, GColorWhite);
	
	img_res = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RES); //48x67
	bl_res = bitmap_layer_create(GRect(0, 60, 50, 70));
	bitmap_layer_set_bitmap(bl_res, img_res);
	bitmap_layer_set_alignment(bl_res, GAlignLeft);

  tl_realtime = text_layer_create(GRect(55, 60, frame.size.w-60, 20));
	text_layer_set_font(tl_realtime, font_s);
	text_layer_set_text_alignment(tl_realtime, GTextAlignmentRight);
	text_layer_set_background_color(tl_realtime, GColorBlack);
	text_layer_set_text_color(tl_realtime, GColorWhite);
  
	tl_conn_layer = text_layer_create(GRect(55, 80, frame.size.w-60, 20));
  text_layer_set_font(tl_conn_layer, font_s);
	text_layer_set_text_alignment(tl_conn_layer, GTextAlignmentRight);
	text_layer_set_background_color(tl_conn_layer, GColorBlack);
	text_layer_set_text_color(tl_conn_layer, GColorWhite);
	text_layer_set_text(tl_conn_layer, "BT: --");
	
  tl_batt_layer = text_layer_create(GRect(55, 100, frame.size.w-60, 20));
  text_layer_set_font(tl_batt_layer, font_s);
	text_layer_set_text_alignment(tl_batt_layer, GTextAlignmentRight);
	text_layer_set_background_color(tl_batt_layer, GColorBlack);
	text_layer_set_text_color(tl_batt_layer, GColorWhite);
	text_layer_set_text(tl_batt_layer, "BATT: --");
 	
	tl_region_layer = text_layer_create(GRect(30, 120, frame.size.w-35, 20));
 	text_layer_set_font(tl_region_layer, font_s);
	text_layer_set_text_alignment(tl_region_layer, GTextAlignmentRight);
	text_layer_set_background_color(tl_region_layer, GColorBlack);
	text_layer_set_text_color(tl_region_layer, GColorWhite);
	text_layer_set_text(tl_region_layer, "-REGION-");	
 
	layer_add_child(root_layer, bitmap_layer_get_layer(bl_res));
	layer_add_child(root_layer, text_layer_get_layer(tl_cycle));	
	layer_add_child(root_layer, text_layer_get_layer(tl_cp));	
	layer_add_child(root_layer, text_layer_get_layer(tl_countdown));	
	layer_add_child(root_layer, text_layer_get_layer(tl_list));
	layer_add_child(root_layer, text_layer_get_layer(tl_conn_layer));
	layer_add_child(root_layer, text_layer_get_layer(tl_batt_layer));
  layer_add_child(root_layer, text_layer_get_layer(tl_region_layer));
	layer_add_child(root_layer, text_layer_get_layer(tl_realtime));
  
	//tick_timer_service_subscribe(SECOND_UNIT, &handle_tick);
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_tick);
	bluetooth_connection_service_subscribe(handle_conn);
	handle_conn(bluetooth_connection_service_peek());
	battery_state_service_subscribe(handle_batt);
	handle_batt(battery_state_service_peek());
	update_time(true);
  appmessage_init();
}


void handle_deinit(void) {
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
	text_layer_destroy(tl_cycle);
	text_layer_destroy(tl_cp);
	text_layer_destroy(tl_countdown);
	text_layer_destroy(tl_list);
	text_layer_destroy(tl_conn_layer);
	text_layer_destroy(tl_batt_layer);
	bitmap_layer_destroy(bl_res);
	gbitmap_destroy(img_res);
	window_destroy(my_window);
}


int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}
