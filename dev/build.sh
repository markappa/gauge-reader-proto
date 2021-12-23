rm *.jpg 2>/dev/null
g++ readGauge.cpp -o readGauge -I/usr/include/opencv4/ -Wl,/usr/lib/libopencv_core.so,/usr/lib/libopencv_imgcodecs.so,/usr/lib/libopencv_highgui.so,/usr/lib/libopencv_imgproc.so

cp readGauge .. 2>/dev/null
