export GST_PLUGIN_PATH=~/work/myproject/gst-live/build/videoplug
gst-inspect-1.0 videoplug
gst-launch-1.0 -m -v fakesrc ! videoplug ! fakesink silent=TRUE
gst-launch-1.0 -m -v videotestsrc ! videoplug ! videoconvert ! osxvideosink
