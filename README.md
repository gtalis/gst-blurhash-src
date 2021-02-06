# gstblurhashsrc: BlurHash GStreamer decoder

GStreamer plugin allowing the decoding of BlurHash (https://blurha.sh/) encoded hash values

It decodes an hash value into a 32-bit RGBA raw image

Uses decoder code from: https://github.com/woltapp/blurhash/tree/master/C 

## Build from source

### Dependencies
* gstreamer-1.0 (dev)
* gstreamer-base-1.0 (dev)
* gstreamer-video-1.0 (dev)

### Building
	mkdir build
	cd build
	cmake ..
	make

## Usage

### Plugin parameters:
* hash: hash value to be decoded (default value is: "LaJHjmVu8_~po#smR+a~xaoLWCRj")

### Plugin default values:
* hash: "LaJHjmVu8_~po#smR+a~xaoLWCRj"
* width: 128
* height: 128

### Sample usage pipelines:
$ gst-launch-1.0 blurhashsrc ! imagefreeze ! fakesink

$ gst-launch-1.0 blurhashsrc ! imagefreeze ! videoconvert ! ximagesink

$ gst-launch-1.0 blurhashsrc hash="L34x;VRlagRlRSowWDRl4aodowod" ! imagefreeze ! videoconvert ! ximagesink

$ gst-launch-1.0 blurhashsrc hash="L34x;VRlagRlRSowWDRl4aodowod" ! video/x-raw, width=256, height=256 ! imagefreeze ! videoconvert ! ximagesink

### TODO:
* Add support for 24-bit RGB format.
