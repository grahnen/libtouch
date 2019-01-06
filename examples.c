#include "libtouch.h"


void libtouch_add_tap(libtouch_gesture *gesture, uint32_t n_fingers, uint32_t maxhold) {
  libtouch_action *touch = libtouch_add_touch(gesture,0);
  libtouch_action_set_treshold(touch1, n_fingers);
  
  release = libtouch_add_touch(gesture, 1);
  libtouch_action_set_treshold(release, n_fingers);
  libtouch_action_set_duration(release, maxhold);

  libtouch_action_move_tolerance(touch, 10);
  libtouch_action_move_tolerance(release, 10)
}

libtouch_gesture *libtouch_add_leftedge_swipe(libtouch_engine *engine, uint32_t n_fingers, uint32_t margin, uint32_t height) {
  left_edge = libtouch_target_create(engine, 0, 0, margin, height);

  g = libtouch_gesture_create(engine);
  touch = libtouch_gesture_add_touch(g,0);
  libtouch_action_set_boundaries(touch, left_edge);
  libtouch_action_set_threshold(touch, n_fingers);
  
  move = libtouch_gesture_add_move(g, LIBTOUCH_MOVE_POSITIVE_X);
  libtouch_action_set_treshold(move, 50);
}
