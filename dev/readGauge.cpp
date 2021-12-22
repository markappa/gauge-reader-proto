/**
TODO list:
Done - Accept calibration arguments from commandline
Done - Option to do Canny on input file
Done - commandline options to perform calibration:
    step1 find center and radius
    step2 find start/end angles
    step3 find brightness threshold
Done (implicit in calibration) - option to output calibratrion files:
    1) center/radius
    2) angle compass
**/

#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <cxxopts.hpp>

#undef DEBUG
#undef CALIBRATE
#undef FIND_THRESHOLD
#undef CANNY

#define PI 3.1415826535
#define max(a,b) ((a<b)?b:a)
#define dist_2_pts(x1,y1,x2,y2) (sqrt(pow((x2)-(x1),2)+pow((y2)-(y1),2)))
#define rad2deg(a) ((a)*180.0/PI)
#define ERROR (-1)

bool verbose = false;
bool dryRun = false;
bool calibrate_step1 = false;
bool calibrate_step2 = false;
bool calibrate_step3 = false;
bool canny = false;
bool save_threshold_image = false;

using namespace cv;
int main(int argc, char** argv )
{
    //char *fname = NULL;
    std::string fname;
    int r = 83;
    Point2i c(405,242);
    int min_angle = 58;
    int max_angle = 322;
    int min_value = 0;
    int max_value = 4;
    int thresh = 95;
    int maxValue = 255;

    // worked: -r 83 -x 410 -y 240

    cxxopts::Options options(argv[0], "Read gauge value from an image.");

    options.add_options()
        ("r,radius", "Gauge screen radius", cxxopts::value<int>())
        ("c,center", "Gauge screen center x,y coordinates.", cxxopts::value<std::vector<int>>())
        ("A,angle-bounds", "Gauge angle min,max.", cxxopts::value<std::vector<int>>())	
        ("V,value-bounds", "Gauge value min,max.", cxxopts::value<std::vector<int>>())	
        ("t,threshold", "Threshold value for b/w img.", cxxopts::value<int>())
        ("m,max-value", "Max value for b/w img.", cxxopts::value<int>()->default_value("255"))
        ("s,calibrate-step", "Calibration stemp number (1,2 or 3).", cxxopts::value<int>())
        ("n,canny", "Canny img before line detection.")
        ("save_threshold_image", "Save b/w image.")
        ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
        ("d,dry-run", "Just parse options and exit")
        ("h,help", "Print usage")
        ("filename", "Input image file name", cxxopts::value<std::vector<std::string>>())
    ;

    options.positional_help("filename");

    options.parse_positional({"filename"});
    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
      std::cout << options.help() << std::endl;
      exit(0);
    }
    verbose = result["verbose"].as<bool>();
    dryRun = result["dry-run"].as<bool>();
    canny = result["canny"].as<bool>();
    save_threshold_image = result["save_threshold_image"].as<bool>();
    if (result.count("radius"))
        r = result["radius"].as<int>();
    if (result.count("center"))
    {
        c.x = result["center"].as<std::vector<int>>()[0];
        c.y = result["center"].as<std::vector<int>>()[1];
    }
    std::vector<int> tmp;
    if (result.count("angle-bounds"))
    {
        tmp = result["angle-bounds"].as<std::vector<int>>();
        min_angle = tmp[0];   
        max_angle = tmp[1];   
    }
    if (result.count("value-bouns"))
    {
        tmp = result["value-bounds"].as<std::vector<int>>();
        min_value = tmp[0];   
        max_value = tmp[1];   
    }
    if (result.count("threshold"))
        thresh = result["threshold"].as<int>();
    if (result.count("max-value"))
        maxValue = result["max-value"].as<int>();

    if (result.count("calibrate-step"))
    {
        int i = result["calibrate-step"].as<int>();
        if (verbose) std::cout << "Starting calibration step " << i << std::endl;
        switch (i)
        {
            case 1:
                calibrate_step1 = true;
                break;
            case 2:
                calibrate_step2 = true;
                break;
            case 3:
                calibrate_step3 = true;
                break;
            default:
                std::cout << "Unexpexted step value." << std::endl;
                exit ERROR;
        }
    }

    if (result.count("filename"))
    {
        auto& s = result["filename"].as<std::vector<std::string>>()[0];
        fname = s;
    }

    if (dryRun || verbose)
    {
        std::cout << "verbose " << verbose << std::endl;
        std::cout << "canny " << canny << std::endl;
        std::cout << "filename " << fname << std::endl;
        std::cout << "radius " << r << std::endl;
        std::cout << "center " << c << std::endl;
        std::cout << "angle-bounds " << min_angle << "," << max_angle << std::endl;
        std::cout << "value-bounds " << min_value << "," << max_value << std::endl;
        std::cout << "threshold " << thresh << std::endl;
        std::cout << "max-value " << maxValue << std::endl;
        std::cout << "calibrate-step " << calibrate_step1 << ":" <<calibrate_step2 << ":" <<calibrate_step3 << std::endl;

        if (dryRun)
        {
            std::cout << std::endl << "Skipping operations ( dry run)." << std::endl;
            return 0;
        }
    }

    Mat img;
    img = imread( fname, 1 );
    if ( !img.data )
    {
        fprintf(stderr, "No image data.\n");
        return -1;
    }
    if (verbose) printf("Loaded ok.\n");

    if (calibrate_step1) {
        int height, width;
        height = img.rows;
        width = img.cols;
        if (verbose) printf("File cols %d, rows %d.\n", width, height);

        Mat gray;
        cvtColor(img, gray, COLOR_BGR2GRAY);
        //imwrite("gray.jpg", gray);

        std::vector<Vec3f> circles;
        HoughCircles(gray, circles, HOUGH_GRADIENT, 1, 20, 100, 50);

        int n = int(circles.size());
        if (verbose) printf("Found %d circles.\n",n);

        if ( n == 0 )
            return ERROR;

        // Reset c and r to calculate mean value.
        c.x = c.y = r = 0;

        for( size_t i = 0; i < n; i++ )
        {
            Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
            int radius = cvRound(circles[i][2]);
            c += center;
            r += radius;

            if (verbose) std::cout << i << ": c/r " << center << "/" << radius << std::endl;

            // draw the circle center
            circle( img, center, 3, Scalar(0,0,0), -1, 8, 0 );
            // draw the circle outline
            circle( img, center, radius, Scalar(0,0,0), 3, 8, 0 );
        }
        c /= n;
        r = cvRound(r / n);
        if (verbose) std::cout << "avg : c/r " << c << "/" << r << std::endl;

        // draw the circle center
        circle( img, c, 3, Scalar(0,255,0), -1, 8, 0 );
        // draw the circle outline
        circle( img, c, r, Scalar(0,0,255), 3, 8, 0 );

        imwrite("calibration_step1.jpg", img);
    }
    // end of calibration step 1

    if (calibrate_step2) {
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

        // draw the circle center
        circle( img, c, 3, Scalar(0,255,0), -1, 8, 0 );
        // draw the circle outline
        circle( img, c, r, Scalar(0,0,255), 3, 8, 0 );

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
            if (verbose) std::cout << ".. " << p1[i] << " " <<p2[i] << std::endl;
        }

        imwrite("calibration_step2.jpg", img);
    }
    // end of calibration step 2

    Mat gray2;
    cvtColor(img, gray2, COLOR_BGR2GRAY);

    // Set threshold and maxValue
    Mat dst2;

    if (calibrate_step3) {
        for(thresh= 80; thresh<120; thresh += 5) {
            // apply thresholding which helps for finding lines
            threshold(gray2, dst2, thresh, maxValue, THRESH_BINARY_INV);

            // found Hough Lines generally performs better without Canny / blurring, though there were a couple exceptions where it would only work with Canny / blurring
            //dst2 = cv2.medianBlur(dst2, 5)
            if (canny)
                Canny(dst2, dst2, 50, 150, 3);

            //dst2 = cv2.GaussianBlur(dst2, (5, 5), 0)
            // show image after thresholding
            imwrite("calibration_step3_"+ std::to_string(thresh) +".jpg", dst2);
         }
         // end of calibration step 3
    } else {
         threshold(gray2, dst2, thresh, maxValue, THRESH_BINARY_INV);
         if(save_threshold_image)
             imwrite("threshold.jpg", dst2);
    }

    // find lines
    int minLineLength = 10;
    int maxLineGap = 6;
    int threshold = 50;
    std::vector<Vec4i> lines;

    HoughLinesP(dst2, lines, 3, PI / 360, threshold, minLineLength, maxLineGap);
    // rho is set to 3 to detect more lines, easier to get more then filter them out later

    if(verbose) std::cout << "Found " << std::to_string(lines.size()) << " lines.\n";
#ifdef DEBUG
    // for testing purposes, show all found lines
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        line( img, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, LINE_AA);
    }
    imwrite("righe_unfiltered.jpg", img);
#endif

    // remove all lines outside a given radius
    std::vector<Vec4i> final_line_list;

    if(verbose) std::cout << "radius: " << r << std::endl;

    float diff1LowerBound = 0.15; // diff1LowerBound and diff1UpperBound determine how close the line should be from the center
    float diff1UpperBound = 0.25;
    float diff2LowerBound = 0.5; // diff2LowerBound and diff2UpperBound determine how close the other point of the line should be to the outside of the gauge
    float diff2UpperBound = 1.0;
    int maxi; 
    float maxl=0.0f; // find the longest line from selectd

    for ( int i=0; i<lines.size(); i++)
    {
        int x1, y1, x2, y2;
        x1 = lines[i][0];
        y1 = lines[i][1];
        x2 = lines[i][2];
        y2 = lines[i][3];
        // c is center of circle
        float diff1 = dist_2_pts(c.x, c.y, x1, y1);
        float diff2 = dist_2_pts(c.x, c.y, x2, y2);
        // set diff1 to be the smaller (closest to the center) of the two), makes the math easier
        if (diff2 < diff1) {
           float tmp = diff2;
           diff2 = diff1;
           diff1 = tmp;
        }
        // check if line is within an acceptable range
        if ( (diff1<diff1UpperBound*r) && (diff1>diff1LowerBound*r)
           && (diff2<diff2UpperBound*r) && (diff2>diff2LowerBound*r)
           )
        {
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

#ifdef DEBUG
    // testing only, show all lines after filtering
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
 
    // show the line overlayed on the original image
    //cv2.line(img, (x1, y1), (x2, y2), (0, 255, 0), 2)
    //cv2.imwrite('gauge-1-test.jpg', img)
    //cv2.imwrite('gauge-%s-lines-2.%s' % (gauge_number, file_type), img)

    // Calculate the reading value from line angle

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

    // Convert angle to value
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
