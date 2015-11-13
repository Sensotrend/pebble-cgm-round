#include "simple_analog.h"

#include "pebble.h"

static Window *s_window;
static Layer *s_simple_bg_layer, *s_hands_layer;
static TextLayer *s_num_label;

static GPath *s_minute_arrow, *s_hour_arrow;
static char s_num_buffer[5];

static GColor status;

typedef struct {
  time_t measure_time;
  int measured_value; // times 10, as floats are painful
} CGMValue;

static CGMValue cgm_value_buffer[36]; // max 3 hours
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
    index = sizeof(cgm_value_buffer) + cgm_value_index - history;
  } else {
    index = cgm_value_index - history;
  }
  return cgm_value_buffer[index];
}

static void cgm_value_update() {
  int current_value = get_cgm_value(0).measured_value;
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

  bool status_changed = gcolor_equal(status, newStatus);
  if(!status_changed) {
    status = newStatus;
    layer_mark_dirty(s_simple_bg_layer);
    text_layer_set_text_color(s_num_label, status);
  }
  
  int decimal = (current_value%10);
  char decimal_buffer[2];
  snprintf(s_num_buffer, sizeof(s_num_buffer), "%d", (int)current_value/10);
  strcat(s_num_buffer, ".");
  snprintf(decimal_buffer, 2, "%d", decimal);
  strcat(s_num_buffer, decimal_buffer);
  text_layer_set_text(s_num_label, s_num_buffer);
}

static void add_cgm_value(time_t time, int value) {
  if (cgm_value_index == sizeof(cgm_value_buffer) -1) {
    cgm_value_index = -1;
  }
  cgm_value_buffer[++cgm_value_index] = (CGMValue) {time, value};
  cgm_value_update();
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorBlack);

  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);

  printf(text_layer_get_text(s_num_label));
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  printf("Here!");
  status = GColorWhite;
  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_num_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(0, 40, bounds.size.w, 30),
    GRect(0, 40, bounds.size.w, 30)));
  
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, GColorClear);
  text_layer_set_text_color(s_num_label, GColorBlack);
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_num_label, GTextAlignmentCenter);
  
  layer_add_child(window_layer, text_layer_get_layer(s_num_label));

  // mockup
  add_cgm_value(time(NULL), 56);
  cgm_value_update();
  
  printf(text_layer_get_text(s_num_label));
  
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

  // s_num_buffer[0] = '\0';

  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
