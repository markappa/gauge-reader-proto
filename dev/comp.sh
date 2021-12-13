rm *.jpg
g++ readGauge.cpp -o readGauge -I/usr/include/opencv4/ -Wl,/usr/lib/libopencv_core.so,/usr/lib/libopencv_imgcodecs.so,/usr/lib/libopencv_highgui.so,/usr/lib/libopencv_imgproc.so

