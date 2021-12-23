#/bin/bash
set -xT
rm *.jpg 2>/dev/null
if [ readGauge.cpp -nt readGauge ] ; then
  g++ readGauge.cpp -o readGauge -I/usr/include/opencv4/ -Wl,/usr/lib/libopencv_core.so,/usr/lib/libopencv_imgcodecs.so,/usr/lib/libopencv_highgui.so,/usr/lib/libopencv_imgproc.so
fi

if [ readGauge -nt ../readGauge ] ; then
  cp readGauge .. 
fi

