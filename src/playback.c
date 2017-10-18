#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "libplayback/playback.h"
#include "playback-dbus.h"

#define PLAYBACK_PATH "/org/maemo/playback%u"

static DBusHandlerResult _dbus_playback_message(DBusConnection *connection,
                                                DBusMessage *message,
                                                void *user_data);

static uint32_t object_id = 0;
static int name_requested = FALSE;
static DBusObjectPathVTable _dbus_playback_table =
{
  NULL,
  _dbus_playback_message,
  NULL,
  NULL,
  NULL,
  NULL
};

typedef struct pb_req_list_s pb_req_list_t;

struct pb_req_list_s
{
  struct pb_req_list_s *next;
  void *data;
};

typedef struct pbreq_listhead_s pbreq_listhead_t;

struct pbreq_listhead_s
{
  struct pb_req_list_s *first;
  struct pb_req_list_s *last;
};

struct pb_playback_s
{
  DBusConnection *connection;
  uint32_t object_id;
  enum pb_class_e pb_class;
  enum pb_state_e pb_state;
  PBStateRequest state_req_handler;
  void *state_req_handler_data;
  int allowed_state[3];
  PBStateHint state_hint_handler;
  void *state_hint_handler_data;
  uint32_t flags;
  pid_t pid;
  char *stream;
  pbreq_listhead_t req_list;
};

struct pb_req_s
{
  pb_playback_t *pb;
  DBusMessage *message;
  DBusPendingCall *pending;
  PBStateReply state_reply;
  enum pb_state_e pb_state;
  int finished;
  void *data;
};

static int
pb_list_empty(pbreq_listhead_t *list)
{
  return list->last == NULL;
}

static int
pb_list_append(pbreq_listhead_t *head,
               void *data)
{
  pb_req_list_t *item;

  if (data && (item = (pb_req_list_t *)calloc(1, sizeof(pb_req_list_t))))
  {
    item->data = data;

    if (pb_list_empty(head))
    {
      head->first = item;
      head->last = item;
    }
    else
    {
      head->last->next = item;
      head->last = item;
    }

    return TRUE;
  }

  return FALSE;
}

static void
pb_list_remove(pbreq_listhead_t *head,
               void *data)
{
  pb_req_list_t *l, *prev = NULL;

  if (!data)
    return;

  for(l = head->first; l; l = l->next)
  {
    if (l->data == data)
    {
      if (prev)
        prev->next = l->next;

      if (head->last == l)
      {
        head->last = prev;

        if (prev)
          prev->next = NULL;
      }

      if (head->first == l)
        head->first = l->next;

      free(l);
      break;
    }

    prev = l;
  }
}

static void
pb_list_free(pbreq_listhead_t *head)
{
    pb_req_list_t *l, *next;

    for (l = head->first; l; l = next)
    {
      next = l->next;
      free(l);
    }

    head->first = NULL;
    head->last = NULL;
}

static int
_add_property(DBusMessageIter *iter,
              const char *name,
              const char *val)
{
  DBusMessageIter valIter;
  DBusMessageIter dictIter;

  if (!dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL,
                                        &dictIter))
  {
    return FALSE;
  }

  if (!dbus_message_iter_append_basic(&dictIter, DBUS_TYPE_STRING, &name))
    return FALSE;

  if (!dbus_message_iter_open_container(&dictIter, DBUS_TYPE_VARIANT,
                                        DBUS_TYPE_STRING_AS_STRING, &valIter))
  {
    return FALSE;
  }

  if (!dbus_message_iter_append_basic(&valIter, DBUS_TYPE_STRING, &val))
    return FALSE;

  dbus_message_iter_close_container(&dictIter, &valIter);
  dbus_message_iter_close_container(iter, &dictIter);

  return TRUE;
}

static void
_playback_hello(pb_playback_t *pb)
{
  DBusMessage *message;
  char path[256];

  assert(pb != ((void *)0));

  snprintf(path, sizeof(path), PLAYBACK_PATH, pb->object_id);
  message = dbus_message_new_signal(path,
                                    DBUS_PLAYBACK_INTERFACE,
                                    DBUS_HELLO_SIGNAL);

  if (message)
  {
    dbus_connection_send(pb->connection, message, NULL);
    dbus_message_unref(message);
  }
}

static DBusHandlerResult
_nameowner_filter(DBusConnection *connection,
                  DBusMessage *message,
                  void *user_data)
{
  pb_playback_t *pb = (pb_playback_t *)user_data;


  if (!user_data)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL &&
      !strcmp(dbus_message_get_interface(message), DBUS_ADMIN_INTERFACE) &&
      !strcmp(dbus_message_get_member(message), DBUS_NAME_OWNER_CHANGED_SIGNAL))
  {
    const char *name, *old, *new;
    DBusError error;

    dbus_error_init(&error);
    dbus_message_get_args(message, &error,
                          DBUS_TYPE_STRING, &name,
                          DBUS_TYPE_STRING, &old,
                          DBUS_TYPE_STRING, &new,
                          DBUS_TYPE_INVALID);

    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    else if (name && old && new && *new &&
             !strcmp(name, DBUS_PLAYBACK_MANAGER_SERVICE))
    {
      _playback_hello(pb);
    }
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
_update_allowed_states(pb_playback_t *pb,
                       char **allowed_states,
                       int len)
{
  int i;

  pb->allowed_state[PB_STATE_NONE] = FALSE;
  pb->allowed_state[PB_STATE_STOP] = FALSE;
  pb->allowed_state[PB_STATE_PLAY] = FALSE;

  for (i = 0; i < len; i++)
    pb->allowed_state[pb_string_to_state(allowed_states[i])] = TRUE;

  if (pb->state_hint_handler)
    pb->state_hint_handler(pb, pb->allowed_state, pb->state_hint_handler_data);
}

static DBusHandlerResult
_allowed_state_filter(DBusConnection *connection,
                      DBusMessage *message,
                      void *user_data)
{
  pb_playback_t *pb = (pb_playback_t *)user_data;

  if (!pb)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL &&
      !strcmp(dbus_message_get_interface(message),
              DBUS_PLAYBACK_MANAGER_INTERFACE) &&
      !strcmp(dbus_message_get_member(message),
              DBUS_PLAYBACK_ALLOWED_STATE_PROP))
  {
    DBusError error;
    const char *cls;
    int len;
    char **states;

    dbus_error_init(&error);
    dbus_message_get_args(message, &error,
                          DBUS_TYPE_STRING, &cls,
                          DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &states, &len,
                          DBUS_TYPE_INVALID);

    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    else if (pb->pb_class == pb_string_to_class(cls))
    {
      _update_allowed_states(pb, states, len);
      dbus_free_string_array(states);
    }
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

pb_playback_t *
pb_playback_new_2(DBusConnection *connection,
                  enum pb_class_e pb_class,
                  uint32_t flags,
                  enum pb_state_e pb_state,
                  PBStateRequest state_req_handler,
                  void *data)
{
  pb_playback_t *pb;
  char path[256];
  DBusError error;

  assert(connection != ((void *)0) && state_req_handler != ((void *)0));

  pb = (pb_playback_t *)calloc(sizeof(pb_playback_t), 1);

  if (!pb)
    return NULL;

  pb->pb_state = pb_state;
  pb->pb_class = pb_class;
  pb->state_req_handler = state_req_handler;
  pb->object_id = object_id;
  pb->state_req_handler_data = data;
  pb->stream = NULL;
  pb->connection = connection;
  pb->flags = flags;
  pb->allowed_state[PB_STATE_NONE] = TRUE;
  pb->allowed_state[PB_STATE_STOP] = TRUE;
  pb->allowed_state[PB_STATE_PLAY] = TRUE;
  pb->pid = getpid();

  object_id += 1;

  if (!name_requested)
  {
    dbus_error_init(&error);

    if (dbus_bus_request_name(connection, DBUS_PLAYBACK_SERVICE, 0, &error) < 0)
      dbus_error_free(&error);
    else
      name_requested = TRUE;
  }

  dbus_error_init(&error);
  dbus_bus_add_match(connection,
                     "type='signal',interface='org.maemo.Playback.Manager',path='/org/maemo/Playback/Manager'",
                     &error);

  if (dbus_error_is_set(&error))
    dbus_error_free(&error);

  dbus_error_init(&error);
  dbus_bus_add_match(connection,
                     "type='signal', "
                     "sender='org.freedesktop.DBus',path='/org/freedesktop/DBus', "
                     "interface='org.freedesktop.DBus', "
                     "member='NameOwnerChanged',arg0='org.maemo.Playback.Manager'",
                     &error);

  if (dbus_error_is_set(&error))
    dbus_error_free(&error);

  dbus_connection_add_filter(connection, _nameowner_filter, pb, NULL);
  dbus_connection_add_filter(connection, _allowed_state_filter, pb, NULL);
  snprintf(path, sizeof(path), PLAYBACK_PATH, pb->object_id);
  dbus_connection_register_object_path(
        connection, path, &_dbus_playback_table, pb);
  _playback_hello(pb);

  return pb;
}

pb_playback_t *
pb_playback_new(DBusConnection *connection,
                enum pb_class_e pb_class,
                enum pb_state_e pb_state,
                PBStateRequest state_req_handler,
                void *data)
{
  return pb_playback_new_2(connection, pb_class, PB_FLAG_AUDIO, pb_state,
                           state_req_handler, data);
}

void
pb_playback_destroy(pb_playback_t *pb)
{
  pb_req_t *req;
  DBusMessage *message;
  char path[256];

  if (!pb)
    return;

  dbus_connection_remove_filter(pb->connection, _nameowner_filter, pb);
  dbus_connection_remove_filter(pb->connection, _allowed_state_filter, pb);
  req = pb_playback_req_state(pb, PB_STATE_STOP, NULL, NULL);
  pb_playback_req_completed(pb, req);
  snprintf(path, sizeof(path), PLAYBACK_PATH, pb->object_id);
  dbus_connection_unregister_object_path(pb->connection, path);
  message = dbus_message_new_signal(path,
                                    DBUS_PLAYBACK_INTERFACE,
                                    DBUS_GOODBYE_SIGNAL);

  if (message)
  {
    dbus_connection_send(pb->connection, message, 0);
    dbus_message_unref(message);
  }

  free(pb->stream);
  pb->stream = NULL;
  pb_list_free(&pb->req_list);
}

static pb_req_t *
_pb_request_new(pb_playback_t *pb)
{
  pb_req_t *req;

  assert(pb != ((void *)0));

  req = (pb_req_t *)calloc(sizeof(pb_req_t), 1);

  if (req)
    req->pb = pb;

  return req;
}

void
pb_playback_set_stream(pb_playback_t *pb,
                       char *stream)
{
  printf("> set_stream '%s'\n", stream);

  if (stream)
  {
    free(pb->stream);
    pb->stream = strdup(stream);
  }
}

void
pb_playback_set_pid(pb_playback_t *pb,
                    pid_t pid)
{
  pb->pid = pid;
}

static void
_request_state_reply(DBusPendingCall *pending,
                     void *user_data)
{
  pb_req_t *req = (pb_req_t *)user_data;
  DBusMessage *reply;
  DBusError error;
  const char *state;

  if (!pending || !req || req->pending != pending)
    return;

  reply = dbus_pending_call_steal_reply(pending);
  dbus_error_init(&error);

  if (dbus_set_error_from_message(&error, reply))
  {
    if (req->state_reply)
    {
      if (dbus_error_has_name(&error, DBUS_ERROR_SERVICE_UNKNOWN))
      {
        req->finished = TRUE;
        req->state_reply(req->pb, req->pb_state, NULL, req, req->data);
      }
      else
        req->state_reply(req->pb, PB_STATE_NONE, error.message, req, req->data);
    }

    dbus_error_free(&error);
  }
  else
  {
    if (dbus_message_get_args(reply, &error,
                              DBUS_TYPE_STRING, &state,
                              DBUS_TYPE_INVALID))
    {
      if (req->state_reply)
      {
        req->finished = TRUE;
        req->state_reply(
              req->pb, pb_string_to_state(state), NULL, req, req->data);
      }
    }
    else if (req->state_reply)
    {
        req->state_reply(req->pb, PB_STATE_NONE, "Invalid reply arguments",
                         req, req->data);
    }
  }

  dbus_message_unref(reply);
}

pb_req_t *
pb_playback_req_state(pb_playback_t *pb,
                      enum pb_state_e pb_state,
                      PBStateReply state_reply,
                      void *data)
{
  pb_req_t *req = NULL;
  const char *pb_state_string;
  const char *stream;
  char _path[256];
  const char *path = _path;
  char _pid[64];
  const char *pid = _pid;
  DBusMessage *message;

  if (!pb || !state_reply)
    return NULL;

  message = dbus_message_new_method_call(DBUS_PLAYBACK_MANAGER_SERVICE,
                                         DBUS_PLAYBACK_MANAGER_PATH,
                                         DBUS_PLAYBACK_MANAGER_INTERFACE,
                                         DBUS_PLAYBACK_REQ_STATE_METHOD);

  if (!message)
    return NULL;

  if (!(req = _pb_request_new(pb)))
  {
    state_reply(pb, PB_STATE_NONE,
                "Unable to create the request handler", NULL, data);
    dbus_message_unref(message);
    return NULL;
  }

  snprintf(_pid, sizeof(_pid), "%ld", (long)pb->pid);
  pb_state_string = pb_state_to_string(pb_state);
  stream = pb->stream;

  if (!stream)
    stream = "";

  snprintf(_path, sizeof(_path), PLAYBACK_PATH, pb->object_id);
  dbus_message_append_args(message,
                           DBUS_TYPE_OBJECT_PATH, &path,
                           DBUS_TYPE_STRING, &pb_state_string,
                           DBUS_TYPE_STRING, &pid,
                           DBUS_TYPE_STRING, &stream,
                           DBUS_TYPE_INVALID);
  req->pb_state = pb_state;
  req->state_reply = state_reply;
  req->data = data;
  req->finished = FALSE;

  if (pb_list_empty(&pb->req_list))
  {
    DBusPendingCall *pending;

    pb_list_append(&pb->req_list, req);

    if (dbus_connection_send_with_reply(pb->connection, message, &pending, -1))
    {
      req->pending = pending;
      dbus_pending_call_set_notify(pending, _request_state_reply, req, NULL);
    }
    else
    {
      state_reply(
            pb, PB_STATE_NONE, "Error while sending the message call "
            DBUS_PLAYBACK_MANAGER_INTERFACE"."DBUS_PLAYBACK_REQ_STATE_METHOD,
            req, data);
      req = NULL;
    }
  }
  else
    pb_list_append(&pb->req_list, req);

  dbus_message_unref(message);

  return req;
}

static void
_playback_signal_state(pb_playback_t *pb)
{
  const char *state_string = pb_state_to_string(pb->pb_state);
  DBusMessage *message;
  const char *prop = DBUS_PLAYBACK_STATE_PROP;
  const char *iface = DBUS_PLAYBACK_INTERFACE;
  char path[256];

  snprintf(path, sizeof(path), PLAYBACK_PATH, pb->object_id);
  message = dbus_message_new_signal(path,
                                    DBUS_INTERFACE_PROPERTIES,
                                    DBUS_NOTIFY_SIGNAL);

  if (message)
  {
    dbus_message_append_args(message,
                             DBUS_TYPE_STRING, &iface,
                             DBUS_TYPE_STRING, &prop,
                             DBUS_TYPE_STRING, &state_string,
                             DBUS_TYPE_INVALID);
    dbus_connection_send(pb->connection, message, NULL);
    dbus_message_unref(message);
  }
}

static void
_get_allowed_state_reply(DBusPendingCall *pending,
                         void *user_data)
{
  DBusMessage *message;
  DBusError error;
  char **states;
  int len;

  if (!pending || !user_data)
    return;

  dbus_error_init(&error);
  message = dbus_pending_call_steal_reply(pending);

  if (!dbus_set_error_from_message(&error, message) &&
      dbus_message_get_args(message, &error,
                            DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &states, &len,
                            DBUS_TYPE_INVALID))
  {
    _update_allowed_states(user_data, states, len);
    dbus_free_string_array(states);
  }
  else
    dbus_error_free(&error);

  dbus_message_unref(message);
  dbus_pending_call_unref(pending);
}

void
pb_playback_set_state_hint(pb_playback_t *pb,
                           PBStateHint state_hint_handler,
                           void *data)
{
  DBusMessage *message;
  DBusPendingCall *pending;

  if (!pb || !state_hint_handler)
    return;

  pb->state_hint_handler = state_hint_handler;
  pb->state_hint_handler_data = data;

  message = dbus_message_new_method_call(
        DBUS_PLAYBACK_MANAGER_SERVICE,
        DBUS_PLAYBACK_MANAGER_PATH,
        DBUS_PLAYBACK_MANAGER_INTERFACE,
        DBUS_PLAYBACK_GET_ALLOWED_METHOD);

  if (message)
  {
    char _path[256];
    const char *path = _path;

    snprintf(_path, sizeof(_path), PLAYBACK_PATH, pb->object_id);
    dbus_message_append_args(message,
                             DBUS_TYPE_OBJECT_PATH, &path,
                             DBUS_TYPE_INVALID);

    if (dbus_connection_send_with_reply(pb->connection, message, &pending, -1))
      dbus_pending_call_set_notify(pending, _get_allowed_state_reply, pb, NULL);

    dbus_message_unref(message);
  }
}

static DBusHandlerResult
_dbus_error_reply(DBusConnection *connection,
                  DBusMessage *message,
                  const char *error,
                  const char *reason)
{
  DBusMessage *err_msg;

    if (!connection || !message || !reason)
    return DBUS_HANDLER_RESULT_HANDLED;

  err_msg = dbus_message_new_error(message, error, reason);

  if (err_msg)
  {
    dbus_connection_send(connection, err_msg, NULL);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NEED_MEMORY;
}

static void
process_request_list(pb_playback_t *pb)
{
  if (!pb_list_empty(&pb->req_list))
  {
    pb_req_t *req = (pb_req_t *)pb->req_list.first->data;
    DBusMessage *message =
        dbus_message_new_method_call(DBUS_PLAYBACK_MANAGER_SERVICE,
                                     DBUS_PLAYBACK_MANAGER_PATH,
                                     DBUS_PLAYBACK_MANAGER_INTERFACE,
                                     DBUS_PLAYBACK_REQ_STATE_METHOD);

    if (message)
    {
      const char *stream = pb->stream;
      const char *new_state = pb_state_to_string(req->pb_state);
      char _path[256];
      const char *path = _path;
      char _pid[64];
      const char *pid = _pid;

      snprintf(_path, sizeof(_path), PLAYBACK_PATH, pb->object_id);
      snprintf(_pid, sizeof(_pid), "%ld", (long)pb->pid);

      if (!stream)
        stream = "";

      dbus_message_append_args(message,
                               DBUS_TYPE_OBJECT_PATH, &path,
                               DBUS_TYPE_STRING, &new_state,
                               DBUS_TYPE_STRING, &pid,
                               DBUS_TYPE_STRING, &stream,
                               DBUS_TYPE_INVALID);

      if (dbus_connection_send_with_reply(
            pb->connection, message, &req->pending, -1))
      {
        dbus_pending_call_set_notify(
              req->pending, _request_state_reply, req, NULL);
        dbus_message_unref(message);
        return;
      }

      dbus_message_unref(message);
    }

    if (req->state_reply)
      req->state_reply(req->pb, PB_STATE_NONE,
                       "Failed to send a queued state request", req, req->data);
  }
}

int
pb_playback_req_discarded(pb_playback_t *pb,
                          pb_req_t *req,
                          const char *reason)
{
  if (!pb || !req)
    return TRUE;

  pb->pb_state = PB_STATE_NONE;

  if (req->pb != pb)
    return FALSE;

  if (req->message)
  {
    _dbus_error_reply(pb->connection, req->message,
                      DBUS_MAEMO_ERROR_DISCARDED, reason);
    req->message = NULL;
    _playback_signal_state(pb);
  }
  else
  {
    _playback_signal_state(pb);
    pb_list_remove(&pb->req_list, req);
    process_request_list(pb);
  }

  if (req->pending)
  {
    if (!dbus_pending_call_get_completed(req->pending))
      dbus_pending_call_cancel(req->pending);

    dbus_pending_call_unref(req->pending);
  }

  free(req);

  return TRUE;
}

int
pb_playback_req_completed(pb_playback_t *pb,
                          pb_req_t *req)
{
  if (!pb || !req)
    return TRUE;

  if (req->pb != pb)
    return FALSE;

  if (req->finished)
    pb->pb_state = req->pb_state;

  if (req->message)
  {
    DBusMessage *message = dbus_message_new_method_return(req->message);

    if (message)
    {
      dbus_connection_send(pb->connection, message, 0);
      dbus_message_unref(message);
    }

    req->message = NULL;
    _playback_signal_state(pb);
  }
  else
  {
    _playback_signal_state(pb);
    pb_list_remove(&pb->req_list, req);
    process_request_list(pb);
  }

  if (req->pending)
  {
    if (!dbus_pending_call_get_completed(req->pending))
      dbus_pending_call_cancel(req->pending);

    dbus_pending_call_unref(req->pending);
  }

  free(req);

  return TRUE;
}

static DBusHandlerResult
_playback_get_string_reply(pb_playback_t *pb,
                           DBusMessage *message,
                           const char *s)
{
  DBusMessage *msg = dbus_message_new_method_return(message);

  if (!msg)
    return DBUS_HANDLER_RESULT_NEED_MEMORY;

  dbus_message_append_args(msg,
                           DBUS_TYPE_STRING, &s,
                           DBUS_TYPE_INVALID);
  dbus_connection_send(pb->connection, msg, 0);
  dbus_message_unref(msg);

  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
_playback_get(pb_playback_t *pb, DBusMessage *message)
{
  DBusError error;
  char *prop;
  char *iface;

  dbus_error_init(&error);
  dbus_message_get_args(message, &error,
                        DBUS_TYPE_STRING, &iface,
                        DBUS_TYPE_STRING, &prop,
                        DBUS_TYPE_INVALID);

  if (dbus_error_is_set(&error))
  {
    _dbus_error_reply(pb->connection, message, DBUS_MAEMO_ERROR_INVALID_ARGS,
                      error.message);
    dbus_error_free(&error);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (strcmp(iface, DBUS_PLAYBACK_INTERFACE))
  {
    return _dbus_error_reply(pb->connection, message,
                             DBUS_MAEMO_ERROR_INVALID_IFACE, "");
  }

  if (!strcmp(prop, DBUS_PLAYBACK_STATE_PROP))
  {
    return _playback_get_string_reply(pb, message,
                                      pb_state_to_string(pb->pb_state));
  }
  else if (!strcmp(prop, DBUS_PLAYBACK_CLASS_PROP))
  {
    return _playback_get_string_reply(pb, message,
                                      pb_class_to_string(pb->pb_class));
  }
  else if (!strcmp(prop, DBUS_PLAYBACK_PID_PROP))
  {
    char pid[64];

    snprintf(pid, sizeof(pid), "%ld", (long)pb->pid);
    return _playback_get_string_reply(pb, message, pid);
  }
  else if (!strcmp(prop, DBUS_PLAYBACK_FLAGS_PROP))
  {
    char flags[64];

    snprintf(flags, sizeof(flags), "%u", pb->flags);
    return _playback_get_string_reply(pb, message, flags);
  }
  else if (!strcmp(prop, DBUS_PLAYBACK_STREAM_PROP))
  {
    const char *stream = pb->stream;

    if (!stream)
      stream = "";

    return _playback_get_string_reply(pb, message, stream);
  }
  else if (!strcmp(prop, DBUS_PLAYBACK_ALLOWED_STATE_PROP))
  {
    DBusMessage *msg = dbus_message_new_method_return(message);
    int len = 0;
    int i;
    const char *_states[3];
    const char **states = _states;

    if (!msg)
      return DBUS_HANDLER_RESULT_NEED_MEMORY;

    for (i = 0; i < PB_STATE_LAST; i++)
    {
      if (pb->allowed_state[i] == TRUE)
      {
        _states[len] = pb_state_to_string(i);
        len ++;
      }
    }

    dbus_message_append_args(msg,
                             DBUS_TYPE_ARRAY,
                             DBUS_TYPE_STRING, &states, len,
                             DBUS_TYPE_INVALID);
    dbus_connection_send(pb->connection, msg, 0);
    dbus_message_unref(msg);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
_playback_set(pb_playback_t *pb, DBusMessage *message)
{
  DBusError error;
  DBusMessageIter iter;
  const char *iface;
  const char *prop;

  dbus_error_init(&error);
  dbus_message_iter_init(message, &iter);

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
  {
    _dbus_error_reply(
          pb->connection, message, DBUS_MAEMO_ERROR_INVALID_ARGS, "");
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  dbus_message_iter_get_basic(&iter, &iface);
  dbus_message_iter_next(&iter);

  if (strcmp(iface, DBUS_PLAYBACK_INTERFACE))
  {
    return _dbus_error_reply(pb->connection, message,
                             DBUS_MAEMO_ERROR_INVALID_IFACE, "");
  }

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
  {
    _dbus_error_reply(
          pb->connection, message, DBUS_MAEMO_ERROR_INVALID_ARGS, "");
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  dbus_message_iter_get_basic(&iter, &prop);
  dbus_message_iter_next(&iter);

  if (!strcmp(prop, DBUS_PLAYBACK_STATE_PROP))
  {
    DBusMessage *msg;
    enum pb_state_e state;
    const char *s;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
    {
      _dbus_error_reply(pb->connection, message,
                        DBUS_MAEMO_ERROR_INVALID_ARGS, "");
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    dbus_message_iter_get_basic(&iter, &s);
    dbus_message_iter_next(&iter);
    state = pb_string_to_state(s);

    if (state == PB_STATE_NONE)
    {
      return _dbus_error_reply(pb->connection, message,
                               DBUS_MAEMO_ERROR_INVALID_ARGS,
                               "Bad state value");
    }

    if (state != pb->pb_state)
    {
      pb_req_t *req = _pb_request_new(pb);

      if (!req)
      {
        return _dbus_error_reply(pb->connection, message,
                                 DBUS_MAEMO_ERROR_INTERNAL_ERR, "");
      }

      req->message = message;
      req->pb_state = state;
      req->finished = TRUE;
      pb->state_req_handler(pb, state, req, pb->state_req_handler_data);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    msg = dbus_message_new_method_return(message);

    if (msg)
    {
      dbus_connection_send(pb->connection, msg, 0);
      dbus_message_unref(msg);
    }

    _playback_signal_state(pb);

    return DBUS_HANDLER_RESULT_HANDLED;

  }
  else if (!strcmp(prop, DBUS_PLAYBACK_ALLOWED_STATE_PROP))
  {
    DBusMessage *msg;
    const char *iface;
    char **states;
    int len;

    dbus_message_get_args(message, &error,
                          DBUS_TYPE_STRING, &iface,
                          DBUS_TYPE_STRING, &prop,
                          DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &states, &len,
                          DBUS_TYPE_INVALID);

    if (dbus_error_is_set(&error))
    {
      return _dbus_error_reply(pb->connection, message,
                               DBUS_MAEMO_ERROR_INVALID_ARGS, error.message);
    }

    _update_allowed_states(pb, states, len);
    dbus_free_string_array(states);
    msg = dbus_message_new_method_return(message);

    if (!msg)
      return DBUS_HANDLER_RESULT_NEED_MEMORY;

    dbus_connection_send(pb->connection, msg, 0);
    dbus_message_unref(msg);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
_playback_get_all(pb_playback_t *pb, DBusMessage *message)
{
  DBusError error;
  DBusMessageIter iter;
  DBusMessageIter prop_it;
  DBusMessageIter array_it;
  DBusMessageIter val_it;
  DBusMessageIter states_it;
  const char *iface;
  const char *stream = "";
  const char *prop = DBUS_PLAYBACK_ALLOWED_STATE_PROP;
  DBusMessage *msg;
  char buf[64];

  dbus_error_init(&error);
  dbus_message_get_args(message, &error,
                        DBUS_TYPE_STRING, &iface,
                        DBUS_TYPE_INVALID);

  if (dbus_error_is_set(&error))
  {
    _dbus_error_reply(pb->connection, message, DBUS_MAEMO_ERROR_INVALID_ARGS,
                      error.message);
    dbus_error_free(&error);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (strcmp(iface, DBUS_PLAYBACK_INTERFACE))
  {
    return _dbus_error_reply(pb->connection, message,
                             DBUS_MAEMO_ERROR_INVALID_IFACE, "");
  }

  msg = dbus_message_new_method_return(message);

  if (!msg)
    goto err;

  dbus_message_iter_init_append(msg, &iter);

  if (!!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}",
                                         &prop_it) ||
      !_add_property(&prop_it, DBUS_PLAYBACK_STATE_PROP,
                     pb_state_to_string(pb->pb_state)) ||
      !_add_property(&prop_it, DBUS_PLAYBACK_CLASS_PROP,
                     pb_class_to_string(pb->pb_class)))
  {
    goto err;
  }

  snprintf(buf, sizeof(buf), "%ld", (long)pb->pid);

  if (!_add_property(&prop_it, DBUS_PLAYBACK_PID_PROP, buf))
    goto err;

  snprintf(buf, sizeof(buf), "%u", pb->flags);

  if (!_add_property(&prop_it, DBUS_PLAYBACK_FLAGS_PROP, buf))
    goto err;

  if (pb->stream)
    stream = pb->stream;

  if (!_add_property(&prop_it, DBUS_PLAYBACK_STREAM_PROP, stream) ||
      !dbus_message_iter_open_container(&prop_it, DBUS_TYPE_DICT_ENTRY,
                                        NULL, &val_it))
  {
    goto err;
  }

  if (dbus_message_iter_append_basic(&val_it, DBUS_TYPE_STRING, &prop) &&
      dbus_message_iter_open_container(&val_it, DBUS_TYPE_VARIANT,
                                       "as", &array_it) &&
      dbus_message_iter_open_container(&array_it, DBUS_TYPE_ARRAY,
                                       DBUS_TYPE_STRING_AS_STRING, &states_it))
  {
    int i;

    for (i = 0; i < PB_STATE_LAST; i++)
    {
      if (pb->allowed_state[i] == TRUE)
      {
        const char *s = pb_state_to_string(i);

        if (!dbus_message_iter_append_basic(&states_it, DBUS_TYPE_STRING, &s))
          goto err;
      }
    }

    dbus_message_iter_close_container(&array_it, &states_it);
    dbus_message_iter_close_container(&val_it, &array_it);
    dbus_message_iter_close_container(&prop_it, &val_it);
    dbus_message_iter_close_container(&iter, &prop_it);
    dbus_connection_send(pb->connection, msg, 0);
    dbus_message_unref(msg);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

err:

  if (msg)
    dbus_message_unref(msg);

  return _dbus_error_reply(pb->connection, message,
                           DBUS_MAEMO_ERROR_INTERNAL_ERR, "");
}

static const char *introspect =
DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
"<node>\n"
 "<interface name=\"org.maemo.Playback\">\n"
  " <property name=\"State\" type=\"s\" access=\"readwrite\"/>\n"
  " <property name=\"AllowedState\" type=\"as\" access=\"readwrite\"/>\n"
  " <property name=\"Class\" type=\"s\" access=\"read\"/>\n"
  " <property name=\"Flags\" type=\"s\" access=\"read\"/>\n"
  " <property name=\"Pid\" type=\"s\" access=\"read\"/>\n"
  " <property name=\"Stream\" type=\"s\" access=\"read\"/>\n"
  " <signal name=\"Hello\"/>\n"
  " <signal name=\"Notify\"/>\n"
 "</interface>\n"
"</node>";

static DBusHandlerResult
_dbus_playback_message(DBusConnection *connection,
                       DBusMessage *message,
                       void *user_data)
{
  pb_playback_t *pb;
  DBusError error;
  const char *iface;
  const char *member;

  pb = (pb_playback_t *)user_data;
  dbus_error_init(&error);

  if (dbus_set_error_from_message(&error, message))
  {
    dbus_error_free(&error);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  iface = dbus_message_get_interface(message);
  member = dbus_message_get_member(message);

  if (!strcmp(DBUS_INTERFACE_INTROSPECTABLE, iface) &&
      !strcmp("Introspect", member))
  {
    DBusMessage *msg;

    if (!dbus_message_has_signature(message, DBUS_TYPE_INVALID_AS_STRING))
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    msg = dbus_message_new_method_return(message);

    if (!msg)
      return DBUS_HANDLER_RESULT_NEED_MEMORY;

    dbus_message_append_args(msg,
                             DBUS_TYPE_STRING, &introspect,
                             DBUS_TYPE_INVALID);
    dbus_connection_send(pb->connection, msg, 0);
    dbus_message_unref(msg);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (strcmp(iface, DBUS_INTERFACE_PROPERTIES))
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (!strcmp(member, "Get"))
    return _playback_get(pb, message);

  if (!strcmp(member, "Set"))
    return _playback_set(pb, message);

  if (!strcmp(member, "GetAll"))
    return _playback_get_all(pb, message);

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

