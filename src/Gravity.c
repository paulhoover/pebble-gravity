#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "my_math.h"

#define MY_UUID { 0x7B, 0x4E, 0x89, 0x33, 0x1E, 0x3F, 0x4B, 0x57, 0xBE, 0x13, 0xD0, 0x7D, 0x76, 0xB6, 0x14, 0x17 }
PBL_APP_INFO(MY_UUID,
             "Gravity", "Kids, Inc.",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define FOREGROUND GColorWhite
#define BACKGROUND GColorBlack

#define DIAL_RADIUS 70
#define V_OFFSET 25

#define RADS 2*M_PI

Window window;

Layer dial_layer, hour_layer, minute_layer, second_layer;
GPoint centre, v_centre;
GPath hour_hand, minute_hand;

const GPathInfo HOUR_HAND_PATH_POINTS = {
  5,
  (GPoint[]) {
    {-4, 2},
    {4, 2},
    {4, -30},
    {0, -40},
    {-4, -30}
  }
};

const GPathInfo MINUTE_HAND_PATH_POINTS = {
  5,
  (GPoint[]) {
    {-4, 2},
    {4, 2},
    {4, -55},
    {0, -65},
    {-4, -55}
  }
};

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
  graphics_context_set_fill_color(ctx, BACKGROUND);
  graphics_fill_rect(ctx, dial_layer.bounds, 0, GCornerNone);

  // Draw some dial markings
  graphics_context_set_fill_color(ctx, FOREGROUND);
  for (int i=0; i<12; i++) {
    float angle = get_angle(12, i);
    GPoint pip;
    get_point_at_angle(&pip, angle, DIAL_RADIUS);
    graphics_fill_circle(ctx, pip, 4);
  }
  // Draw a centre marking
  graphics_fill_circle(ctx, centre, 4);
}

void second_layer_update(Layer *me, GContext *ctx) {
  PblTm now;
  get_time(&now);

  float angle = get_angle(60, now.tm_sec);
  GPoint sec;
  get_point_at_angle(&sec, angle, DIAL_RADIUS-2);
  graphics_context_set_stroke_color(ctx, FOREGROUND);
  graphics_draw_line(ctx, centre, sec);
}

void minute_layer_update(Layer *me, GContext *ctx) {
  PblTm now;
  get_time(&now);

  // Want to update every ten seconds; 6 events per minute.
  int16_t offset = now.tm_min*6 + (now.tm_sec / 10);
  float angle = get_angle(360, offset);
  gpath_rotate_to(&minute_hand, angle);

  graphics_context_set_fill_color(ctx, FOREGROUND);
  graphics_context_set_stroke_color(ctx, BACKGROUND);
  gpath_draw_filled(ctx, &minute_hand);
  gpath_draw_outline(ctx, &minute_hand);
}

void hour_layer_update(Layer *me, GContext *ctx) {
  PblTm now;
  get_time(&now);

  int16_t offset = now.tm_hour*30 + (now.tm_min / 2);
  float angle = get_angle(360, offset);
  gpath_rotate_to(&hour_hand, angle);

  graphics_context_set_fill_color(ctx, FOREGROUND);
  graphics_context_set_stroke_color(ctx, BACKGROUND);
  gpath_draw_filled(ctx, &hour_hand);
  gpath_draw_outline(ctx, &hour_hand);
}

void handle_tick(AppContextRef ctx, PebbleTickEvent *t) {
  layer_mark_dirty(&second_layer);
  if (t->tick_time->tm_sec % 10 == 0) {
    layer_mark_dirty(&minute_layer);
  }
}

void handle_init(AppContextRef ctx) {

  window_init(&window, "Gravity");
  window_stack_push(&window, true /* Animated */);

  centre = grect_center_point(&window.layer.frame);
  v_centre = GPoint(centre.x, (centre.y-V_OFFSET));

  layer_init(&dial_layer, window.layer.bounds);
  dial_layer.update_proc = dial_layer_update;
  layer_init(&second_layer, window.layer.bounds);
  second_layer.update_proc = second_layer_update;
  layer_init(&minute_layer, window.layer.bounds);
  minute_layer.update_proc = minute_layer_update;
  layer_init(&hour_layer, window.layer.bounds);
  hour_layer.update_proc = hour_layer_update;
  layer_add_child(&window.layer, &dial_layer);
  layer_add_child(&window.layer, &second_layer);
  layer_add_child(&window.layer, &minute_layer);
  layer_add_child(&window.layer, &hour_layer);

  // init hands
  gpath_init(&minute_hand, &MINUTE_HAND_PATH_POINTS);
  gpath_move_to(&minute_hand, centre);
  gpath_init(&hour_hand, &HOUR_HAND_PATH_POINTS);
  gpath_move_to(&hour_hand, centre);
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = handle_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
