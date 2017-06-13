#include <dbus/dbus.h>
#include <string.h>

#include "libplayback/playback.h"
#include "playback-dbus.h"

static PBBluetoothCb _bluetooth_cb = NULL;
static void *_bluetooth_data = NULL;

static DBusHandlerResult
_bluetooth_override_filter(DBusConnection *connection, DBusMessage *message,
                           void *user_data)
{
  if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL &&
      !strcmp(dbus_message_get_interface(message),
              DBUS_PLAYBACK_MANAGER_INTERFACE) &&
      !strcmp(dbus_message_get_member(message), DBUS_BLUETOOTH_SIGNAL) )
  {
    DBusError error;
    dbus_int32_t status;

    dbus_error_init(&error);
    dbus_message_get_args(message, &error,
                          DBUS_TYPE_INT32, &status,
                          DBUS_TYPE_INVALID);

    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    else if (_bluetooth_cb)
      _bluetooth_cb(status, NULL, _bluetooth_data);
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void
pb_set_bluetooth_override_cb(DBusConnection *connection,
                             PBBluetoothCb bluetooth_cb,
                             void *data)
{
  if (bluetooth_cb)
  {
    _bluetooth_cb = bluetooth_cb;
    _bluetooth_data = data;
    dbus_connection_add_filter(connection, _bluetooth_override_filter, 0, 0);
  }
}

static void
_request_override_reply(DBusPendingCall *pending, void *user_data)
{
  DBusMessage *reply;

  if (!pending)
    return;

  reply = dbus_pending_call_steal_reply(pending);
  dbus_pending_call_unref(pending);

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR && _bluetooth_cb)
    _bluetooth_cb(FALSE, dbus_message_get_error_name(reply), _bluetooth_data);

  dbus_message_unref(reply);
}

int
pb_req_bluetooth_override(DBusConnection *connection,
                          int override)
{
  dbus_bool_t ovr = override ? TRUE : FALSE;
  DBusMessage *message;
  DBusPendingCall *pending;
  int rv = FALSE;

  message = dbus_message_new_method_call(DBUS_PLAYBACK_MANAGER_SERVICE,
                                         DBUS_PLAYBACK_MANAGER_PATH,
                                         DBUS_PLAYBACK_MANAGER_INTERFACE,
                                         DBUS_PLAYBACK_REQ_BLUETOOTH_METHOD);
  if (!message)
    return FALSE;

  dbus_message_append_args(message,
                           DBUS_TYPE_BOOLEAN, &ovr,
                           DBUS_TYPE_INVALID);

  if (dbus_connection_send_with_reply(connection, message, &pending, -1))
  {
    dbus_pending_call_set_notify(pending, _request_override_reply, NULL, NULL);
    rv = TRUE;
  }

  dbus_message_unref(message);

  return rv;
}

static void
_get_override_reply(DBusPendingCall *pending, void *user_data)
{
  DBusMessage *reply;

  if (!pending)
    return;

  reply = dbus_pending_call_steal_reply(pending);
  dbus_pending_call_unref(pending);

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR && _bluetooth_cb)
    _bluetooth_cb(FALSE, dbus_message_get_error_name(reply), _bluetooth_data);

  dbus_message_unref(reply);
}

int
pb_get_bluetooth_override(DBusConnection *connection)
{
  DBusMessage *message;
  DBusPendingCall *pending;
  int rv = FALSE;

  message = dbus_message_new_method_call(DBUS_PLAYBACK_MANAGER_SERVICE,
                                         DBUS_PLAYBACK_MANAGER_PATH,
                                         DBUS_PLAYBACK_MANAGER_INTERFACE,
                                         DBUS_PLAYBACK_GET_BLUETOOTH_METHOD);
  if (!message)
    return FALSE;

  if (dbus_connection_send_with_reply(connection, message, &pending, -1))
  {
    dbus_pending_call_set_notify(pending, _get_override_reply, NULL, NULL);
    rv = TRUE;
  }

  dbus_message_unref(message);

  return rv;
}
