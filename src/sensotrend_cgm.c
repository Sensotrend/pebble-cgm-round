#include "sensotrend_cgm.h"

#include "pebble.h"
#define HAND_MAX_RADIUS 55
#define HAND_MARGIN  10
#define KEY_TIME 1
#define KEY_VALUE 2
// max 3 hours
#define MAX_POINTS 36


static Window *s_window;
static Layer *s_simple_bg_layer, *s_hands_layer, *spark_layer;
static TextLayer *s_num_label;

static char s_num_buffer[5];

static GColor status;

typedef struct {
  int hours;
  int minutes;
} Time;

//static GPoint s_center;
static Time s_last_time;
  
typedef struct {
  time_t time;
  int value; // times 10, as floats are painful
} CGMValue;

static CGMValue cgm_value_buffer[MAX_POINTS];
static int cgm_value_index = 0;

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, status);
  #if defined(PBL_ROUND)
  graphics_fill_radial(ctx, layer_get_bounds(layer), GOvalScaleModeFitCircle, 25, 0, TRIG_MAX_ANGLE);
  #endif  
}

static CGMValue get_cgm_value(int history) {
  int index;
  if (history > cgm_value_index) {
    index = MAX_POINTS + cgm_value_index - history;
  } else {
    index = cgm_value_index - history;
  }
  return cgm_value_buffer[index];
}

static void cgm_value_update() {
  int current_value = get_cgm_value(0).value;
  GColor newStatus;
  if (current_value == 0) {
    newStatus = GColorBlack;
  } else if (current_value < 40) {
    newStatus = GColorRed;
  } else if (current_value < 80) {
    newStatus = GColorBlueMoon;
  } else {
    newStatus = GColorVividViolet;
  }

  bool status_changed = !gcolor_equal(status, newStatus);
  if(status_changed) {
    status = newStatus;
    layer_mark_dirty(s_simple_bg_layer);
  }
  
  int decimal = (current_value%10);
  char decimal_buffer[2];
  snprintf(s_num_buffer, sizeof(s_num_buffer), "%d", (int)current_value/10);
  strcat(s_num_buffer, ".");
  snprintf(decimal_buffer, 2, "%d", decimal);
  strcat(s_num_buffer, decimal_buffer);
  text_layer_set_text(s_num_label, s_num_buffer);
  // APP_LOG(APP_LOG_LEVEL_INFO, "New value %d added at %d", current_value, cgm_value_index);
  layer_mark_dirty(spark_layer);
}

static void add_cgm_value(time_t time, int value) {
  if (cgm_value_index == MAX_POINTS -1) {
    cgm_value_index = -1;
  }
  // APP_LOG(APP_LOG_LEVEL_INFO, "Adding value %d at %d", value, cgm_value_index);
  cgm_value_buffer[++cgm_value_index] = (CGMValue) {time, value};
  cgm_value_update();
}

static void draw_spark(Layer *layer, GContext *ctx) {
  graphics_context_set_antialiased(ctx, true);
  GPoint points[MAX_POINTS];
  time_t now = time(NULL);
  graphics_context_set_fill_color(ctx, GColorBlack);
  GRect bounds = layer_get_bounds(layer);
  for (int i=MAX_POINTS; i>=0; i--) {
    CGMValue v = get_cgm_value(i);
    if (v.value != 0) {
      int x = MAX_POINTS * 2 - (now - v.time)/120; // 5 minute interval
      int y = bounds.size.h - (int) (v.value/5);
      points[i] = GPoint(x, y);
      graphics_fill_circle(ctx, points[i], i==0 ? 3 : 1); // most recent is bigger!
      // APP_LOG(APP_LOG_LEVEL_INFO, "Drew a circle at %d, %d", x, y);
    }
  }
}

static void hands_update_proc(Layer *layer, GContext *ctx) {

  graphics_context_set_antialiased(ctx, true);
  
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  // Adjust for minutes through the hour
  float minute_angle = TRIG_MAX_ANGLE * s_last_time.minutes / 60;
  float hour_angle = TRIG_MAX_ANGLE * s_last_time.hours / 12;
  hour_angle += (minute_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);

  // Plot hands
  GPoint minute_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * s_last_time.minutes / 60) * (int32_t)(HAND_MAX_RADIUS - HAND_MARGIN) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * s_last_time.minutes / 60) * (int32_t)(HAND_MAX_RADIUS - HAND_MARGIN) / TRIG_MAX_RATIO) + center.y,
  };
  GPoint hour_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)(HAND_MAX_RADIUS - (2 * HAND_MARGIN)) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)(HAND_MAX_RADIUS - (2 * HAND_MARGIN)) / TRIG_MAX_RATIO) + center.y,
  };

  graphics_context_set_stroke_width(ctx, 4);
  graphics_draw_line(ctx, center, hour_hand);
//  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, center, minute_hand);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Store time
  s_last_time.hours = tick_time->tm_hour;
  s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
  s_last_time.minutes = tick_time->tm_min;
  
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
  Tuple *time_tuple = dict_find(iterator, KEY_TIME);
  Tuple *value_tuple = dict_find(iterator, KEY_VALUE);
  add_cgm_value((time_t) time_tuple->value->uint32, value_tuple->value->int16);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Message dropped!");
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  status = GColorWhite;
  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_num_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(0, 37, bounds.size.w, 30),
    GRect(0, 37, bounds.size.w, 30)));
  
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, GColorClear);
  text_layer_set_text_color(s_num_label, GColorBlack);
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_num_label, GTextAlignmentCenter);
  
  layer_add_child(window_layer, text_layer_get_layer(s_num_label));

  spark_layer = layer_create(PBL_IF_ROUND_ELSE(
    GRect(bounds.size.w/2-MAX_POINTS-2, 70, MAX_POINTS*2+4, 80),
    GRect(bounds.size.w/2-MAX_POINTS-2, 47, MAX_POINTS*2+4, 80)));
  layer_set_update_proc(spark_layer, draw_spark);
  layer_add_child(window_layer, spark_layer);
  
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);

  text_layer_destroy(s_num_label);

  layer_destroy(s_hands_layer);
}

static void init() {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_open(64, 8);
  
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
