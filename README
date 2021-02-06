# gstblurhashsrc: BlurHash GStreamer decoder

GStreamer plugin allowing the decoding of BlurHash (https://blurha.sh/) encoded hash values
It decodes an hash value into a 32-bit RGBA raw image
Uses decoder code from: https://github.com/woltapp/blurhash/tree/master/C 

Parameters:
	1) hash: hash value to be decoded (default value is: "LaJHjmVu8_~po#smR+a~xaoLWCRj")

Default values:
	1) hash: "LaJHjmVu8_~po#smR+a~xaoLWCRj"
	2) width: 128
	3) height: 128

Sample usage pipelines:
$ gst-launch-1.0 blurhashsrc ! imagefreeze ! fakesink

$ gst-launch-1.0 blurhashsrc ! imagefreeze ! videoconvert ! ximagesink

TODO:
- Add support for 24-bit RGB format.