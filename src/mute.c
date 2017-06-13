#include "libplayback/playback.h"

#include <dbus/dbus.h>

static PBMuteCb _mute_cb = NULL;
static void *_mute_data = NULL;

static DBusHandlerResult
_mute_filter(DBusConnection *connection, DBusMessage *message, void *user_data)
{
  if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL &&
      !strcmp(dbus_message_get_interface(message),
              "org.maemo.Playback.Manager") &&
      !strcmp(dbus_message_get_member(message), "Mute"))
  {
    DBusError error;
    dbus_bool_t mute;

    dbus_error_init(&error);
    dbus_message_get_args(message, &error,
                          DBUS_TYPE_BOOLEAN, &mute,
                          DBUS_TYPE_INVALID);

    if ( dbus_error_is_set(&error) )
      dbus_error_free(&error);
    else if ( _mute_cb )
      _mute_cb(mute == TRUE, NULL, _mute_data);
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void
pb_set_mute_cb(DBusConnection *connection,
               PBMuteCb mute_cb,
               void *data)
{
  if (mute_cb)
  {
    _mute_cb = mute_cb;
    _mute_data = data;
    dbus_connection_add_filter(connection, _mute_filter, 0, 0);
  }
}

static void
_request_mute_reply(DBusPendingCall *pending, void *user_data)
{
  DBusMessage *reply;

  if (!pending)
    return;

  reply = dbus_pending_call_steal_reply(pending);
  dbus_pending_call_unref(pending);

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR && _mute_cb)
    _mute_cb(0, dbus_message_get_error_name(reply), _mute_data);

  dbus_message_unref(reply);
}

int
pb_req_mute(DBusConnection *connection,
            int mute)
{
  dbus_bool_t m = mute ? TRUE :FALSE;
  DBusMessage *message;
  DBusPendingCall *pending;
  int rv = FALSE;

  message = dbus_message_new_method_call("org.maemo.Playback.Manager",
                                         "/org/maemo/Playback/Manager",
                                         "org.maemo.Playback.Manager",
                                         "RequestMute");

  if (!message)
    return FALSE;

  dbus_message_append_args(message,
                           DBUS_TYPE_BOOLEAN, &m,
                           DBUS_TYPE_INVALID);

  if (dbus_connection_send_with_reply(connection, message, &pending, -1))
  {
    dbus_pending_call_set_notify(pending, _request_mute_reply, NULL, NULL);
    rv = TRUE;
  }

  dbus_message_unref(message);

  return rv;
}

static void
_get_mute_reply(DBusPendingCall *pending, void *user_data)
{
  DBusMessage *reply;

  if (!pending)
    return;

  reply = dbus_pending_call_steal_reply(pending);
  dbus_pending_call_unref(pending);

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR && _mute_cb)
    _mute_cb(0, dbus_message_get_error_name(reply), _mute_data);

  dbus_message_unref(reply);
}

int
pb_get_mute(DBusConnection *connection)
{
  DBusMessage *message;
  DBusPendingCall *pending;
  int rv = FALSE;

  message = dbus_message_new_method_call("org.maemo.Playback.Manager",
                                         "/org/maemo/Playback/Manager",
                                         "org.maemo.Playback.Manager",
                                         "GetMute");
  if (!message)
    return FALSE;

  if (dbus_connection_send_with_reply(connection, message, &pending, -1))
  {
    dbus_pending_call_set_notify(pending, _get_mute_reply, NULL, NULL);
    rv = TRUE;
  }

  dbus_message_unref(message);

  return rv;
}
