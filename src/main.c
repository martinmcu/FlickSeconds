#include <pebble.h>
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static GBitmap *s_bt_connected_bitmap;
static GBitmap *s_bt_disconnected_bitmap;
static BitmapLayer *s_bt_layer;
static bool s_first_run_bool = true;
static bool s_show_seconds_bool;
static int s_show_seconds_duration_int = 15;
static time_t s_hide_seconds_time_t;
static InverterLayer *s_inv_layer;
static GBitmap *s_pi_bitmap;
static BitmapLayer *s_pi_layer;
static bool s_pi_time_bool;

static void format_time() {
  if (s_show_seconds_bool == true) {
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  } else {
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  }
}

static void update_time() {
  
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a text buffers
  static char buffer[] = "00:00:00";
  static char date_buffer[16];
  static char pi_start_buffer[] = "15:14:00";
  static char pi_end_buffer[] = "15:15:00";
  static char pi_cmp_buffer[] = "00:00:00";
  
  // Check to see if it's time to hide the seconds
  if (temp > s_hide_seconds_time_t) {
    s_show_seconds_bool = false;
    format_time();
  }

  // Detect Pi Time
  strftime(pi_cmp_buffer, sizeof(pi_cmp_buffer), "%H:%M:%S", tick_time);
  if(strcmp(pi_cmp_buffer, pi_start_buffer) == 0) {
    layer_set_hidden(bitmap_layer_get_layer(s_pi_layer), false);
    vibes_double_pulse();
  }
  if(strcmp(pi_cmp_buffer, pi_end_buffer) == 0) {
    layer_set_hidden(bitmap_layer_get_layer(s_pi_layer), true);
  }
  
  // Write the current time into the buffer based on tap state
  if(s_show_seconds_bool == true) {
    if(clock_is_24h_style() == true) {
      // Use 24 hour format
      strftime(buffer, sizeof("00:00:00"), "%H:%M:%S", tick_time);
    } else {
      // Use 12 hour format
      strftime(buffer, sizeof("00:00:00"), "%I:%M:%S", tick_time);
    }
  } else {
    if(clock_is_24h_style() == true) {
      // Use 24 hour format
      strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
    } else {
      // Use 12 hour format
      strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
    }
  }

  // Display formatted time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  
  // Parse date
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);
  text_layer_set_text(s_date_layer, date_buffer);
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void battery_handler(BatteryChargeState charge_state) {
  static char battery_text[] = "+100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "+%d%%", charge_state.charge_percent);
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

static void bluetooth_handler(bool connected) {
  
  // Vibrate upon change in connection status
  if (s_first_run_bool == true){
    s_first_run_bool = false;
  } else {
    vibes_double_pulse();
  }
    
  // Set connection image
  if (connected == true) {
    bitmap_layer_set_bitmap(s_bt_layer, s_bt_connected_bitmap);
  } else {
    bitmap_layer_set_bitmap(s_bt_layer, s_bt_disconnected_bitmap);    
  }
  
  // Invert colors if disconnected
  layer_set_hidden(inverter_layer_get_layer(s_inv_layer), connected);
  
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  
  s_show_seconds_bool = !s_show_seconds_bool;
  format_time();
  update_time();
  
  // Create time to hide seconds
  s_hide_seconds_time_t = time(NULL) + s_show_seconds_duration_int;
  
}

static void main_window_load(Window *window) {
  
  // Create root layer
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
    // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 35, window_bounds.size.w, 44));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(0, 82, window_bounds.size.w, 26));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  
  // Create battery TextLayer
  s_battery_layer = text_layer_create(GRect(window_bounds.size.w / 2, 0, (window_bounds.size.w / 2) - 2, 18));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  
  // Create Pi time BitmapLayer
  s_pi_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PI_BACKGROUND);
  s_pi_layer = bitmap_layer_create(GRect(0,17,144,150));
  bitmap_layer_set_bitmap(s_pi_layer, s_pi_bitmap);
  layer_set_hidden(bitmap_layer_get_layer(s_pi_layer), true);
  
  // Create bluetooth BitmapLayer
  s_bt_connected_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_CONNECTED);
  s_bt_disconnected_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_DISCONNECTED);
  s_bt_layer = bitmap_layer_create(GRect(3,3,12,11));
 
  
  // Create bluetooth connected InverterLayer
  s_inv_layer = inverter_layer_create(GRect(0,0,window_bounds.size.w,window_bounds.size.h));
      
  // Initialize watch state
  s_show_seconds_bool = true;
  battery_handler(battery_state_service_peek());
  bluetooth_handler(bluetooth_connection_service_peek());
  format_time();

  // Add all layers to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_pi_layer));
  layer_add_child(window_layer, inverter_layer_get_layer(s_inv_layer));
    
}

static void main_window_unload(Window *window) {
  // Unregister services 
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  accel_tap_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  
  // Destroy BT resources
  bitmap_layer_destroy(s_bt_layer);
  gbitmap_destroy(s_bt_connected_bitmap);
  gbitmap_destroy(s_bt_disconnected_bitmap);
  inverter_layer_destroy(s_inv_layer);
  
  // Destroy Pi Time
  bitmap_layer_destroy(s_pi_layer);
  gbitmap_destroy(s_pi_bitmap);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Register with services
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  accel_tap_service_subscribe(tap_handler);
  bluetooth_connection_service_subscribe(bluetooth_handler);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Set all displays
  update_time();

}

static void deinit() {
    // Destroy Window
    window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
