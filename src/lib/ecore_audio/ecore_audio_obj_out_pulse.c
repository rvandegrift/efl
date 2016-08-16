#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libgen.h>

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif

#include <Eo.h>
#include "ecore_audio_private.h"
#include <pulse/pulseaudio.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

extern pa_mainloop_api functable;

#define MY_CLASS ECORE_AUDIO_OUT_PULSE_CLASS
#define MY_CLASS_NAME "Ecore_Audio_Out_Pulse"

struct _Ecore_Audio_Pulse_Class {
  pa_mainloop_api *api;
  pa_context *context;
  pa_context_state_t state;
  Ecore_Job *state_job;
  Eina_List *outputs;
};

static struct _Ecore_Audio_Pulse_Class class_vars = {
    .api = &functable,
};

struct _Ecore_Audio_Out_Pulse_Data
{
  char *foo;
};

typedef struct _Ecore_Audio_Out_Pulse_Data Ecore_Audio_Out_Pulse_Data;

EOLIAN static void
_ecore_audio_out_pulse_ecore_audio_volume_set(Eo *eo_obj, Ecore_Audio_Out_Pulse_Data *_pd EINA_UNUSED, double volume)
{
  Eo *in;
  pa_stream *stream = NULL;
  Eina_List *input;
  uint32_t idx;
  pa_cvolume pa_volume;
  Ecore_Audio_Output *out_obj = eo_data_scope_get(eo_obj, ECORE_AUDIO_OUT_CLASS);

  if (!EPA_LOAD()) return;
  if (volume < 0)
    volume = 0;

  EPA_CALL(pa_cvolume_set)(&pa_volume, 2, volume * PA_VOLUME_NORM);

  ecore_audio_obj_volume_set(eo_super(eo_obj, MY_CLASS), volume);

  EINA_LIST_FOREACH(out_obj->inputs, input, in) {
      stream = eo_key_data_get(in, "pulse_data");
      idx = EPA_CALL(pa_stream_get_index)(stream);
      EPA_CALL(pa_operation_unref)(EPA_CALL(pa_context_set_sink_input_volume)(class_vars.context, idx, &pa_volume, NULL, NULL));
  }
}

static void _write_cb(pa_stream *stream, size_t len, void *data)
{
  Eo *in = data;

  void *buf;
  ssize_t bread = 0;
  size_t wlen = len;

  if (!EPA_LOAD()) return;
  EPA_CALL(pa_stream_begin_write)(stream, &buf, &wlen);

  bread = ecore_audio_obj_in_read(in, buf, wlen);

  EPA_CALL(pa_stream_write)(stream, buf, bread, NULL, 0, PA_SEEK_RELATIVE);
  if (bread < (int)len)
    {
      EPA_CALL(pa_operation_unref)(EPA_CALL(pa_stream_trigger)(stream, NULL, NULL));
    }
}

static void _update_samplerate_cb(void *data EINA_UNUSED, const Eo_Event *event)
{
  pa_stream *stream = NULL;
  int samplerate = 0;
  double speed = 0;

  samplerate = ecore_audio_obj_in_samplerate_get(event->object);
  speed = ecore_audio_obj_in_speed_get(event->object);

  stream = eo_key_data_get(event->object, "pulse_data");

  EPA_CALL(pa_operation_unref)(EPA_CALL(pa_stream_update_sample_rate)(stream, samplerate * speed, NULL, NULL));
}

static Eina_Bool _input_attach_internal(Eo *eo_obj, Eo *in)
{
  const char *name = NULL;
  pa_sample_spec ss;
  double speed = 0;
  pa_stream *stream;
  Eina_Bool ret = EINA_FALSE;
  Ecore_Audio_Object *ea_obj = eo_data_scope_get(eo_obj, ECORE_AUDIO_CLASS);

  if (!EPA_LOAD()) return EINA_FALSE;
  ret = ecore_audio_obj_out_input_attach(eo_super(eo_obj, MY_CLASS), in);
  if (!ret)
    return EINA_FALSE;

  ss.format = PA_SAMPLE_FLOAT32LE;
  ss.rate = ecore_audio_obj_in_samplerate_get(in);
  speed = ecore_audio_obj_in_speed_get(in);
  ss.channels = ecore_audio_obj_in_channels_get(in);
  name = ecore_audio_obj_name_get(in);

  ss.rate = ss.rate * speed;

  stream = EPA_CALL(pa_stream_new)(class_vars.context, name, &ss, NULL);
  if (!stream) {
      ERR("Could not create stream");
      ecore_audio_obj_out_input_detach(eo_super(eo_obj, MY_CLASS), in);
      return EINA_FALSE;
  }

  eo_event_callback_add(in, ECORE_AUDIO_IN_EVENT_IN_SAMPLERATE_CHANGED, _update_samplerate_cb, eo_obj);

  eo_key_data_set(in, "pulse_data", stream);


  EPA_CALL(pa_stream_set_write_callback)(stream, _write_cb, in);
  EPA_CALL(pa_stream_connect_playback)(stream, NULL, NULL, PA_STREAM_VARIABLE_RATE, NULL, NULL);

  if (ea_obj->paused)
    EPA_CALL(pa_operation_unref)(EPA_CALL(pa_stream_cork)(stream, 1, NULL, NULL));

  return ret;
}

static void _delayed_attach_cb(void *data, const Eo_Event *event)
{
  Eo *in = data;
  eo_event_callback_del(event->object, ECORE_AUDIO_OUT_PULSE_EVENT_CONTEXT_READY, _delayed_attach_cb, in);

  _input_attach_internal(event->object, in);
}

EOLIAN static Eina_Bool
_ecore_audio_out_pulse_ecore_audio_out_input_attach(Eo *eo_obj, Ecore_Audio_Out_Pulse_Data *_pd EINA_UNUSED, Eo *in)
{
  Eina_Bool retval = EINA_TRUE;

  if (class_vars.state != PA_CONTEXT_READY) {
    DBG("Delaying input_attach because PA context is not ready.");
    eo_event_callback_add(eo_obj, ECORE_AUDIO_OUT_PULSE_EVENT_CONTEXT_READY, _delayed_attach_cb, in);
  } else {
    retval = _input_attach_internal(eo_obj, in);
  }

  return retval;
}

static void _drain_cb(pa_stream *stream, int success EINA_UNUSED, void *data EINA_UNUSED)
{
  if (!EPA_LOAD()) return;
  EPA_CALL(pa_stream_disconnect)(stream);
  EPA_CALL(pa_stream_unref)(stream);
}

EOLIAN static Eina_Bool
_ecore_audio_out_pulse_ecore_audio_out_input_detach(Eo *eo_obj, Ecore_Audio_Out_Pulse_Data *_pd EINA_UNUSED, Eo *in)
{
  pa_stream *stream = NULL;
  Eina_Bool ret2 = EINA_FALSE;
  pa_operation *op;

  if (!EPA_LOAD()) return EINA_FALSE;
  ret2 = ecore_audio_obj_out_input_detach(eo_super(eo_obj, MY_CLASS), in);
  if (!ret2)
    return EINA_FALSE;

  stream = eo_key_data_get(in, "pulse_data");

  EPA_CALL(pa_stream_set_write_callback)(stream, NULL, NULL);
  op = EPA_CALL(pa_stream_drain) (stream, _drain_cb, NULL);
  if (!op)
    {
       ERR("Failed to drain PulseAudio stream.");
       return EINA_FALSE;
    }

  EPA_CALL(pa_operation_unref)(op);
  return EINA_TRUE;
}

static void _state_cb(pa_context *context, void *data EINA_UNUSED)
{
   Eina_List *out, *tmp;
   Eo *eo_obj;
   pa_context_state_t state;
   
   if (!EPA_LOAD()) return;
   state = EPA_CALL(pa_context_get_state)(context);
   class_vars.state = state;
   
   //ref everything in the list to be sure...
   EINA_LIST_FOREACH(class_vars.outputs, out, eo_obj) {
      eo_ref(eo_obj);
   }
   // the callback here can delete things in the list..
   if (state == PA_CONTEXT_READY) {
      DBG("PA context ready.");
      EINA_LIST_FOREACH(class_vars.outputs, out, eo_obj) {
         eo_event_callback_call(eo_obj, ECORE_AUDIO_OUT_PULSE_EVENT_CONTEXT_READY, NULL);
      }
   } else if ((state == PA_CONTEXT_FAILED) || (state == PA_CONTEXT_TERMINATED)) {
      DBG("PA context fail.");
      EINA_LIST_FOREACH(class_vars.outputs, out, eo_obj) {
         eo_event_callback_call(eo_obj, ECORE_AUDIO_OUT_PULSE_EVENT_CONTEXT_FAIL, NULL);
      }
   } else {
      DBG("Connection state %i", state);
   }
   // now unref everything safely
   EINA_LIST_FOREACH_SAFE(class_vars.outputs, out, tmp, eo_obj) {
      eo_unref(eo_obj);
   }
}

static void _state_job(void *data EINA_UNUSED)
{
   if ((class_vars.state == PA_CONTEXT_FAILED) ||
       (class_vars.state == PA_CONTEXT_TERMINATED))
     {
        Eo *eo_obj;
        Eina_List *out, *tmp;
        
        DBG("PA context fail.");
        //ref everything in the list to be sure...
        EINA_LIST_FOREACH(class_vars.outputs, out, eo_obj) {
           eo_ref(eo_obj);
        }
        // the callback here can delete things in the list..
        EINA_LIST_FOREACH(class_vars.outputs, out, eo_obj) {
           eo_event_callback_call(eo_obj, ECORE_AUDIO_OUT_PULSE_EVENT_CONTEXT_FAIL, NULL);
        }
        // now unref everything safely
        EINA_LIST_FOREACH_SAFE(class_vars.outputs, out, tmp, eo_obj) {
           eo_unref(eo_obj);
        }
     }
   class_vars.state_job = NULL;
}

EOLIAN static Eo *
_ecore_audio_out_pulse_eo_base_constructor(Eo *eo_obj, Ecore_Audio_Out_Pulse_Data *_pd EINA_UNUSED)
{
  int argc;
  char **argv;
  Ecore_Audio_Output *out_obj = eo_data_scope_get(eo_obj, ECORE_AUDIO_OUT_CLASS);

  if (!EPA_LOAD()) return NULL;
  eo_obj = eo_constructor(eo_super(eo_obj, MY_CLASS));

  out_obj->need_writer = EINA_FALSE;

  if (!class_vars.context) {
    ecore_app_args_get(&argc, &argv);
    if (!argc) {
        DBG("Could not get program name, pulse outputs will be named ecore_audio");
       class_vars.context = EPA_CALL(pa_context_new)(class_vars.api, "ecore_audio");
    } else {
       class_vars.context = EPA_CALL(pa_context_new)(class_vars.api, basename(argv[0]));
    }
    EPA_CALL(pa_context_set_state_callback)(class_vars.context, _state_cb, NULL);
    EPA_CALL(pa_context_connect)(class_vars.context, NULL, PA_CONTEXT_NOFLAGS, NULL);
  }

  class_vars.outputs = eina_list_append(class_vars.outputs, eo_obj);
  if (class_vars.state_job) ecore_job_del(class_vars.state_job);
  class_vars.state_job = ecore_job_add(_state_job, NULL);

  return eo_obj;
}

EOLIAN static void
_ecore_audio_out_pulse_eo_base_destructor(Eo *eo_obj, Ecore_Audio_Out_Pulse_Data *_pd EINA_UNUSED)
{
  class_vars.outputs = eina_list_remove(class_vars.outputs, eo_obj);
  eo_destructor(eo_super(eo_obj, MY_CLASS));
}

#include "ecore_audio_out_pulse.eo.c"
