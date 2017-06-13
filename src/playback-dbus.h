#ifndef PLAYBACKDBUS_H
#define PLAYBACKDBUS_H

#include <dbus/dbus.h>

/* D-Bus errors */
#define DBUS_MAEMO_ERROR_PREFIX   "org.maemo.Error"

#define DBUS_MAEMO_ERROR_FAILED        DBUS_MAEMO_ERROR_PREFIX ".Failed"
#define DBUS_MAEMO_ERROR_DENIED        DBUS_MAEMO_ERROR_PREFIX ".RequestDenied"
#define DBUS_MAEMO_ERROR_DISCARDED     DBUS_MAEMO_ERROR_PREFIX ".RequestDiscarded"
#define DBUS_MAEMO_ERROR_INVALID_ARGS  DBUS_MAEMO_ERROR_PREFIX ".InvalidArguments"
#define DBUS_MAEMO_ERROR_INVALID_IFACE DBUS_MAEMO_ERROR_PREFIX ".InvalidInterface"
#define DBUS_MAEMO_ERROR_INTERNAL_ERR  DBUS_MAEMO_ERROR_PREFIX ".InternalError"

/* D-Bus service names */
#define DBUS_PLAYBACK_SERVICE              "org.maemo.Playback"
#define DBUS_PLAYBACK_MANAGER_SERVICE      DBUS_PLAYBACK_SERVICE ".Manager"

/* D-Bus interface names */
#define DBUS_ADMIN_INTERFACE               "org.freedesktop.DBus"
#define DBUS_PLAYBACK_INTERFACE            "org.maemo.Playback"
#define DBUS_PLAYBACK_MANAGER_INTERFACE    DBUS_PLAYBACK_INTERFACE ".Manager"

/* D-Bus signal & method names */
#define DBUS_POLICY_NEW_SESSION            "NewSession"

#define DBUS_INFO_SIGNAL                   "info"
#define DBUS_NAME_OWNER_CHANGED_SIGNAL     "NameOwnerChanged"
#define DBUS_HELLO_SIGNAL                  "Hello"
#define DBUS_GOODBYE_SIGNAL                "Goodbye"
#define DBUS_NOTIFY_SIGNAL                 "Notify"
#define DBUS_PRIVACY_SIGNAL                "PrivacyOverride"
#define DBUS_BLUETOOTH_SIGNAL              "BluetoothOverride"
#define DBUS_MUTE_SIGNAL                   "Mute"

#define DBUS_PLAYBACK_REQ_STATE_METHOD     "RequestState"
#define DBUS_PLAYBACK_REQ_PRIVACY_METHOD   "RequestPrivacyOverride"
#define DBUS_PLAYBACK_REQ_BLUETOOTH_METHOD "RequestBluetoothOverride"
#define DBUS_PLAYBACK_REQ_MUTE_METHOD      "RequestMute"

#define DBUS_PLAYBACK_GET_ALLOWED_METHOD   "GetAllowedState"
#define DBUS_PLAYBACK_GET_PRIVACY_METHOD   "GetPrivacyOverride"
#define DBUS_PLAYBACK_GET_BLUETOOTH_METHOD "GetBluetoothOverride"
#define DBUS_PLAYBACK_GET_MUTE_METHOD      "GetMute"

/* D-Bus property names */
#define DBUS_PLAYBACK_STATE_PROP           "State"
#define DBUS_PLAYBACK_CLASS_PROP           "Class"
#define DBUS_PLAYBACK_PID_PROP             "Pid"
#define DBUS_PLAYBACK_FLAGS_PROP           "Flags"
#define DBUS_PLAYBACK_STREAM_PROP          "Stream"
#define DBUS_PLAYBACK_ALLOWED_STATE_PROP   "AllowedState"

/* D-Bus pathes */
#define DBUS_ADMIN_PATH                  "/org/freedesktop/DBus"
#define DBUS_PLAYBACK_MANAGER_PATH       "/org/maemo/Playback/Manager"

#endif /* PLAYBACKDBUS_H */
