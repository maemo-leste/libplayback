#include <string.h>
#include <assert.h>

#include "libplayback/playback-types.h"

static const struct
{
  const char *pb_class_name;
  enum pb_class_e pb_class;
}
class_table[] =
{
  {"None", PB_CLASS_NONE},
  {"Test", PB_CLASS_TEST},
  {"Event", PB_CLASS_EVENT},
  {"VoIP", PB_CLASS_VOIP},
  {"Call", PB_CLASS_CALL},
  {"Ringtone", PB_CLASS_RINGTONE},
  {"Media", PB_CLASS_MEDIA},
  {"VoiceUI", PB_CLASS_VOICEUI},
  {"Camera", PB_CLASS_CAMERA},
  {"Game", PB_CLASS_GAME},
  {"Background", PB_CLASS_BACKGROUND},
  {"Alarm", PB_CLASS_ALARM},
  {"Flash", PB_CLASS_FLASH},
  {"System", PB_CLASS_SYSTEM},
  {"Input", PB_CLASS_INPUT}
};

static const struct
{
  const char *pb_state_name;
  enum pb_state_e pb_state;
}
state_table[] =
{
  {"None", PB_STATE_NONE},
  {"Stop", PB_STATE_STOP},
  {"Play", PB_STATE_PLAY}
};

const char *
pb_class_to_string(enum pb_class_e pb_class)
{
  int i;

  for (i = 0; i < pb_n_elements(class_table); i++)
  {
    if (class_table[i].pb_class == pb_class)
      return class_table[i].pb_class_name;
  }

  return "";
}

enum pb_class_e
pb_string_to_class(const char *aclass)
{
  int i;

  assert(aclass != ((void *)0));

  for (i = 0; i < pb_n_elements(class_table); i++)
  {
    if (!strcmp(class_table[i].pb_class_name, aclass))
      return class_table[i].pb_class;
  }

  return PB_CLASS_NONE;
}

const char *
pb_state_to_string(enum pb_state_e pb_state)
{
  int i;

  for (i = 0; i < pb_n_elements(state_table); i++)
  {
    if (state_table[i].pb_state == pb_state)
      return state_table[i].pb_state_name;
  }

  return "";
}

enum pb_state_e
pb_string_to_state(const char *state)
{
  int i;

  assert(state != ((void *)0));

  for (i = 0; i < pb_n_elements(state_table); i++)
  {
    if (!strcmp(state_table[i].pb_state_name, state))
      return state_table[i].pb_state;
  }

  return PB_STATE_NONE;
}
