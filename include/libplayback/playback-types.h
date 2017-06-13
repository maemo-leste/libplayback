/*
** Playback manager - common types
** (c) Nokia MM - Makoto Sugano, 2007
** 
*/

#ifndef PLAYBACK_TYPES_H_
# define PLAYBACK_TYPES_H_

#include <libplayback/playback-macros.h>

PB_BEGIN_DECLS

/* Note that states are not actual system states, instead they are
 * instructions from the Manager to the client ("Go to state STOP"). */

enum pb_state_e {
  /**< define an unknown PLAYBACK state (must be avoided) */
  PB_STATE_NONE,
  /**< PLAYBACK state is "STOP" (assumptions on resource handles are strong 
   * while there is no resource management solution: RESOURCE SHOULD BE FREED) */
  PB_STATE_STOP,
  /**< PLAYBACK state  is "PLAY" */
  PB_STATE_PLAY,
  PB_STATE_LAST,
};

#define PB_FLAG_AUDIO 0x1
#define PB_FLAG_VIDEO 0x2
#define PB_FLAG_AUDIO_RECORDING 0x4
#define PB_FLAG_VIDEO_RECORDING 0x8

enum pb_class_e {
  PB_CLASS_NONE,	/**<  declare an "unknown" (or undefined) PLAYBACK class */
  PB_CLASS_TEST,	/**< "test" class */
  PB_CLASS_EVENT,	/**<  declare an "event" playback */
  PB_CLASS_VOIP,	/**< "VoIP" class */
  PB_CLASS_CALL = PB_CLASS_VOIP,
  PB_CLASS_MEDIA,	/**< "Media" class */
  PB_CLASS_BACKGROUND,	/**< "Background" class */
  PB_CLASS_RINGTONE,	/**< "Ringtone" class */
  PB_CLASS_VOICEUI,	/**< "Voice UI" class, for instance for text-to-speech */
  PB_CLASS_CAMERA,	/**< "Camera" class for active camera preview. */
  PB_CLASS_GAME,	/**< "Game" class for video games */
  PB_CLASS_ALARM,	/**< "Alarm" class for alarm clock */
  PB_CLASS_FLASH,	/**< "Flash" class for the Flash player */
  PB_CLASS_SYSTEM,      /**> "Systemsound" class desktop, etc. sounds */
  PB_CLASS_INPUT,       /**> "Inputsound" class for keypress, touchscreen */
  PB_CLASS_LAST,
};

enum pb_bt_override_status_e {
    BT_OVERRIDE_DISCONNECTED = -1,
    BT_OVERRIDE_OFF = 0,
    BT_OVERRIDE_ON = 1
};

enum pb_state_e	pb_string_to_state		(const char		*state);
const char *	pb_state_to_string		(enum pb_state_e	pb_state);

enum pb_class_e	pb_string_to_class		(const char		*aclass);
const char *	pb_class_to_string		(enum pb_class_e	pb_class);

PB_END_DECLS

#endif	/* PLAYBACK_TYPES_H_ */
