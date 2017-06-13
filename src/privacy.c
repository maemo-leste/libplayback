#include <dbus/dbus.h>
#include <string.h>

#include "libplayback/playback.h"
#include "playback-dbus.h"


static PBPrivacyCb _privacy_cb = NULL;
static void *_privacy_data = NULL;

static DBusHandlerResult
_privacy_override_filter(DBusConnection *connection, DBusMessage *message,
                         void *user_data)
{
  DBusError error;
  dbus_bool_t override;

  if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL &&
      !strcmp(dbus_message_get_interface(message),
              DBUS_PLAYBACK_MANAGER_INTERFACE) &&
      !strcmp(dbus_message_get_member(message), DBUS_PRIVACY_SIGNAL))
  {
    dbus_error_init(&error);
    dbus_message_get_args(message, &error,
                          DBUS_TYPE_BOOLEAN, &override,
                          DBUS_TYPE_INVALID);

    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    else if (_privacy_cb)
      _privacy_cb(override == TRUE, NULL, _privacy_data);
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void
pb_set_privacy_override_cb(DBusConnection *connection,
                           PBPrivacyCb privacy_cb,
                           void *data)
{
  if (privacy_cb)
  {
    _privacy_cb = privacy_cb;
    _privacy_data = data;
    dbus_connection_add_filter(
          connection, _privacy_override_filter, NULL, NULL);
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

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR && _privacy_cb)
    _privacy_cb(FALSE, dbus_message_get_error_name(reply), _privacy_data);

  dbus_message_unref(reply);
}

int
pb_req_privacy_override(DBusConnection *connection,
                        int override)
{
  DBusMessage *message;
  DBusPendingCall *pending;
  dbus_bool_t ovr = override ? TRUE : FALSE;
  int rv = FALSE;

  message = dbus_message_new_method_call(DBUS_PLAYBACK_MANAGER_SERVICE,
                                         DBUS_PLAYBACK_MANAGER_PATH,
                                         DBUS_PLAYBACK_MANAGER_INTERFACE,
                                         DBUS_PLAYBACK_REQ_PRIVACY_METHOD);
  if (!message)
    return FALSE;

  dbus_message_append_args(message,
                           DBUS_TYPE_BOOLEAN, &ovr,
                           DBUS_TYPE_INVALID);

  if (dbus_connection_send_with_reply(connection, message, &pending, -1))
  {
    dbus_pending_call_set_notify( pending, _request_override_reply, NULL, NULL);
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

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR && _privacy_cb)
    _privacy_cb(FALSE, dbus_message_get_error_name(reply), _privacy_data);

    dbus_message_unref(reply);
}

int
pb_get_privacy_override(DBusConnection *connection)
{
  DBusMessage *message;
  DBusPendingCall *pending;
  int rv = FALSE;

  message = dbus_message_new_method_call(DBUS_PLAYBACK_MANAGER_SERVICE,
                                         DBUS_PLAYBACK_MANAGER_PATH,
                                         DBUS_PLAYBACK_MANAGER_INTERFACE,
                                         DBUS_PLAYBACK_GET_PRIVACY_METHOD);
  if (message)
  {
    if (dbus_connection_send_with_reply(connection, message, &pending, -1))
    {
      dbus_pending_call_set_notify(pending, _get_override_reply, NULL, NULL);
      rv = TRUE;
    }

    dbus_message_unref(message);
  }

  return rv;
}
