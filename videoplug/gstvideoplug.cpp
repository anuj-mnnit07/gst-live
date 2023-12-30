/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2023  <<user@hostname.org>>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-videoplug
 *
 * FIXME:Describe videoplug here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! videoplug ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#include "gstvideoplug.h"

#include "custom.h"

GST_DEBUG_CATEGORY_STATIC(gst_videoplug_debug);
#define GST_CAT_DEFAULT gst_videoplug_debug
static CustomVideo *custom_video_obj = nullptr;
/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_CUSTOM,
};

#define GST_VIDEO_FORMATS GST_VIDEO_FORMATS_ALL

static GstStaticCaps gst_videoplug_format_caps = GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE("{ ARGB, BGRA, ABGR, RGBA, xRGB, BGRx, xBGR, RGBx}"));

static void gst_videoplug_finalize(Gstvideoplug *videoplug);
static GstFlowReturn gst_videoplug_transform_frame(GstVideoFilter *filter,
                                                   GstVideoFrame *frame);

static GstCaps *gst_videoplug_get_capslist(void)
{
  static GstCaps *caps = NULL;
  static gsize inited = 0;

  if (g_once_init_enter(&inited))
  {
    caps = gst_static_caps_get(&gst_videoplug_format_caps);
    g_once_init_leave(&inited, 1);
  }
  return caps;
}

static GstPadTemplate *gst_videoplug_src_template_factory(void)
{
  return gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                              gst_videoplug_get_capslist());
}

static GstPadTemplate *gst_videoplug_sink_template_factory(void)
{
  return gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                              gst_videoplug_get_capslist());
}

#define gst_videoplug_parent_class parent_class
G_DEFINE_TYPE(Gstvideoplug, gst_videoplug, GST_TYPE_ELEMENT);

GST_ELEMENT_REGISTER_DEFINE(videoplug, "videoplug", GST_RANK_NONE,
                            GST_TYPE_VIDEOPLUG);

static void gst_videoplug_set_property(GObject *object, guint prop_id,
                                       const GValue *value, GParamSpec *pspec);
static void gst_videoplug_get_property(GObject *object, guint prop_id,
                                       GValue *value, GParamSpec *pspec);

static gboolean gst_videoplug_sink_event(GstPad *pad, GstObject *parent,
                                         GstEvent *event);
static GstFlowReturn gst_videoplug_chain(GstPad *pad, GstObject *parent,
                                         GstBuffer *buf);

/* GObject vmethod implementations */

/* initialize the videoplug's class */
static void gst_videoplug_class_init(GstvideoplugClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *)klass;
  GstElementClass *gstelement_class = (GstElementClass *)klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *)klass;
  GstVideoFilterClass *video_filter_class = (GstVideoFilterClass *)klass;

  GST_DEBUG_CATEGORY_INIT(gst_videoplug_debug, "videoplug", 0,
                          "videoplug");

  gobject_class->finalize = (GObjectFinalizeFunc)gst_videoplug_finalize;
  custom_video_obj = new CustomVideo();

  gobject_class->set_property = gst_videoplug_set_property;
  gobject_class->get_property = gst_videoplug_get_property;
  g_object_class_install_property(
      gobject_class, PROP_SILENT,
      g_param_spec_boolean("silent", "Silent", "Produce verbose output ?",
                           FALSE, G_PARAM_READWRITE));

  g_object_class_install_property(
      gobject_class, PROP_CUSTOM,
      g_param_spec_int("custom", "CustomVideo", "Produce Custom Video Plugin",
                       0, 255, 0, G_PARAM_READWRITE));

  gst_element_class_set_static_metadata(gstelement_class, "Chroma hold filter",
                                        "VideoPlug",
                                        "Generic Video plugin",
                                        "Anuj Kumar");

  // gst_element_class_set_details_simple(
  //     gstelement_class, "videoplug", "Generic Video Plugin",
  //     "Generic Video Plugin Element", " Anuj Kumar");

  gst_element_class_add_pad_template(gstelement_class,
                                     gst_videoplug_src_template_factory());
  gst_element_class_add_pad_template(gstelement_class,
                                     gst_videoplug_sink_template_factory());

  video_filter_class->transform_frame_ip =
      GST_DEBUG_FUNCPTR(gst_videoplug_transform_frame);
  // video_filter_class->set_info = GST_DEBUG_FUNCPTR
  // (gst_chroma_hold_set_info);
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad callback functions
 * initialize instance structure
 */
static void gst_videoplug_init(Gstvideoplug *filter)
{
  filter->sinkpad = gst_pad_new_from_template(gst_videoplug_sink_template_factory(), "sink");
  gst_pad_set_event_function(filter->sinkpad,
                             GST_DEBUG_FUNCPTR(gst_videoplug_sink_event));
  gst_pad_set_chain_function(filter->sinkpad,
                             GST_DEBUG_FUNCPTR(gst_videoplug_chain));
  GST_PAD_SET_PROXY_CAPS(filter->sinkpad);
  gst_element_add_pad(GST_ELEMENT(filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_template(gst_videoplug_src_template_factory(), "src");
  GST_PAD_SET_PROXY_CAPS(filter->srcpad);
  gst_element_add_pad(GST_ELEMENT(filter),
                      filter->srcpad);

  filter->silent = FALSE;
  filter->custom = 0;
}

static void gst_videoplug_set_property(GObject *object, guint prop_id,
                                       const GValue *value, GParamSpec *pspec)
{
  Gstvideoplug *filter = GST_VIDEOPLUG(object);

  switch (prop_id)
  {
  case PROP_SILENT:
    filter->silent = g_value_get_boolean(value);
    break;
  case PROP_CUSTOM:
    filter->custom = g_value_get_int(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void gst_videoplug_get_property(GObject *object, guint prop_id,
                                       GValue *value, GParamSpec *pspec)
{
  Gstvideoplug *filter = GST_VIDEOPLUG(object);

  switch (prop_id)
  {
  case PROP_SILENT:
    g_value_set_boolean(value, filter->silent);
    break;
  case PROP_CUSTOM:
    g_value_set_int(value, filter->custom);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean gst_videoplug_sink_event(GstPad *pad, GstObject *parent,
                                         GstEvent *event)
{
  Gstvideoplug *filter;

  filter = GST_VIDEOPLUG(parent);

  GST_LOG_OBJECT(filter, "Received %s event: %" GST_PTR_FORMAT,
                 GST_EVENT_TYPE_NAME(event), event);

  switch (GST_EVENT_TYPE(event))
  {
  case GST_EVENT_CAPS:
  {
    GstCaps *caps;

    gst_event_parse_caps(event, &caps);
    /* do something with the caps */
    break;
  }
  case GST_EVENT_EOS:
  {
    // gst_videoplug_stop_processing (filter);
    break;
  }
  default:
    break;
  }
  return gst_pad_event_default(pad, parent, event);
  ;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn gst_videoplug_chain(GstPad *pad, GstObject *parent,
                                         GstBuffer *buf)
{
  Gstvideoplug *filter = GST_VIDEOPLUG(parent);
  // GstBuffer *outbuf = (GstBuffer*)malloc(sizeof (GstBuffer));
  // gst_buffer_copy_into(outbuf, buf, GST_BUFFER_COPY_ALL, 0,
  // gst_buffer_get_size(buf));
  if (filter->silent == FALSE)
  {
    // g_print ("Silent Video\n");
    g_print("Have data of size %" G_GSIZE_FORMAT " bytes!\n",
            gst_buffer_get_size(buf));
  }
  if (filter->custom == 0)
  {
    g_print("Custom Plugin Video\n");
  }

  /* just push out the incoming buffer without touching it */
  return gst_pad_push(filter->srcpad, buf);
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean videoplug_init(GstPlugin *plugin)
{
  /* debug category for filtering log messages
   *
   * exchange the string 'Template videoplug' with your description
   */
  GST_DEBUG_CATEGORY_INIT(gst_videoplug_debug, "videoplug", 0,
                          "Template videoplug");

  return GST_ELEMENT_REGISTER(videoplug, plugin);
}

GstFlowReturn gst_videoplug_transform_frame(GstVideoFilter *filter,
                                            GstVideoFrame *frame)
{
  return GST_FLOW_OK;
}

void gst_videoplug_finalize(Gstvideoplug *videoplug)
{
  g_print("finalzing video plug\n");
  G_OBJECT_CLASS(parent_class)->finalize(G_OBJECT(videoplug));
}

/* PACKAGE: this is usually set by meson depending on some _INIT macro
 * in meson.build and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use meson to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "videoplug"
#endif

/* gstreamer looks for this structure to register videoplugs
 *
 * exchange the string 'Template videoplug' with your videoplug description
 */
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, videoplug, "videoplug",
                  videoplug_init, PACKAGE_VERSION, GST_LICENSE,
                  GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
