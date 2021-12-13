#include <stdio.h>
#include <opencv2/opencv.hpp>

#undef DEBUG
#undef CALIBRATE
#undef FIND_THRESHOLD
#undef CANNY

#define PI 3.1415826535
#define max(a,b) ((a<b)?b:a)
#define dist_2_pts(x1,y1,x2,y2) (sqrt(pow((x2)-(x1),2)+pow((y2)-(y1),2)))
#define rad2deg(a) ((a)*180.0/PI)

using namespace cv;
int main(int argc, char** argv )
{
    if ( argc != 2 )
    {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }

    Mat img;
    img = imread( argv[1], 1 );
    if ( !img.data )
    {
        printf("No image data \n");
        return -1;
    }
#ifdef DEBUG
    printf("Loaded ok\n");
#endif

    int r = 83;
    Point2i c(410,240);
#ifdef CALIBRATE
    int height, width;
    height = img.rows;
    width = img.cols;
#ifdef DEBUG
    printf("cols %d, rows %d \n", width, height);
#endif

    Mat gray;
    cvtColor(img, gray, COLOR_BGR2GRAY);
    imwrite("gray.jpg", gray);

    std::vector<Vec3f> circles;
    HoughCircles(gray, circles, HOUGH_GRADIENT,
                 1, 20, 100, 50); //, 40, 80);
    //           1, 20, 100, 50, int(height*0.15), int(height*0.48));

    int n = int(circles.size());
    printf("Found %d circles.\n",n);

    if ( n == 0 )
       return 1;

    for( size_t i = 0; i < n; i++ )
    {
         Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
         int radius = cvRound(circles[i][2]);
         // printf("[%d %d] %d\n", cvRound(circles[i][0]), cvRound(circles[i][1]), cvRound(circles[i][2]));
         // printf("[%d %d] %d\n", center.x, center.y, radius);
         // Point2f a(0.3f, 0.f), b(0.f, 0.4f);
         // Point pt = (a + b)*10.f;
         c += center;
         r += radius;
#ifdef DEBUG
         std::cout << center << " " << radius << std::endl;
#endif
         // draw the circle center
         circle( img, center, 3, Scalar(0,255,0), -1, 8, 0 );
         // draw the circle outline
         circle( img, center, radius, Scalar(0,0,255), 3, 8, 0 );
    }
    c /= n;
    r = cvRound(r / n);
#ifdef DEBUG
    std::cout << "# " << c << " " << r << std::endl;
#endif
    // draw the circle center
    circle( img, c, 3, Scalar(0,255,250), -1, 8, 0 );
    // draw the circle outline
    circle( img, c, r, Scalar(0,250,255), 3, 8, 0 );

    /**
    goes through the motion of a circle and sets x and y values based on the set separation spacing.  Also adds text to each
    line.  These lines and text labels serve as the reference point for the user to enter
    NOTE: by default this approach sets 0/360 to be the +x axis (if the image has a cartesian grid in the middle), the addition
    (i+9) in the text offset rotates the labels by 90 degrees so 0/360 is at the bottom (-y in cartesian).  So this assumes the
    gauge is aligned in the image, but it can be adjusted by changing the value of 9 to something else.
    **/
    float separation = 10.0f; //in degrees
    int interval = int(360 / separation);
    Point2i p1[interval]; // set empty arrays
    Point2i p2[interval];
    Point2i p_text[interval];
    for ( int i=0; i<interval; i++) {
        p1[i].x = c.x + 0.9 * r * cos(separation * i * 3.14 / 180); // #point for lines
        p1[i].y = c.y + 0.9 * r * sin(separation * i * 3.14 / 180);
    }

    int text_offset_x = 10;
    int text_offset_y = 5;
    for ( int i=0; i<interval; i++) {
        p2[i].x = c.x + r * cos(separation * i * 3.14 / 180);
        p2[i].y = c.y + r * sin(separation * i * 3.14 / 180);
        p_text[i].x = c.x - text_offset_x + 1.2 * r * cos((separation) * (i+9) * 3.14 / 180); // point for text labels, i+9 rotates the labels by 90 degrees
        p_text[i].y = c.y + text_offset_y + 1.2* r * sin((separation) * (i+9) * 3.14 / 180); //  point for text labels, i+9 rotates the labels by 90 degrees
    }

    // add the lines and labels to the image
    for ( int i=0; i<interval; i++) {
        line(img, p1[i], p2[i], Scalar(0, 255, 0), 2);
        putText(img, std::to_string(int(i*separation)), p_text[i], FONT_HERSHEY_SIMPLEX, 0.3f, Scalar(0,0,0),1,LINE_AA);
#ifdef DEBUG
        std::cout << ".. " << p1[i] << " " <<p2[i] << std::endl;
#endif
    }

    imwrite("cerchi.jpg", img);
#endif // CALIBRATE

    int min_angle = 60;
    int max_angle = 320;
    int min_value = 0;
    int max_value = 4;
    String units = "bar";

    Mat gray2;
    cvtColor(img, gray2, COLOR_BGR2GRAY);

    // Set threshold and maxValue
    int thresh = 95;
    int maxValue = 255;
    Mat dst2;
#ifdef FIND_THRESHOLD
  for(thresh= 80; thresh<120; thresh += 5) {
#endif
    // apply thresholding which helps for finding lines
    threshold(gray2, dst2, thresh, maxValue, THRESH_BINARY_INV);

    // found Hough Lines generally performs better without Canny / blurring, though there were a couple exceptions where it would only work with Canny / blurring
    //dst2 = cv2.medianBlur(dst2, 5)
#ifdef CANNY
    Canny(dst2, dst2, 50, 150, 3);
#endif
    //dst2 = cv2.GaussianBlur(dst2, (5, 5), 0)
    // for testing, show image after thresholding
#ifdef DEBUG
#ifdef FIND_THRESHOLD
    imwrite("soglia"+ std::to_string(thresh) +".jpg", dst2);
#else
    imwrite("soglia.jpg", dst2);
#endif //FIND_THRESHOLD
#endif //DEBUG

#ifdef FIND_THRESHOLD
  }
#endif

    // find lines
    int minLineLength = 10;
    int maxLineGap = 6;
    int threshold = 50;
    std::vector<Vec4i> lines;

    HoughLinesP(dst2, lines, 3, PI / 360, threshold, minLineLength, maxLineGap);
    // rho is set to 3 to detect more lines, easier to get more then filter them out later

    // for testing purposes, show all found lines
#ifdef DEBUG
    std::cout << "Found " << std::to_string(lines.size()) << " lines.\n";
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        line( img, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, LINE_AA);
    }
    imwrite("righe_unfiltered.jpg", img);
#endif

    // remove all lines outside a given radius
    std::vector<Vec4i> final_line_list;

#ifdef DEBUG
    std::cout << "radius: " << r << std::endl;
#endif

    float diff1LowerBound = 0.15; // diff1LowerBound and diff1UpperBound determine how close the line should be from the center
    float diff1UpperBound = 0.25;
    float diff2LowerBound = 0.5; // diff2LowerBound and diff2UpperBound determine how close the other point of the line should be to the outside of the gauge
    float diff2UpperBound = 1.0;
//int mini; float mindiff=9999999.0f;
    int maxi; float maxl=0.0f; // find the longest line from selectd
    for ( int i=0; i<lines.size(); i++)
    {
        int x1, y1, x2, y2;
        x1 = lines[i][0];
        y1 = lines[i][1];
        x2 = lines[i][2];
        y2 = lines[i][3];
        // c is center of circle
        //float diff1 = sqrt(pow(c.x - x1, 2) + pow(c.y - y1, 2));
        //float diff2 = sqrt(pow(c.x - x2, 2) + pow(c.y - y2, 2));
        float diff1 = dist_2_pts(c.x, c.y, x1, y1);
        float diff2 = dist_2_pts(c.x, c.y, x2, y2);
        // set diff1 to be the smaller (closest to the center) of the two), makes the math easier
        if (diff2 < diff1) {
           float t = diff2;
           diff2 = diff1;
           diff1 = t;
        }
//if(i==316)std::cout<<"c "<<c<<", line "<<lines[i]<<", diff1,2 "<<diff1<<","<<diff2<<std::endl;
//if(diff1 < mindiff){mindiff=diff1;mini=i;}
        // check if line is within an acceptable range
        if ( (diff1<diff1UpperBound*r) && (diff1>diff1LowerBound*r)
           && (diff2<diff2UpperBound*r) && (diff2>diff2LowerBound*r)
           )
        {
            //float line_length = sqrt(pow(x2-x1,2)+pow(y2-y1,2));
            float line_length = dist_2_pts(x1, y1, x2, y2);
            if(line_length>maxl){
               maxi = final_line_list.size();
               maxl = line_length;
            }
            // add to final list
            Vec4i l{x1, y1, x2, y2};
            final_line_list.push_back(l);
        }
    }
//std::cout<<"min d, i "<<mindiff<<", "<<mini<<std::endl;

    // testing only, show all lines after filtering
#ifdef DEBUG
    std::cout << "Remaining " << std::to_string(final_line_list.size()) << " lines.\n";
    for( size_t i = 0; i < final_line_list.size(); i++ )
    {
        Vec4i l = final_line_list[i];
        if(i==maxi) {
           line( img, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,255,0), 1, LINE_AA);
        } else {
           line( img, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255,0,0), 1, LINE_AA);
        }
    }
    imwrite("righe_filtered.jpg", img);
#endif

    // assumes the longest line is the best one
    int x1, y1, x2, y2;
    x1 = final_line_list[maxi][0];
    y1 = final_line_list[maxi][1];
    x2 = final_line_list[maxi][2];
    y2 = final_line_list[maxi][3];
    //cv2.line(img, (x1, y1), (x2, y2), (0, 255, 0), 2)

    //for testing purposes, show the line overlayed on the original image
    //cv2.imwrite('gauge-1-test.jpg', img)
    //cv2.imwrite('gauge-%s-lines-2.%s' % (gauge_number, file_type), img)

    // find the farthest point from the center to be what is used to determine the angle
    float dist_pt_0 = dist_2_pts(c.x, c.y, x1, y1);
    float dist_pt_1 = dist_2_pts(c.x, c.y, x2, y2);
    int x_angle, y_angle;
    if (dist_pt_0 > dist_pt_1) {
        x_angle = x1 - c.x;
        y_angle = c.y - y1;
    } else {
        x_angle = x2 - c.x;
        y_angle = c.y - y2;
    }
    // take the arc tan of y/x to find the angle
    float res = atan(float(y_angle)/ float(x_angle));
    //np.rad2deg(res) #coverts to degrees

    // print x_angle
    // print y_angle
    // print res
    // print np.rad2deg(res)

    //these were determined by trial and error
    res = rad2deg(res);
    float final_angle;
    if (x_angle > 0 and y_angle > 0) { //in quadrant I
        final_angle = 270 - res;
    }
    if (x_angle < 0 and y_angle > 0) { //in quadrant II
        final_angle = 90 - res;
    }
    if (x_angle < 0 and y_angle < 0) { //in quadrant III
        final_angle = 90 - res;
    }
    if (x_angle > 0 and y_angle < 0) { //in quadrant IV
        final_angle = 270 - res;
    }

    float old_min = float(min_angle);
    float old_max = float(max_angle);
    float new_min = float(min_value);
    float new_max = float(max_value);
    float old_value = final_angle;

    float old_range = (old_max - old_min);
    float new_range = (new_max - new_min);
    float new_value = (((old_value - old_min) * new_range) / old_range) + new_min;

    new_value = floor(new_value * 100.0 + 0.5) /100.0;
    std::cout<<new_value<<std::endl;

    return 0;

}
