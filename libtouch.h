#ifndef _LIBTOUCH_H
#define _LIBTOUCH_H
#include <stdbool.h>
#include <stdint.h>

enum libtouch_action_type {
	/**
	 * Pressing or releasing a finger to or from the touch device.
	 */
	LIBTOUCH_ACTION_TOUCH,
	/**
	 * A difference in the position of the center of the touch group over time.
	 */
	LIBTOUCH_ACTION_MOVE,
	/**
	 * The angle of rotation between each finger and the center of the touch
	 * group changes.
	 */
	LIBTOUCH_ACTION_ROTATE,
	/**
	 * The average distance between each finger and the center of the touch
	 * group changes.
	 */
	LIBTOUCH_ACTION_SCALE,
	/**
	 * No change within the configured thresholds over a certain time frame.
	 */
	LIBTOUCH_ACTION_DELAY,
};

/**
 * Represents a change in the number of touch points in the current touch group.
 * DOWN represents pressing a finger against the touch device, and UP represents
 * removing the finger from the device. Both represents any kind of change.
 */
enum libtouch_touch_mode {
	LIBTOUCH_TOUCH_UP = 1 << 0,
	LIBTOUCH_TOUCH_DOWN 1 << 1,
};

/**
 * Represents the directions in which a LIBTOUCH_ACTION_MOVE can occur. Both
 * POSITIVE_X and NEGATIVE_X corresponds to movement in either direction along
 * the X axis; the same holds for Y.
 */
enum libtouch_move_dir {
	LIBTOUCH_MOVE_POSITIVE_X = 1 << 0,
	LIBTOUCH_MOVE_POSITIVE_Y = 1 << 1,
	LIBTOUCH_MOVE_NEGATIVE_X = 1 << 2,
	LIBTOUCH_MOVE_NEGATIVE_Y = 1 << 3,
};

/**
 * Represents the direction of rotation in which a LIBTOUCH_ACTION_ROTATE can
 * occur. Both CLOCKWISE and ANTICLOCKWISE corresponds to a rotation in either
 * direction.
 */
enum libtouch_rotate_dir {
	LIBTOUCH_ROTATE_CLOCKWISE = 1 << 0,
	LIBTOUCH_ROTATE_ANTICLOCKWISE = 1 << 1,
};

/**
 * Represents the direction in which a LIBTOUCH_ACTION_SCALE can occur. Both
 * UP and DOWN corresponds to a change of any amount.
 */
enum libtouch_scale_dir {
	LIBTOUCH_SCALE_UP = 1 << 0,
	LIBTOUCH_SCALE_DOWN = 1 << 1,
};

struct libtouch_engine;
struct libtouch_gesture;
struct libtouch_action;
struct libtouch_target;

struct libtouch_engine *libtouch_engine_create();

struct libtouch_gesture *libtouch_gesture_create(
		struct libtouch_engine *engine);

struct libtouch_target *libtouch_target_create(
		struct libtouch_engine *engine, unsigned int x, unsigned int y,
		unsigned int width, unsigned int height);

typedef bool (*candidate_handler)(struct libtouch_engine *,
		struct libtouch_gesture *, void *user_data);

/**
 * Iterates over the current gesture candidates in descending order of
 * progress. Return false to stop the iteration, or true to continue.
 */
void libtouch_engine_get_candidates(struct libtouch_engine *engine,
		candidate_handler handler, void user_data);

/**
 * Informs the touch engine of an input event.
 *
 * timestamp: milliseconds from an arbitrary epoch (e.g. CLOCK_MONOTONIC)
 * slot: the slot of this event (e.g. which finger the event was caused by)
 */
void libtouch_engine_touch_event(struct libtouch_engine *engine,
		uint32_t timestamp, int slot, enum libtouch_touch_mode mode,
		unsigned int x, unsigned int y);

/** Returns the progress of this gesture from 0..1. */
double libtouch_gesture_get_progress(struct libtouch_gesture *gesture);

/** Returns the active action for this gesture. */
struct libtouch_action *libtouch_gesture_get_current_action(
		struct libtouch_gesture *gesture);

struct libtouch_action *libtouch_gesture_add_touch(
		struct libtouch_gesture *gesture, uint32_t mode);
struct libtouch_action *libtouch_gesture_add_move(
		struct libtouch_gesture *gesture, uint32_t direction);
struct libtouch_action *libtouch_gesture_add_rotate(
		struct libtouch_gesture *gesture, uint32_t direction);
struct libtouch_action *libtouch_gesture_add_scale(
		struct libtouch_gesture *gesture, uint32_t direction);

/**
 * Sets the threshold of change for an action to be considered complete. The map
 * of threshold units to action type is as follows:
 *
 * - LIBTOUCH_ACTION_TOUCH:  number of touch points
 * - LIBTOUCH_ACTION_MOVE:   positional units
 * - LIBTOUCH_ACTION_ROTATE: degrees
 * - LIBTOUCH_ACTION_SCALE:  positional units
 * - LIBTOUCH_ACTION_DELAY:  milliseconds (must be positive)
 */
void libtouch_action_set_threshold(
		struct libtouch_action *action, int threshold);

/**
 * Sets a libtouch_target that the action must reach to be considered complete.
 * Only valid for LIBTOUCH_ACTION_MOVE actions, and only valid if a threshold is
 * not set.
 */
void libtouch_action_set_target(
		struct libtouch_action *action, struct libtouch_target *target);

/**
 * Sets the minimum duration this action must take place during to be considered
 * a match.
 */
void libtouch_action_set_duration(struct libtouch_action *action,
		uint32_t duration_ms);

/**
 * Gets the current progress of this action between 0 and 1.
 */
double libtouch_action_get_progress(struct libtouch_action *action);

/**
 * Gets the next action in this gesture, or NULL if there are none.
 */
struct libtouch_action_get_next(struct libtouch_action *action);

struct libtouch_target_set_boundaries(struct libtouch_target *target,
		unsigned int x, unsigned int y,
		unsigned int width, unsigned int height);

#endif
