/* GStreamer
 * Copyright (C) 2021 Gilles Talis <gilles.talis@protonmail.com>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstblurhashsrc
 *
 * The blurhashsrc element decodes a BlurHash hash to a raw image
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v blurhashsrc ! imagefreeze ! fakesink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/video/video.h>
#include "gstblurhashsrc.h"
#include "decode.h"

GST_DEBUG_CATEGORY_STATIC (gst_blurhash_src_debug_category);
#define GST_CAT_DEFAULT gst_blurhash_src_debug_category

#define gst_blurhash_src_parent_class parent_class

#define DEFAULT_WIDTH_HEIGHT  128
#define DEFAULT_NUM_CHANNELS  4
#define DEFAULT_PUNCH         1

/* prototypes */


static void gst_blurhash_src_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_blurhash_src_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_blurhash_src_dispose (GObject * object);
static void gst_blurhash_src_finalize (GObject * object);

static GstCaps *gst_blurhash_src_get_caps (GstBaseSrc * src, GstCaps * filter);
static GstCaps *gst_blurhash_src_fixate (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_blurhash_src_set_caps (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_blurhash_src_decide_allocation (GstBaseSrc * src,
    GstQuery * query);
static gboolean gst_blurhash_src_start (GstBaseSrc * src);
static gboolean gst_blurhash_src_stop (GstBaseSrc * src);
static gboolean gst_blurhash_src_is_seekable (GstBaseSrc * src);
static gboolean gst_blurhash_src_query (GstBaseSrc * src, GstQuery * query);
static GstFlowReturn gst_blurhash_src_fill (GstPushSrc * src, GstBuffer * buf);

enum
{
  PROP_0,
  PROP_HASH
};

/* pad templates */

static GstStaticPadTemplate gst_blurhash_src_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
	GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE
        ("{ RGBA, RGB }"))
    );

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstBlurHashSrc, gst_blurhash_src, GST_TYPE_PUSH_SRC,
  GST_DEBUG_CATEGORY_INIT (gst_blurhash_src_debug_category, "blurhashsrc", 0,
  "debug category for blurhashsrc element"));

static void
gst_blurhash_src_class_init (GstBlurHashSrcClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS (klass);
  GstPushSrcClass *pushsrc_class = GST_PUSH_SRC_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &gst_blurhash_src_src_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "BlurHash hash decoder",
      "Generic",
      "Decode a BlurHash hash to a raw image",
      "Gilles Talis <gilles.talis@protonmail.com>");

  gobject_class->set_property = gst_blurhash_src_set_property;
  gobject_class->get_property = gst_blurhash_src_get_property;
  gobject_class->dispose = gst_blurhash_src_dispose;

  g_object_class_install_property (gobject_class, PROP_HASH,
      g_param_spec_string ("hash", "Hash",
          "Hash of raw image to be decoded", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gobject_class->finalize = gst_blurhash_src_finalize;
  base_src_class->fixate = GST_DEBUG_FUNCPTR (gst_blurhash_src_fixate);
  base_src_class->set_caps = GST_DEBUG_FUNCPTR (gst_blurhash_src_set_caps);
  base_src_class->is_seekable = GST_DEBUG_FUNCPTR (gst_blurhash_src_is_seekable);
  base_src_class->query = GST_DEBUG_FUNCPTR (gst_blurhash_src_query);
  base_src_class->start = GST_DEBUG_FUNCPTR (gst_blurhash_src_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR (gst_blurhash_src_stop);
  base_src_class->decide_allocation = GST_DEBUG_FUNCPTR (gst_blurhash_src_decide_allocation);
  pushsrc_class->fill = GST_DEBUG_FUNCPTR (gst_blurhash_src_fill);
}

static void
gst_blurhash_src_init (GstBlurHashSrc *blurhashsrc)
{
  blurhashsrc->hash         = "LaJHjmVu8_~po#smR+a~xaoLWCRj";
  blurhashsrc->num_channels = DEFAULT_NUM_CHANNELS;
}

void
gst_blurhash_src_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (object);

  GST_DEBUG_OBJECT (blurhashsrc, "set_property");

  switch (property_id) {
    case PROP_HASH:
      blurhashsrc->hash = g_strdup (g_value_get_string (value));
      // TODO: verify blurhash validity
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_blurhash_src_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (object);

  GST_DEBUG_OBJECT (blurhashsrc, "get_property");

  switch (property_id) {
    case PROP_HASH:
      g_value_set_string (value, blurhashsrc->hash);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_blurhash_src_dispose (GObject * object)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (object);

  GST_DEBUG_OBJECT (blurhashsrc, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_blurhash_src_parent_class)->dispose (object);
}

void
gst_blurhash_src_finalize (GObject * object)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (object);

  GST_DEBUG_OBJECT (blurhashsrc, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_blurhash_src_parent_class)->finalize (object);
}

/* get caps from subclass */
static GstCaps *
gst_blurhash_src_get_caps (GstBaseSrc * src, GstCaps * filter)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (src);

  GST_DEBUG_OBJECT (blurhashsrc, "get_caps");

  return NULL;
}

/* called if, in negotiation, caps need fixating */
static GstCaps *
gst_blurhash_src_fixate (GstBaseSrc * src, GstCaps * caps)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (src);
  GstStructure *structure;
  guint width;
  guint height;
  gboolean ret;

  GST_DEBUG_OBJECT (blurhashsrc, "fixate");

  caps = gst_caps_make_writable (caps);
  structure = gst_caps_get_structure (caps, 0);

  ret = gst_structure_get_int (structure, "width", &width);
  if (!ret) {
    width = DEFAULT_WIDTH_HEIGHT;
  }

  ret = gst_structure_get_int (structure, "height", &height);
  if (!ret) {
    height = DEFAULT_WIDTH_HEIGHT;
  }

  GST_DEBUG_OBJECT (blurhashsrc, "width=%d, height=%d", width, height);

  gst_structure_fixate_field_nearest_int (structure, "width", width);
  gst_structure_fixate_field_nearest_int (structure, "height", height);

  caps = GST_BASE_SRC_CLASS (parent_class)->fixate (src, caps);

  return caps;
}

/* notify the subclass of new caps */
static gboolean
gst_blurhash_src_set_caps (GstBaseSrc * src, GstCaps * caps)
{
  GstVideoInfo info;
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (src);

  GST_DEBUG_OBJECT (blurhashsrc, "set_caps");

  gst_video_info_init (&info);

  if (!gst_video_info_from_caps (&info, caps))
      return FALSE;

  blurhashsrc->info = info;
  GST_DEBUG_OBJECT (blurhashsrc, "size %dx%d, %d/%d fps",
      info.width, info.height, info.fps_n, info.fps_d);

  return TRUE;
}

/* setup allocation query */
static gboolean
gst_blurhash_src_decide_allocation (GstBaseSrc * src, GstQuery * query)
{
  GstBufferPool *pool;
  gboolean update;
  guint size, min, max;
  GstStructure *config;
  GstCaps *caps = NULL;
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (src);

  GST_DEBUG_OBJECT (blurhashsrc, "decide_allocation");

  if (gst_query_get_n_allocation_pools (query) > 0) {
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
    GST_DEBUG_OBJECT (blurhashsrc, "decide_allocation (pool=%p), size=%d, min=%d, max=%d", pool, size, min, max);
  } else {
    GST_DEBUG_OBJECT (blurhashsrc, "decide_allocation did not get allocation pools");
    pool = NULL;
    size = blurhashsrc->info.size;
    min = max = 0;
  }

  if (pool == NULL) {
    pool = gst_video_buffer_pool_new ();
  }

  config = gst_buffer_pool_get_config (pool);
  gst_query_parse_allocation (query, &caps, NULL);
  if (caps)
    gst_buffer_pool_config_set_params (config, caps, size, min, max);

  if (gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL)) {
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
  }
  gst_buffer_pool_set_config (pool, config);
  gst_query_add_allocation_pool (query, pool, size, min, max);

  if (pool)
    gst_object_unref (pool);

  return GST_BASE_SRC_CLASS (parent_class)->decide_allocation (src, query);
}

/* start and stop processing, ideal for opening/closing the resource */
static gboolean
gst_blurhash_src_start (GstBaseSrc * src)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (src);

  GST_DEBUG_OBJECT (blurhashsrc, "start");

  return TRUE;
}

static gboolean
gst_blurhash_src_stop (GstBaseSrc * src)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (src);

  GST_DEBUG_OBJECT (blurhashsrc, "stop");

  return TRUE;
}

/* check if the resource is seekable */
static gboolean
gst_blurhash_src_is_seekable (GstBaseSrc * src)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (src);

  GST_DEBUG_OBJECT (blurhashsrc, "is_seekable");

  return FALSE;
}

/* notify subclasses of a query */
static gboolean
gst_blurhash_src_query (GstBaseSrc * src, GstQuery * query)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (src);
  gboolean res;

  GST_DEBUG_OBJECT (blurhashsrc, "query");

  switch( GST_QUERY_TYPE(query)) {
    case GST_QUERY_POSITION:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_POSITION");
      break;
    case GST_QUERY_DURATION:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_DURATION");
      break;
    case GST_QUERY_LATENCY:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_LATENCY");
      break;
    case GST_QUERY_JITTER:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_JITTER");
      break;
    case GST_QUERY_RATE:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_RATE");
      break;
    case GST_QUERY_SEEKING:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_SEEKING");
      break;
    case GST_QUERY_SEGMENT:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_SEGMENT");
      break;
    case GST_QUERY_CONVERT:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_CONVERT");
      break;
    case GST_QUERY_FORMATS:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_FORMATS");
      break;
    case GST_QUERY_BUFFERING:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_BUFFERING");
      break;
    case GST_QUERY_CUSTOM:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_CUSTOM");
      break;
    case GST_QUERY_URI:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_URI");
      break;
    case GST_QUERY_ALLOCATION:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_ALLOCATION");
      break;
    case GST_QUERY_SCHEDULING:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_SCHEDULING");
      break;
    case GST_QUERY_ACCEPT_CAPS:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_ACCEPT_CAPS");
      break;
    case GST_QUERY_CAPS:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_CAPS");
      break;
    case GST_QUERY_DRAIN:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_DRAIN");
      break;
    case GST_QUERY_CONTEXT:
      GST_DEBUG_OBJECT (blurhashsrc, "GST_QUERY_CONTEXT");
      break;
  }

  res = GST_BASE_SRC_CLASS (parent_class)->query (src, query);

  return TRUE;
}

/* ask the subclass to fill the buffer with data from offset and size */
static GstFlowReturn
gst_blurhash_src_fill (GstPushSrc * src, GstBuffer * buf)
{
  GstBlurHashSrc *blurhashsrc = GST_BLURHASH_SRC (src);
  GstMapInfo info;
  gsize buf_size = gst_buffer_get_size (buf);

  GST_DEBUG_OBJECT (blurhashsrc, "fill (%lu)", buf_size);

  gst_buffer_map (buf, &info, GST_MAP_WRITE);
  decodeToArray(blurhashsrc->hash,
    blurhashsrc->info.width,
    blurhashsrc->info.height,
    DEFAULT_PUNCH,
    blurhashsrc->num_channels,
    info.data);
  gst_buffer_unmap (buf, &info);

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "blurhashsrc", GST_RANK_NONE,
      GST_TYPE_BLURHASH_SRC);
}

#ifndef VERSION
#define VERSION "0.1"
#endif
#ifndef PACKAGE
#define PACKAGE "gstblurhash"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "gstblurhash"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://github.com/gtalis/gstblurhashsrc"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    blurhashsrc,
    "BlurHash plugin library",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)