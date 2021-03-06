#include <pebble.h>

#include "my_math.h"

#define DIAL_RADIUS 70
#define V_OFFSET 25

#define RADS 2*M_PI

Window *root_window;

static GColor FGColor = GColorWhite;
static GColor BGColor = GColorBlack;

enum {
  CONFIG_KEY_FACESTYLE = 0x52
};

enum {
  REGULAR_STYLE = 1,
  INVERTED_STYLE = 2
};

Layer *dial_layer, *hour_layer, *minute_layer, *second_layer, *spindle_layer;
GPoint centre, v_centre;
GPath *hour_hand, *minute_hand, *spindle;
GFont *face_font;
GPoint second_points[60][2]; // Cache for points for second hand elements.
float hand_angles[360]; // Cache for minute and hour hand angles.

const GPathInfo HOUR_HAND_PATH_POINTS = {
  5,
  (GPoint[]) {
    {-4, 15},
    {4, 15},
    {10, -30},
    {0, -40},
    {-10, -30}
  }
};

const GPathInfo MINUTE_HAND_PATH_POINTS = {
  5,
  (GPoint[]) {
    {-4, 15},
    {4, 15},
    {10, -55},
    {0, -65},
    {-10, -55}
  }
};

// Thanks to @nerdi for drawing a tiny tiny bolt for me!
const GPathInfo BOLT_PATH_POINTS = {
  6,
  (GPoint[]) {
    {-9, 0},
    {-5, 8},
    {5, 8},
    {9, 0},
    {5, -8},
    {-5, -8}
  }
};

void read_config() {
  if (persist_exists(CONFIG_KEY_FACESTYLE)) {
    int savedstyle = persist_read_int(CONFIG_KEY_FACESTYLE);
    if (savedstyle == INVERTED_STYLE) {
      FGColor = GColorBlack;
      BGColor = GColorWhite;
    }
  }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  (void)reason;
  (void)context;
  APP_LOG(APP_LOG_LEVEL_ERROR, "Data from phone dropped");
}

void in_received_handler(DictionaryIterator *received, void *context) {
  (void)received;
  (void)context;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Data packet received");
  Tuple *remote_facestyle_tuple = dict_find(received, CONFIG_KEY_FACESTYLE);
  if (remote_facestyle_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got facestyle");
    int new_style = remote_facestyle_tuple->value->uint8;
    // BETA 3 seems to be sending single digits as bytes instead of ints
    // so we correct here.
    new_style = new_style-48;
    if (new_style == REGULAR_STYLE) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Switching to regular face");
      FGColor = GColorWhite;
      BGColor = GColorBlack;
      persist_write_int(CONFIG_KEY_FACESTYLE, REGULAR_STYLE);
    } else if (new_style == INVERTED_STYLE) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Switching to inverted face");
      FGColor = GColorBlack;
      BGColor = GColorWhite;
      persist_write_int(CONFIG_KEY_FACESTYLE, INVERTED_STYLE);
    }
    layer_mark_dirty(dial_layer);
  }
}

float get_angle(int16_t divisions, uint16_t count) {
  /* Theory here:
     We use divisions and count to figure the virtual angle from v_centre.
     We know the distance from real centre to v_centre, and the radius
     of the circle around the real centre. These form a SSA triangle.
     We then solve it using the law of sines for the angle from the 
     real centre.
   */
  float v_angle = M_PI + (RADS / divisions * count);
  float x = ((float)V_OFFSET/(float)DIAL_RADIUS) * my_sin(v_angle);
  float angle = M_PI - v_angle - my_asin(x);
  // Finally, return something Pebblesque.
  return -(TRIG_MAX_ANGLE / (2*M_PI) * angle);
}

void get_point_at_angle(GPoint *target, float angle, int8_t length) {
  target->y = (int16_t)(-cos_lookup(angle) *
			length / TRIG_MAX_RATIO) + centre.y;
  target->x = (int16_t)(sin_lookup(angle) *
			length / TRIG_MAX_RATIO) + centre.x;
}

void dial_layer_update(Layer *me, GContext *ctx) {
  (void)me;
  graphics_context_set_fill_color(ctx, BGColor);
  graphics_fill_rect(ctx, layer_get_bounds(dial_layer), 0, GCornerNone);

  // Draw some dial markings
  graphics_context_set_fill_color(ctx, FGColor);
  for (int i=0; i<12; i++) {
    if (i % 3 != 0) { // We'll draw numbers for the quarter-hour later.
      float angle = get_angle(12, i);
      GPoint pip;
      get_point_at_angle(&pip, angle, DIAL_RADIUS);
      graphics_fill_circle(ctx, pip, 4);
    }
  }
  graphics_context_set_text_color(ctx, FGColor);
  // The 12 is separated so we can drop half of it.
  GPoint num_point;
  get_point_at_angle(&num_point, get_angle(12, 0), DIAL_RADIUS);
  graphics_draw_text(ctx, "1", face_font,
		     GRect(num_point.x-16, num_point.y-27, 20, 45),
		     GTextOverflowModeTrailingEllipsis,
		     GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "2", face_font,
		     GRect(num_point.x-4, num_point.y-23, 20, 45),
		     GTextOverflowModeTrailingEllipsis,
		     GTextAlignmentCenter, NULL);
  // The rest are a bit simpler.
  get_point_at_angle(&num_point, get_angle(12, 3), DIAL_RADIUS);
  graphics_draw_text(ctx, "3", face_font,
		     GRect(num_point.x-10, num_point.y-28, 20, 45),
		     GTextOverflowModeTrailingEllipsis,
		     GTextAlignmentCenter, NULL);
  get_point_at_angle(&num_point, get_angle(12, 6), DIAL_RADIUS);
  graphics_draw_text(ctx, "6", face_font,
		     GRect(num_point.x-8, num_point.y-24, 20, 45),
		     GTextOverflowModeTrailingEllipsis,
		     GTextAlignmentCenter, NULL);
  get_point_at_angle(&num_point, get_angle(12, 9), DIAL_RADIUS);
  graphics_draw_text(ctx, "9", face_font,
		     GRect(num_point.x-8, num_point.y-28, 20, 45),
		     GTextOverflowModeTrailingEllipsis,
		     GTextAlignmentCenter, NULL);
}

void second_layer_update(Layer *me, GContext *ctx) {
  (void)me;
  time_t t = time(NULL);
  struct tm *now = localtime(&t);

  if (second_points[now->tm_sec][0].x == 0) { // points have not been cached
    float angle = get_angle(60, now->tm_sec);
    // end of second hand
    get_point_at_angle(&second_points[now->tm_sec][0], angle, DIAL_RADIUS-8);
    // centre of circle on second hand
    get_point_at_angle(&second_points[now->tm_sec][1], angle, DIAL_RADIUS-18);
  }
  graphics_context_set_stroke_color(ctx, FGColor);
  graphics_context_set_fill_color(ctx, FGColor);
  graphics_draw_line(ctx, centre, second_points[now->tm_sec][0]);
  graphics_fill_circle(ctx, second_points[now->tm_sec][1], 7);
}

void minute_layer_update(Layer *me, GContext *ctx) {
  (void)me;
  time_t t = time(NULL);
  struct tm *now = localtime(&t);

  // Want to update every ten seconds; 6 events per minute.
  int16_t offset = now->tm_min*6 + (now->tm_sec / 10);
  if (hand_angles[offset] == 10) { // angle has not been cached
    hand_angles[offset] = get_angle(360, offset);
  }
  gpath_rotate_to(minute_hand, hand_angles[offset]);

  graphics_context_set_fill_color(ctx, FGColor);
  graphics_context_set_stroke_color(ctx, BGColor);
  gpath_draw_filled(ctx, minute_hand);
  gpath_draw_outline(ctx, minute_hand);
}

void hour_layer_update(Layer *me, GContext *ctx) {
  (void)me;
  time_t t = time(NULL);
  struct tm *now = localtime(&t);

  int16_t offset = (now->tm_hour%12)*30 + (now->tm_min / 2);
  if (hand_angles[offset] == 10) { // angle has not been cached
    hand_angles[offset] = get_angle(360, offset);
  }
  gpath_rotate_to(hour_hand, hand_angles[offset]);

  graphics_context_set_fill_color(ctx, FGColor);
  graphics_context_set_stroke_color(ctx, BGColor);
  gpath_draw_filled(ctx, hour_hand);
  gpath_draw_outline(ctx, hour_hand);
}

void spindle_layer_update(Layer *me, GContext *ctx) {
  (void)me;
  graphics_context_set_fill_color(ctx, FGColor);
  graphics_context_set_stroke_color(ctx, BGColor);
  gpath_draw_filled(ctx, spindle);
  gpath_draw_outline(ctx, spindle);

  graphics_draw_circle(ctx, centre, 3);
}

void handle_tick(struct tm *t, TimeUnits units_changed) {
  (void)units_changed;
  layer_mark_dirty(second_layer);
  if (t->tm_sec % 10 == 0) {
    layer_mark_dirty(minute_layer);
  }
}

void init() {
  read_config();

  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

  root_window = window_create();
  window_stack_push(root_window, true /* Animated */);

  face_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_STANISLAV_36));

  // Initialise centre points and hand caches
  Layer *root_window_layer = window_get_root_layer(root_window);
  GRect root_window_bounds = layer_get_bounds(root_window_layer);
  centre = grect_center_point(&root_window_bounds);
  v_centre = GPoint(centre.x, (centre.y-V_OFFSET));
  for (int i=0; i<60; i++) {
    second_points[i][0].x = 0;
    second_points[i][0].y = 0;
    second_points[i][1].x = 0;
    second_points[i][1].y = 0;
  }
  for (int i=0; i<360; i++) {
    hand_angles[i] = 10; // 0 is a valid angle, so pick something else
  }

  // Initialise layers
  dial_layer = layer_create(root_window_bounds);
  layer_set_update_proc(dial_layer, dial_layer_update);
  second_layer = layer_create(root_window_bounds);
  layer_set_update_proc(second_layer, second_layer_update);
  minute_layer = layer_create(root_window_bounds);
  layer_set_update_proc(minute_layer, minute_layer_update);
  hour_layer = layer_create(root_window_bounds);
  layer_set_update_proc(hour_layer, hour_layer_update);
  spindle_layer = layer_create(root_window_bounds);
  layer_set_update_proc(spindle_layer, spindle_layer_update);
  layer_add_child(root_window_layer, dial_layer);
  layer_add_child(root_window_layer, second_layer);
  layer_add_child(root_window_layer, minute_layer);
  layer_add_child(root_window_layer, hour_layer);
  layer_add_child(root_window_layer, spindle_layer);

  // Initialise hands
  minute_hand = gpath_create(&MINUTE_HAND_PATH_POINTS);
  gpath_move_to(minute_hand, centre);
  hour_hand = gpath_create(&HOUR_HAND_PATH_POINTS);
  gpath_move_to(hour_hand, centre);
  spindle = gpath_create(&BOLT_PATH_POINTS);
  gpath_move_to(spindle, centre);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

void deinit(void) {
  gpath_destroy(spindle);
  gpath_destroy(hour_hand);
  gpath_destroy(minute_hand);
  layer_destroy(spindle_layer);
  layer_destroy(hour_layer);
  layer_destroy(minute_layer);
  layer_destroy(second_layer);
  layer_destroy(dial_layer);
  window_destroy(root_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
