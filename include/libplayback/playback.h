/*
** Playback manager - client library implementation (using D-Bus)
** (c) Nokia MM - Makoto Sugano, 2007
**
*/

#ifndef PLAYBACK_H_
# define PLAYBACK_H_

#include <dbus/dbus.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#include <libplayback/playback-macros.h>
#include <libplayback/playback-types.h>

/**
 * @namespace PB
 * @brief PB maemo playback object namespace
 */
PB_BEGIN_DECLS

/**
 * pb_playback_t:
 *
 * Opaque  structure  that acts  as  a  RPC  object for  the  playback
 * management.   (hide the  D-Bus  interface implementation,  methods,
 * signals, introspection...)
 */
typedef struct pb_playback_s	pb_playback_t;

/**
 * pb_req_t:
 *
 * A request handler (a private data that enable tracking of messages)
 */
typedef struct pb_req_s		pb_req_t;

/**
 * PBStateRequest:
 * @param[out] pb the playback object
 * @param[out] req_state the requested state
 * @param[out] ext_req the request from external source (usually the manager)
 * @param[out] data the pointer associated to the callback (in pb_playback_new)
 *
 * MANDATORY handler that  will be called when the  playback should be
 * changed  to  the  @req_state.   The  actual  state  change  can  be
 * asynchronous.   Each request  has a  @ext_req. This  id  enable the
 * caller to discard/change the  request.  Whenever a request has been
 * completed,  pb_playback_req_completed  should  be called  with  the
 * pb_req_t  handler.   Meanwhile,  an  ext_req can  be  discarded  or
 * changed  if the  function is  called multiple  times with  the same
 * ext_req.  Behavior of the PLAYBACK  state is undefined in this case
 * (wether the state change should be reverted or not)
 */

typedef void	(* PBStateRequest)		(pb_playback_t *pb, enum pb_state_e req_state, pb_req_t* ext_req, void *data);

/**
 * PBStateHint:
 * @param[out] pb the playback object
 * @param[out] allowed_state an array of boolean (ie: allowed_state[PB_STATE_PLAY])
 *
 * Notify to the application new allowed state set.
 */
typedef void	(* PBStateHint)			(pb_playback_t *pb, const int allowed_state[], void *data);

/**
 * PBOverrideCb:
 * @param[out] override Boolean state of the privacy override
 * @param[out] error Error string or NULL if no errors
 * @param[out] data The pointer associated to the callback
 *
 * Notify the application about the privacy override status.
 */
typedef void (* PBPrivacyCb) (int override, const char *error, void *data);

/**
 * PBMuteCb:
 * @param[out] mute Boolean state of the mute setting
 * @param[out] error Error string or NULL if no errors
 * @param[out] data The pointer associated to the callback
 *
 * Notify the application about the muting status.
 */
typedef void (* PBMuteCb) (int mute, const char *error, void *data);

/**
 * PBBluetoothCb:
 * @param[out] override Boolean state of the bluetooth override
 * @param[out] error Error string or NULL if no errors
 * @param[out] data The pointer associated to the callback
 *
 * Notify the application about the bluetooth override status.
 */
typedef void (* PBBluetoothCb) (enum pb_bt_override_status_e, const char *error, void *data);


/* FIXME: why is PB_STATE_NONE if denied? Why doesn't the manager just
 * tell the correct state? */
/**
 * PBStateReply:
 * @param[out] pb the playback object
 * @param[out] granted_state the state granted (or PB_STATE_NONE if denied)
 * @param[out] reason the human readable string explanation if request failed
 * @param[out] req request handler associated with the state_request() call
 * @param[out] data the pointer associated with the callback (from pb_playback_request_state)
 *
 * Reply  from a  pb_playback_request_state()  call.  If  a reason  is
 * given, then the granted_state should be PB_STATE_NONE: it means the
 * request has not been granted.  The reason is a user readable string
 * given to explain the request denied.  Each reply is identify with a
 * pb_req_t  handler that  corresponds  to the  returned  handler from  the
 * pb_playback_request_state().
 *
 * @see pb_playback_request_state
 */

typedef void	(* PBStateReply)		(pb_playback_t *pb, enum pb_state_e granted_state, const char *reason,
						 pb_req_t* req, void *data);

/**
 * pb_playback_new_2:
 * @param[in] connection an initialized DBusConnection
 * @param[in] pb_class the playback class (eg. PB_CLASS_VOIP)
 * @param[in] flags Request domain (eg. audio, video, audio rec, video rec)
 * @param[in] pb_state the playback state (eg. PB_STATE_STOP)
 * @param[in] state_req_handler Mandatory handler for state change requests
 * @param[in] data Pointer associated with the handler
 * @return a new playback object
 *
 * A client  should declare the different playbacks  (for example, the
 * different pipelines, or tracks: voice, music, event...)  A playback
 * *must be* controllable and provide a req_state_handler.
 */
pb_playback_t *	pb_playback_new_2			(DBusConnection *connection,
						 enum pb_class_e pb_class, uint32_t flags, enum pb_state_e pb_state,
						 PBStateRequest state_req_handler, void *data);

/**
 * pb_playback_new:
 * @param[in] connection an initialized DBusConnection
 * @param[in] pb_class the playback class (eg. PB_CLASS_VOIP)
 * @param[in] pb_state the playback state (eg. PB_STATE_STOP)
 * @param[in] state_req_handler Mandatory handler for state change requests
 * @param[in] data Pointer associated with the handler
 * @return a new playback object
 *
 * A client  should declare the different playbacks  (for example, the
 * different pipelines, or tracks: voice, music, event...)  A playback
 * *must be* controllable and provide a req_state_handler.
 */
pb_playback_t *	pb_playback_new			(DBusConnection *connection,
						 enum pb_class_e pb_class, enum pb_state_e pb_state,
						 PBStateRequest state_req_handler, void *data);

void		pb_playback_set_state_hint	(pb_playback_t *pb, PBStateHint state_hint_handler, void *data);

/**
 * pb_playback_req_state:
 * @param[in] pb the playback object
 * @param[in] pb_state the state the playback wants to be in
 * @param[in] granted_reply the callback that get the answer
 * @param[in] data user data associated with the callback
 * @return a request handler
 *
 * Whenever  the application wants  to change  the playback  state, it
 * should ask for a state change.  The state_reply will be called with
 * the granted_state.   Each state request is identified  by a request
 * handler returned from  the function call.  You can  also discard it
 * the request by calling pb_playback_req_discarded.
 * (or force the state change by calling pb_playback_req_completed)
 */
pb_req_t*	pb_playback_req_state		(pb_playback_t *pb, enum pb_state_e pb_state, PBStateReply state_reply, void *data);

/* returns only error status, the current privacy override status comes
 * to the callback. */

/**
 * pb_get_privacy_override:
 * @param[in] connection D-Bus Connection
 * @return success value indicating whether starting the query
 * succeeded. Note that this is not the privacy override status!
 *
 * The privacy override status will be returned as a parameter to a
 * call to the (previously defined) privacy callback.
 */
int pb_get_privacy_override(DBusConnection *connection);

/**
 * pb_req_privacy_override:
 * @param[in] connection D-Bus Connection
 * @param[in] override Desired privacy override status (0 if false, otherwise true)
 * @return success value indicating whether starting the request
 * succeeded. Note that this is not the privacy override status!
 *
 * The privacy override status will be returned as a parameter to a
 * call to the (previously defined) privacy callback.
 */
int pb_req_privacy_override(DBusConnection *connection, int override);

/**
 * pb_get_mute:
 * @param[in] connection D-Bus Connection
 * @return success value indicating whether starting the query
 * succeeded. Note that this is not the current mute status!
 *
 * The mute status will be returned as a parameter to a call to the
 * (previously defined) muting callback. You need to set a
 * callback before using the pb_get_mute_override function.

 */
int pb_get_mute(DBusConnection *connection);

/**
 * pb_req_mute:
 * @param[in] connection D-Bus Connection
 * @param[in] override Desired mute status (0 if false, otherwise true)
 * @return success value indicating whether starting the request
 * succeeded. Note that this is not the mute status!
 *
 * The mute status will be returned as a parameter to a
 * call to the (previously defined) muting callback. You need to set a
 * callback before using the pb_get_mute function.
 */
int pb_req_mute(DBusConnection *connection, int mute);

/**
 * pb_get_bluetooth_override:
 * @param[in] connection D-Bus Connection
 * @return success value indicating whether starting the query
 * succeeded. Note that this is not the bluetooth override status!
 *
 * The bluetooth override status will be returned as a parameter to a
 * call to the (previously defined) bluetooth callback.
 *
 * The bluetooth override is used when application wants to move the
 * audio from a bluetooth sink to the master device.
 */
int pb_get_bluetooth_override(DBusConnection *connection);

/**
 * pb_req_bluetooth_override:
 * @param[in] connection D-Bus Connection
 * @param[in] override Desired bluetooth override status (0 if false, otherwise true)
 * @return success value indicating whether starting the request
 * succeeded. Note that this is not the bluetooth override status!
 *
 * The bluetooth override status will be returned as a parameter to a
 * call to the (previously defined) bluetooth callback.
 */
int pb_req_bluetooth_override(DBusConnection *connection, int override);
/* setters for the callbacks */

/**
 * pb_set_privacy_override_cb:
 * @param[in] connection d-bus connection
 * @param[in] privacy_cb the callback for privacy override status messages
 * @param[in] data user data for the callback
 *
 * The callback and user data are singleton objects: the settings are
 * overwritten if this function is called twice. 
 */
void pb_set_privacy_override_cb(DBusConnection *connection, PBPrivacyCb privacy_cb, void *data);

/**
 * pb_set_mute_cb:
 * @param[in] connection d-bus connection
 * @param[in] mute_cb the callback for muting status messages
 * @param[in] data user data for the callback
 *
 * The callback and user data are singleton objects: the settings are
 * overwritten if this function is called twice. 
 */
void pb_set_mute_cb(DBusConnection *connection, PBMuteCb mute_cb, void *data);

/**
 * pb_set_bluetooth_override_cb:
 * @param[in] connection d-bus connection
 * @param[in] bluetooth_cb the callback for bluetooth override status messages
 * @param[in] data user data for the callback
 *
 * The callback and user data are singleton objects: the settings are
 * overwritten if this function is called twice. 
 */
void pb_set_bluetooth_override_cb(DBusConnection *connection, PBBluetoothCb bluetooth_cb, void *data);

/**
 * pb_playback_set_pid:
 * @param[in] pb the playback object
 * @param[in] pid Process id of the process that does the actual playing
 *
 * If the process doing the playback is not the process that creates the
 * playback object, the pid needs to be set with this function before
 * the pb_playback_request call. This allows audio subsystem to map the
 * streams to playback objects.
 */
void pb_playback_set_pid(pb_playback_t *pb, pid_t pid);
void pb_playback_set_stream(pb_playback_t *pb, char *stream);

int		pb_playback_req_discarded	(pb_playback_t *pb, pb_req_t *req, const char *reason);
int		pb_playback_req_completed	(pb_playback_t *pb, pb_req_t *req);

void		pb_playback_destroy		(pb_playback_t *pb);

PB_END_DECLS

#endif /* !PLAYBACK_H_ */
