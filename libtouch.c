#include <stdint.h>
#include "libtouch.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>


#define PI 3.14159265

typedef struct libtouch_target {
	double x,y,w,h;

} libtouch_target;

typedef struct touch_data {
	int slot;
	double startx;
	double starty;
	double curx;
	double cury;
} touch_data;


double distance_dragged(touch_data *d){
	return sqrt(
		pow(d->startx - d->curx,2) +
		pow(d->starty - d->cury,2)
		);
}

enum libtouch_move_dir direction_dragged(touch_data *d) {
	double dx = d->startx - d->curx;
	double dy = d->starty - d->cury;

	enum libtouch_move_dir direction = 0;

	if(dx > 0) {
		direction |= LIBTOUCH_MOVE_POSITIVE_X;
	}
	else if(dx < 0) {
		direction |= LIBTOUCH_MOVE_NEGATIVE_X;
	}

	if(dy > 0) {
		direction |= LIBTOUCH_MOVE_POSITIVE_Y;
	}
	else if (dy < 0) {
		direction |= LIBTOUCH_MOVE_NEGATIVE_Y;
	}

	return direction;
}


double get_incorrect_drag_distance(touch_data *d, enum libtouch_move_dir direction) {
	double incorrect_squared = 0;

	double dx = d->startx - d->curx;
	double dy = d->starty - d->cury;

	if((direction & LIBTOUCH_MOVE_POSITIVE_X) == LIBTOUCH_MOVE_POSITIVE_X) {
		if(dx < 0) {
			incorrect_squared += pow(dx,2);
		}
	}
	else if ((direction & LIBTOUCH_MOVE_NEGATIVE_X) == LIBTOUCH_MOVE_NEGATIVE_X) {
		if(dx > 0) {
			incorrect_squared += pow(dx,2);
		}
	}
	else
	{
		//Stationary in X
		incorrect_squared += pow(dx,2);
	}


	if((direction & LIBTOUCH_MOVE_POSITIVE_Y) == LIBTOUCH_MOVE_POSITIVE_Y) {
		if(dy < 0) {
			incorrect_squared += pow(dy,2);
		}
	}
	else if ((direction & LIBTOUCH_MOVE_NEGATIVE_Y) == LIBTOUCH_MOVE_NEGATIVE_Y)
	{
		if(dy > 0) {
			incorrect_squared += pow(dy,2);
		}
	}
	else {
		//Stationary in X
		incorrect_squared += pow(dy,2);
	}


	return sqrt(incorrect_squared);
}


typedef struct touch_list {
	touch_data data;
	struct touch_list *next;
} touch_list;



touch_data *get_touch_center(touch_list *touches) {
	int count = 0;
	touch_data *res = calloc(1,sizeof(touch_data));
	touch_list *iter = touches;
	while(iter != NULL) {
		count++;
		res->startx += iter->data.startx;
		res->starty += iter->data.starty;
		res->curx += iter->data.curx;
		res->cury += iter->data.cury;
		iter = iter->next;
	}

	res->curx /= count;
	res->cury /= count;
	res->startx /= count;
	res->starty /= count;

	return res;
}

double get_pinch_scale(touch_list *touches) {
	int count = 0;
	touch_data *center = get_touch_center(touches);
	
	touch_list *iter = touches;
	double old = 0;
	double new = 0;
	while(iter != NULL)
	{
		count++;
		old += sqrt(pow(center->startx - iter->data.startx,2) + pow(center->starty - iter->data.starty,2));
		new += sqrt(pow(center->curx - iter->data.curx,2) + pow(center->cury - iter->data.cury,2));
		iter = iter->next;
	}
	old /= count;
	new /= count;

	free(center);

	return new / old;
}


double get_rotate_angle(touch_list *touches) {
	int count = 0;
	touch_data *center = get_touch_center(touches);
	touch_list *iter = touches;
	double old = 0;
	double new = 0;
	while(iter != NULL) {
		count++;
		old += atan2(iter->data.startx - center->startx, iter->data.starty - center->starty);

		new += atan2(iter->data.curx - center->curx, iter->data.cury - center->cury);
		iter = iter->next;
	}

	old /=count;
	new /=count;

	return (new - old) * 180.0 / PI;
}



typedef struct libtouch_action {
	enum libtouch_action_type action_type;
	double move_tolerance;
	libtouch_target* target;
	int threshold;
	uint32_t duration_ms;
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
} libtouch_gesture;

typedef struct libtouch_engine {
	libtouch_gesture** gestures;
	uint32_t n_gestures;

	libtouch_target **targets;
	uint32_t n_targets;
} libtouch_engine;


typedef struct libtouch_gesture_progress {
	libtouch_gesture *gesture;
	uint32_t completed_actions;
	uint32_t last_action_timestamp;

	double action_progress;
	
	touch_list *touches;
} libtouch_gesture_progress;

typedef struct libtouch_progress_tracker {
	libtouch_gesture_progress *gesture_progress;
	uint32_t n_gestures;
} libtouch_progress_tracker;


libtouch_engine *libtouch_engine_create() {
	libtouch_engine *e = malloc(sizeof(libtouch_engine));
	e->targets = NULL;
	e->n_targets = 0;
	e->gestures = NULL;
	e->n_gestures = 0;
	return e;
}

libtouch_progress_tracker *libtouch_progress_tracker_create(libtouch_engine *engine) {
	libtouch_progress_tracker *t = calloc(sizeof(libtouch_progress_tracker), 1);

	t->gesture_progress = calloc(sizeof(libtouch_gesture_progress), engine->n_gestures);	
	for(int i = 0; i < engine->n_gestures; i++) {
		t->gesture_progress[i].gesture = engine->gestures[i];
	}

	return t;
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
	gestures[engine->n_gestures - 1] = gesture;
	
	return gesture;
}


void libtouch_action_move_tolerance(libtouch_action *action, double min){
	action->move_tolerance = min;
}

void libtouch_gesture_move_tolerance(libtouch_gesture *gesture, double min) {
	for(int i = 0; i < gesture->n_actions; i++){
		libtouch_action_move_tolerance(gesture->actions[i], min);
	}
}

void libtouch_engine_move_tolerance(libtouch_engine *engine, double min) {
	for(int i = 0; i < engine->n_gestures; i++){
		libtouch_gesture_move_tolerance(engine->gestures[i], min);
	}
}


libtouch_target *libtouch_target_create(libtouch_engine *engine,
					double x, double y,
					double width, double height) {
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


bool libtouch_target_contains(libtouch_target *target, double x, double y){
	return !(target->x < x || target->y < y || x > (target->x + target->w) || y > (target->y + target->h));
}

void libtouch_progress_register_touch(
	libtouch_progress_tracker *t,
	uint32_t timestamp, int slot, enum libtouch_touch_mode mode,
	double x, double y)
{
	libtouch_gesture *g;
	libtouch_action *a;
	libtouch_gesture_progress *p;
	for(int i = 0; i < t->n_gestures; i++)
	{
		p = &t->gesture_progress[i];
		g = p->gesture;
		a = g->actions[p->completed_actions];
		if(
			(p->completed_actions == 0 || a->duration_ms < (timestamp - p->last_action_timestamp)) &&
			a->action_type == LIBTOUCH_ACTION_TOUCH &&
			a->touch.mode == mode &&
			libtouch_target_contains(a->target, x,y)) {
			p->action_progress += 1.0 / a->threshold;

			if(mode == LIBTOUCH_TOUCH_DOWN) {
				touch_list *tl = malloc(sizeof(touch_list));
				tl->next = p->touches;
				tl->data.slot = slot;
				tl->data.startx = x;
				tl->data.starty = y;
				tl->data.curx = x;
				tl->data.cury = y;
				
				p->touches = tl;
			}
			else
			{
				touch_list *prev = p->touches;
				touch_list *c = p->touches;
				while(c != NULL && c->data.slot != slot){
					prev = c;
					c = c->next;
				}

				if(c != NULL){
					prev->next = c->next;
					free(c);
				}
			}
			
			if(p->action_progress > 0.9) {
				p->completed_actions++;
				p->last_action_timestamp = timestamp;
			}
			
		}
		else {
			libtouch_gesture_reset_progress(p);
		}
	}
}

touch_data *get_touch_slot(libtouch_gesture_progress *g, int slot) {
	touch_list *t = g->touches;
	while(t != NULL && t->data.slot != slot)
	{
		t = t->next;
	}
	return &t->data;
}


void libtouch_progress_register_move(
	libtouch_progress_tracker *t,
	uint32_t timestamp, int slot, double dx, double dy)
{
	libtouch_gesture_progress *p;
	libtouch_gesture *g;
	libtouch_action *a;
	touch_data *avg;
	for(int i = 0; i < t->n_gestures; i++)
	{
		p = &t->gesture_progress[i];
		g = p->gesture;
		a = g->actions[p->completed_actions];


		touch_data *td = get_touch_slot(p,slot);
		
		td->curx +=dx;
		td->cury +=dy;

		avg = get_touch_center(p->touches);
		
		if(a->duration_ms < (timestamp - p->last_action_timestamp)){
			//Timeout
			libtouch_gesture_reset_progress(p);
			continue;
		}

		double rot,scl,distance,wrong;

		
		switch(a->action_type) {
		case LIBTOUCH_ACTION_TOUCH:
		case LIBTOUCH_ACTION_DELAY:
			if(distance_dragged(td) > a->move_tolerance) {
				libtouch_gesture_reset_progress(p);
			}
			break;
		case LIBTOUCH_ACTION_MOVE:
			if(a->target != NULL)
			{
				
				if(libtouch_target_contains(a->target, avg->curx, avg->cury)) {
					p->completed_actions++;
					p->action_progress = 0;
				}
			}
			else
			{
				//TODO: Handle movement in direction.
				distance = distance_dragged(avg);
				wrong = get_incorrect_drag_distance(avg,a->move.dir);
				if(wrong > a->move_tolerance) {
					libtouch_gesture_reset_progress(p);
				}
				else {
					p->action_progress = (distance - wrong) / a->threshold;
					if(p->action_progress > 1) {
						p->completed_actions++;
						p->action_progress = 0;
					}
				}
			}
			break;
		case LIBTOUCH_ACTION_PINCH:
			distance = distance_dragged(avg);
			if(distance > a->move_tolerance) {
				libtouch_gesture_reset_progress(p);
			}
			else {
			
				scl = get_pinch_scale(p->touches);

				if(scl > ((double) a->threshold) / 100.0) {
					p->completed_actions++;
					p->action_progress = 0;
				}
			}
			break;
		case LIBTOUCH_ACTION_ROTATE:
			distance = distance_dragged(avg);
			if(distance > a->move_tolerance) {
				libtouch_gesture_reset_progress(p);
			}
			else {
				rot = get_rotate_angle(p->touches);
				if(rot > a->threshold){
					p->completed_actions++;
					p->action_progress = 0;
				}
			}
			break;
		}

		free(avg);

		
		
		
	}
}






void libtouch_add_action(libtouch_gesture *gesture, libtouch_action *action){
	libtouch_action **new_array = malloc(sizeof(libtouch_action*) * (gesture->n_actions + 1));
	memcpy(new_array, gesture->actions, sizeof(libtouch_action*) * (gesture->n_actions));
	free(gesture->actions);
	gesture->actions = new_array;
	gesture->actions[gesture->n_actions] = action;
	gesture->n_actions++;
}

libtouch_action *create_action(){
	libtouch_action *action = malloc(sizeof(libtouch_action));
	action->duration_ms = 0;
	action->target = NULL;
	action->move_tolerance = 0;
	action->threshold = 0;
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


double libtouch_gesture_get_progress(libtouch_gesture_progress *gesture) {
	double n_actions = ((double)gesture->gesture->n_actions);
	double n_complete= ((double)gesture->completed_actions);
	double current_pr= ((double)gesture->action_progress);
	return (n_complete + current_pr) / n_actions;
}

void libtouch_gesture_reset_progress(libtouch_gesture_progress *progress) {
	while(progress->touches != NULL) {
		touch_list *l = progress->touches;
		progress->touches = l->next;
		free(l);
	}
	progress->completed_actions = 0;
	progress->action_progress = 0;
}

libtouch_action *libtouch_gesture_get_current_action(libtouch_gesture_progress *progress) {
	return progress->gesture->actions[progress->completed_actions];
}

/*
void libtouch_fill_progress_array(
	libtouch_engine *engine,
	libtouch_gesture_progress **arr,
	uint32_t count);
*/


libtouch_gesture *libtouch_handle_finished_gesture(
	libtouch_progress_tracker *tracker)
{
	for(int i = 0; i < tracker->n_gestures; i++) {
		if(libtouch_gesture_get_progress(&tracker->gesture_progress[i]) > 0.9) {
			//It's done!
			libtouch_gesture_reset_progress(&tracker->gesture_progress[i]);
			return tracker->gesture_progress[i].gesture;
		}
	}

	return NULL;
	
}
