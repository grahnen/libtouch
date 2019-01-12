#include <stdint.h>
#include "libtouch.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

typedef struct libtouch_target {
	unsigned int x,y,w,h;

} libtouch_target;

typedef struct touch_data {
	int slot;
	unsigned int startx;
	unsigned int starty;
	unsigned int curx;
	unsigned int cury;
} touch_data;

double distance_dragged(touch_data *d){
	return sqrt(
		pow(d->startx - d->curx,2) +
		pow(d->starty - d->cury,2)
		);
}


typedef struct touch_list {
	touch_data data;
	struct touch_list *next;
} touch_list;


typedef struct libtouch_action {
	enum libtouch_action_type action_type;
	int move_tolerance;
	libtouch_target* target;
	int threshold;
	uint32_t duration_ms;
	double progress;
	union {
		//Touch Action
		struct {
			enum libtouch_touch_mode mode;
		} touch;
		//Move Action
		struct {
			enum libtouch_move_dir dir;
		} move;
		//Rotate Action
		struct {
			enum libtouch_rotate_dir dir;
		} rotate;
		//Pinch Action
		struct {
			enum libtouch_scale_dir dir;
		} pinch;
		//Delay Action
		struct {
			
		} delay;
	};
} libtouch_action;


typedef struct libtouch_gesture {
	libtouch_action **actions;
	uint32_t n_actions;
	uint32_t completed_actions;
	uint32_t last_action_timestamp;
	touch_list *touches;
} libtouch_gesture;

typedef struct libtouch_engine {
	libtouch_gesture** gestures;
	uint32_t n_gestures;

	libtouch_target **targets;
	uint32_t n_targets;

	
	
} libtouch_engine;


libtouch_engine *libtouch_engine_create() {
	libtouch_engine *e = malloc(sizeof(libtouch_engine));
	e->targets = NULL;
	e->n_targets = 0;
	e->gestures = NULL;
	e->n_gestures = 0;
	return e;
}


libtouch_gesture *libtouch_gesture_create(libtouch_engine *engine) {
	//Increase gesture array size.
	libtouch_gesture **gestures = malloc(sizeof(libtouch_gesture*) * engine->n_gestures + 1);
	memcpy(gestures, engine->gestures, sizeof(libtouch_gesture*) * engine->n_gestures);
	free(engine->gestures);
	engine->gestures = gestures;
	engine->n_gestures++;
	
	//Add the gesture
	libtouch_gesture *gesture = malloc(sizeof(libtouch_gesture));
	gesture->actions = NULL;
	gesture->n_actions = 0;
	gesture->completed_actions = 0;

	gesture->touches = NULL;
	
	gestures[engine->n_gestures - 1] = gesture;
	
	return gesture;
}


void libtouch_action_move_tolerance(libtouch_action *action, int min){
	action->move_tolerance = min;
}

void libtouch_gesture_move_tolerance(libtouch_gesture *gesture, int min) {
	for(int i = 0; i < gesture->n_actions; i++){
		libtouch_action_move_tolerance(gesture->actions[i], min);
	}
}

void libtouch_engine_move_tolerance(libtouch_engine *engine, int min) {
	for(int i = 0; i < engine->n_gestures; i++){
		libtouch_gesture_move_tolerance(engine->gestures[i], min);
	}
}


libtouch_target *libtouch_target_create(libtouch_engine *engine,
					unsigned int x, unsigned int y,
					unsigned int width, unsigned int height) {
	//Increase array size
	libtouch_target **arr = malloc(sizeof(libtouch_target*) * (1 + engine->n_targets));
	memcpy(arr, engine->targets, engine->n_targets);

	libtouch_target *t = engine->targets[engine->n_targets];
	t->x = x;
	t->y = y;
	t->w = width;
	t->h = height;

	engine->n_targets++;

	return t;
}


bool libtouch_target_contains(libtouch_target *target, unsigned int x, unsigned int y){
	return !(target->x < x || target->y < y || x > (target->x + target->w) || y > (target->y + target->h));
}

void libtouch_engine_register_touch(
	libtouch_engine *engine,
	uint32_t timestamp, int slot, enum libtouch_touch_mode mode,
	unsigned int x, unsigned int y)
{
	libtouch_gesture *g;
	libtouch_action *a;
	for(int i = 0; i < engine->n_gestures; i++)
	{
		g = engine->gestures[i];
		a = g->actions[g->completed_actions];
		if(
			(g->completed_actions == 0 || a->duration_ms < (timestamp - g->last_action_timestamp)) &&
			a->action_type == LIBTOUCH_ACTION_TOUCH &&
			a->touch.mode == mode &&
			libtouch_target_contains(a->target, x,y)) {
			a->progress += 1.0 / a->threshold;

			if(mode == LIBTOUCH_TOUCH_DOWN) {
				touch_list *t = malloc(sizeof(touch_list));
				t->next = g->touches;
				t->data.slot = slot;
				t->data.startx = x;
				t->data.starty = y;
				t->data.curx = x;
				t->data.cury = y;
				
				g->touches = t;
			}
			else
			{
				touch_list *prev = g->touches;
				touch_list *c = g->touches;
				while(c != NULL && c->data.slot != slot){
					prev = c;
					c = c->next;
				}

				if(c != NULL){
					prev->next = c->next;
					free(c);
				}
			}
			
			if(a->progress > 0.9) {
				g->completed_actions++;
				g->last_action_timestamp = timestamp;
			}
			
		}
		else {
			libtouch_gesture_reset_progress(g);
		}
	}
}

touch_list *get_touch_slot(libtouch_gesture *g, int slot) {
	touch_list *t = g->touches;
	while(t != NULL && t->data.slot != slot)
	{
		t = t->next;
	}
	return t;
}


void libtouch_engine_register_move(
	libtouch_engine *engine,
	uint32_t timestamp, int slot, int dx, int dy)
{
	libtouch_gesture *g;
	libtouch_action *a;
	for(int i = 0; i < engine->n_gestures; i++)
	{

		touch_list *t = get_touch_slot(g,slot);
		
		t->data.curx +=dx;
		t->data.cury +=dy;

		g = engine->gestures[i];
		a = g->actions[g->completed_actions];
		switch(a->action_type) {
		case LIBTOUCH_ACTION_TOUCH:
		case LIBTOUCH_ACTION_DELAY:
			if(distance_dragged(&t->data) > a->move_tolerance) {
				libtouch_gesture_reset_progress(g);
			}
		case LIBTOUCH_ACTION_MOVE:
			if(a->target != NULL)
			{
				if(libtouch_target_contains(a->target, t->data.curx, t->data.cury)) {
					g->completed_actions++;
				}
			}
			else
			{
				//TODO: Handle movement in direction.
			}
		}

		
		
		
	}
}






void libtouch_add_action(libtouch_gesture *gesture, libtouch_action *action){
	libtouch_action **new_array = malloc(sizeof(libtouch_action*) * (gesture->n_actions + 1));
	memcpy(new_array, gesture->actions, sizeof(libtouch_action*) * (gesture->n_actions));
	gesture->actions[gesture->n_actions] = action;
	gesture->n_actions++;
}

libtouch_action *create_action(){
	libtouch_action *action = malloc(sizeof(libtouch_action));
	action->duration_ms = 0;
	action->target = NULL;
	action->move_tolerance = 0;
	action->threshold = 0;
	action->progress = 0;
	return action;
}

struct libtouch_action *libtouch_gesture_add_touch(
	struct libtouch_gesture *gesture,
	uint32_t mode)
{
	libtouch_action *action = create_action();
	action->action_type = LIBTOUCH_ACTION_TOUCH;
	action->touch.mode = mode;
	libtouch_add_action(gesture, action);
	return action;
	
}

struct libtouch_action *libtouch_gesture_add_move(
	struct libtouch_gesture *gesture, uint32_t direction)
{
	libtouch_action *action = create_action();
	action->action_type = LIBTOUCH_ACTION_MOVE;
	action->move.dir = direction;
	libtouch_add_action(gesture, action);
	return action;
}

struct libtouch_action *libtouch_gesture_add_rotate(
	struct libtouch_gesture *gesture, uint32_t direction)
{
	libtouch_action *action = create_action();
	action->action_type = LIBTOUCH_ACTION_ROTATE;
	action->rotate.dir = direction;
	libtouch_add_action(gesture, action);
	return action;
}

struct libtouch_action *libtouch_gesture_add_pinch(
	struct libtouch_gesture *gesture, uint32_t direction)
{
	libtouch_action *action = create_action();
	action->action_type = LIBTOUCH_ACTION_PINCH;
	action->pinch.dir = direction;
	libtouch_add_action(gesture, action);
	return action;
}

struct libtouch_action *libtouch_gesture_add_delay(
	struct libtouch_gesture *gesture, uint32_t duration)
{
	libtouch_action *action = create_action();
	action->action_type = LIBTOUCH_ACTION_DELAY;
	libtouch_add_action(gesture, action);
	return action;
}


void libtouch_action_set_threshold(
	libtouch_action *action, int threshold)
{
	action->threshold = threshold;
}

void libtouch_action_set_target(
	libtouch_action *action, libtouch_target *target)
{
	action->target = target;
}


void libtouch_action_set_duration(
	libtouch_action *action, uint32_t duration_ms)
{
	action->duration_ms = duration_ms;
}


double libtouch_action_get_progress(libtouch_action *action) {

	return action->progress;
}


double libtouch_gesture_get_progress(libtouch_gesture *gesture) {
	double n_actions = ((double)gesture->n_actions);
	double n_complete= ((double)gesture->completed_actions);
	double current_pr= ((double)libtouch_gesture_get_current_action(gesture)->progress);
	return (n_complete + current_pr) / n_actions;
}

void libtouch_gesture_reset_progress(libtouch_gesture *gesture) {
	for(int i = 0; i < gesture->n_actions; i++) {
		gesture->actions[i]->progress = 0;
	}
	gesture->completed_actions = 0;
}

libtouch_action *libtouch_gesture_get_current_action(libtouch_gesture *gesture) {
	return gesture->actions[gesture->completed_actions];
}
