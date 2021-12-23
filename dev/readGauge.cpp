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
#define LINE_WIDTH 1
#define trace(val) if(verbose) std::cerr << "[trace] " << #val << " = " << (val) << std::endl

bool verbose = false;
bool dryRun = false;
bool canny = false;
bool image_out = false;

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
    // diff1LowerBound and diff1UpperBound determine how close the line should be from the center
    float diff1LowerBound = 0.0;
    float diff1UpperBound = 0.45;
    // diff2LowerBound and diff2UpperBound determine how close the other point of the line should be to the outside of the gauge
    float diff2LowerBound = 0.8;
    float diff2UpperBound = 1.2;

    cxxopts::Options options(argv[0], "Read gauge value from an image.");

    options.add_options()
        ("r,radius", "Gauge screen radius", cxxopts::value<int>())
        ("c,center", "Gauge screen center x,y coordinates.", cxxopts::value<std::vector<int>>())
        ("A,angle-bounds", "Gauge angle min,max.", cxxopts::value<std::vector<int>>())	
        ("V,value-bounds", "Gauge value min,max.", cxxopts::value<std::vector<int>>())	
        ("t,threshold", "Threshold value for b/w img.", cxxopts::value<int>())
        ("l,line-bounds", "Line limits [%] minNear,maxNear,minFar,maxFar.", cxxopts::value<std::vector<int>>())	
        ("n,canny", "Canny img before line detection.")
        ("i,save_images", "Save calibration images.")
        ("verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
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
    image_out = result["save_images"].as<bool>();
    if (result.count("radius"))
        r = result["radius"].as<int>();
    if (result.count("center"))
    {
        c.x = result["center"].as<std::vector<int>>()[0];
        c.y = result["center"].as<std::vector<int>>()[1];
    }
    std::vector<int> tmp;
    if (result.count("line-bounds"))
    {
        tmp = result["line-bounds"].as<std::vector<int>>();
	if (tmp.size() == 4) {
          diff1LowerBound = tmp[0]/100.0;   
          diff1UpperBound = tmp[1]/100.0;   
          diff2LowerBound = tmp[2]/100.0;   
          diff2UpperBound = tmp[3]/100.0;
	} else {
          std::cerr << "Invalid line-bounds option format ("<<tmp.size() <<")." << std::endl;
          exit(0);
	}
    }
    if (result.count("angle-bounds"))
    {
        tmp = result["angle-bounds"].as<std::vector<int>>();
        min_angle = tmp[0];   
        max_angle = tmp[1];   
    }

    if (result.count("value-bounds"))
    {
        tmp = result["value-bounds"].as<std::vector<int>>();
	if (tmp.size() == 2) {
          min_value = tmp[0];   
          max_value = tmp[1];   
	} else {
          std::cout << "Invalid value-bounds option format ("<<tmp.size() <<")." << std::endl;
          exit(0);
        }
    }
    if (result.count("threshold"))
        thresh = result["threshold"].as<int>();

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
        std::cout << "save_images " << image_out << std::endl;
	std::cout << "line-bounds " << diff1LowerBound << "-" << diff1UpperBound << " " << diff2LowerBound << "-" << diff2UpperBound << std::endl;

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

    // find auto center if center was not provided
    Mat img1;
    if (!result.count("center")) {
        int height, width;
        height = img.rows;
        width = img.cols;
        if (verbose) printf("File cols %d, rows %d.\n", width, height);

        Mat gray;
        cvtColor(img, gray, COLOR_BGR2GRAY);
        //imwrite("gray.jpg", gray);
	
	if(image_out) {
          img1= img.clone();
	}

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

            if (image_out) {
              // draw the circle center
              circle( img1, center, 3, Scalar(0,0,0), -1, 8, 0 );
              // draw the circle outline
              circle( img1, center, radius, Scalar(0,0,0), LINE_WIDTH, 8, 0 );
	    }
        }
        c /= n;
        r = cvRound(r / n);
        if (verbose) std::cout << "avg : c/r " << c << "/" << r << std::endl;
    }
    // end of calibration step 1

    // Resize image to keep only the meter
    Rect rec(c.x-r, c.y-r, r*2, r*2);
    if (verbose) std::cout << "Crop to " << rec << std::endl;
    Mat sImg = img(rec);
    img.release();
    c.x = c.y = r;

    if ( image_out ) {
	Mat sImg1;
        if ( !img1.empty() ) {
	  sImg1 = img1(rec);
	  img1.release();
	} else
	  sImg1 = sImg.clone();
        // draw the circle center
        circle( sImg1, c, 3, Scalar(0,255,0), -1, 8, 0 );
        // draw the circle outline
        circle( sImg1, c, r, Scalar(0,0,255), LINE_WIDTH, 8, 0 );

        imwrite("calibration_step1.jpg", sImg1);
	sImg1.release();
    }
    
    if (image_out) {
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
	Mat sImg2 = sImg.clone();

        // draw the circle center
        circle( sImg2, c, 3, Scalar(0,255,0), -1, 8, 0 );
        // draw the circle outline
        circle( sImg2, c, r, Scalar(0,0,255), LINE_WIDTH, 8, 0 );

        for ( int i=0; i<interval; i++) {
            p1[i].x = c.x + 0.9 * r * cos(separation * i * 3.14 / 180); // #point for lines
            p1[i].y = c.y + 0.9 * r * sin(separation * i * 3.14 / 180);
        }

        int text_offset_x = 10;
        int text_offset_y = 5;
        for ( int i=0; i<interval; i++) {
            p2[i].x = c.x + r * cos(separation * i * 3.14 / 180);
            p2[i].y = c.y + r * sin(separation * i * 3.14 / 180);
            p_text[i].x = c.x - text_offset_x + 0.8 * r * cos((separation) * (i+9) * 3.14 / 180); // point for text labels, i+9 rotates the labels by 90 degrees
            p_text[i].y = c.y + text_offset_y + 0.8 * r * sin((separation) * (i+9) * 3.14 / 180); //  point for text labels, i+9 rotates the labels by 90 degrees
        }

        // add the lines and labels to the image
        for ( int i=0; i<interval; i++) {
            line(sImg2, p1[i], p2[i], Scalar(0, 255, 0), LINE_WIDTH);
            if((i%3)==0)
	      putText(sImg2, std::to_string(int(i*separation)), p_text[i], FONT_HERSHEY_SIMPLEX, 0.3f, Scalar(0,0,0),1,LINE_AA);
            //if (verbose) std::cout << ".. " << p1[i] << " " <<p2[i] << std::endl;
        }

	// add the two angle limits lines
#define drawSectLine(mat,deg,color) line((mat), c, Point2i(c.x + r * cos((90 + (deg)) * 3.14 / 180),c.y + r * sin((90 +(deg)) * 3.14 / 180)), (color), LINE_WIDTH)	
#define drawText(mat,i,s,col) putText((mat), (s),\
            Point2i( \
              c.x -text_offset_x +0.3*r*cos((90+min_angle+(i))*3.14/180), \
              c.y -text_offset_y +0.3*r*sin((90+min_angle+(i))*3.14/180)), \
	    FONT_HERSHEY_SIMPLEX, 0.3f, (col),1,LINE_AA)
	Point2i min, max;
	drawSectLine(sImg2, min_angle, Scalar(255,0,0));
	drawSectLine(sImg2, max_angle, Scalar(255,0,0));

	// add main values lines
	int sectors = ( (max_value-min_value) / (pow(10,trunc(log10(max_value-min_value)))));
	int increment = (max_angle - min_angle) / sectors;
	int vinc = (max_value - min_value) / sectors;
	for( int i=0; i<=sectors; i++) {
	  if(i!=0 && i!=sectors) // skip lines st ends
            drawSectLine(sImg2, min_angle + i * increment, Scalar(55,0,0));
	  drawText(sImg2, i*increment, std::to_string(int(i*vinc)),Scalar(70,0,0));
	}
	// draw first and last value
	//drawText(sImg2, 0, std::to_string(min_value),Scalar(70,0,0));
	//drawText(sImg2, sectors, std::to_string(max_value),Scalar(70,0,0));

        imwrite("calibration_step2.jpg", sImg2);
	sImg2.release();
    }
    // end of calibration step 2

    Mat gray2;
    cvtColor(sImg, gray2, COLOR_BGR2GRAY);

    // Set threshold and maxValue
    Mat dst2;
    int maxValue = 255;

    if (image_out) {
        for(int th = thresh-20; th <thresh+20; th += 5) {
            // apply thresholding which helps for finding lines
            threshold(gray2, dst2, th, maxValue, THRESH_BINARY_INV);

            // found Hough Lines generally performs better without Canny / blurring, though there were a couple exceptions where it would only work with Canny / blurring
            //dst2 = cv2.medianBlur(dst2, 5)
            if (canny)
                Canny(dst2, dst2, 50, 150, 3);

            //dst2 = cv2.GaussianBlur(dst2, (5, 5), 0)
            // show image after thresholding
	    char tstr [30];
	    sprintf (tstr, "calibration_step3_%03d.jpg", th);
            
            imwrite(tstr, dst2);
         }
         // end of calibration step 3
    }
    threshold(gray2, dst2, thresh, maxValue, THRESH_BINARY_INV);
    if(image_out)
      imwrite("calibration_step3.jpg", dst2);

    // check if image is corrupted
    int channels[] = {0};
    MatND hist;
    int histSize[] = {2};
    float vranges[] = { 0, 256 };
    const float* ranges[] = { vranges };

    calcHist( &dst2, 1, channels, Mat(), // do not use mask
      hist, 1, histSize, ranges,
      true, // the histogram is uniform
      false );

    trace(hist);
    trace(hist.at<float>(0, 0));
    trace(hist.at<float>(0, 1));

    if ( hist.at<float>(0, 0) < hist.at<float>(0, 1) ) {
      std::cerr << "Invalid image brightness." << std::endl;
      exit(ERROR);
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
        line( sImg, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), LINE_WIDTH, LINE_AA);
    }
    imwrite("righe_unfiltered.jpg", sImg);
#endif

    // remove all lines outside a given radius
    std::vector<Vec4i> final_line_list;

    // find the longest line from selectd
    int maxi=-1; 
    float maxl=0.0f;

    if (verbose) {
	    std::cout << diff1LowerBound << "-" << diff1UpperBound << " " << diff2LowerBound << "-" << diff2UpperBound << std::endl;
	    std::cout << "loop lines: " << std::endl;
	    std::cout << std::fixed;
	    std::cout.precision(2);
    }

    int i;
    for ( i=0; i<lines.size(); i++)
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
	// Also reorder the line so that x1,y1 is near the center
        if (diff2 < diff1) {
           float tmp = diff2;
           diff2 = diff1;
           diff1 = tmp;
	   int t=x1; x1=x2; x2=t;
	   t=y1; y1=y2; y2=t;
        }

        if(verbose) std::cout << diff1/r << " - " << diff2/r << 
		" " << dist_2_pts(x1, y1, x2, y2) << std::endl;

        // check if line is within an acceptable range
        if ( (diff1<diff1UpperBound*r) && (diff1>diff1LowerBound*r)
           && (diff2<diff2UpperBound*r) && (diff2>diff2LowerBound*r)
           )
        {
	    // record the one with the farthest point (deeper in the point of the dial)
            float line_length = dist_2_pts(c.x, c.y, x2, y2);
            if(line_length>maxl){
               maxi = final_line_list.size();
               maxl = line_length;
            }
            // add to final list
            Vec4i l{x1, y1, x2, y2};
            final_line_list.push_back(l);
        }
    }
    if(verbose) std::cout << "end on " << i << std::endl <<
	    "maxi is " << maxi << std::endl <<
            "lines are " << std::to_string(final_line_list.size()) << std::endl;

    if ( final_line_list.size() == 0 )
    {
      std::cerr << "I could not find the dial." << std::endl;
      exit(ERROR);
    }

    if(image_out)
    {
      // show all lines after filtering
      Mat sImg4 = sImg.clone();
      if (verbose) std::cout << "Remaining " << std::to_string(final_line_list.size()) << " lines.\n";
      for( size_t i = 0; i < final_line_list.size(); i++ )
      {
          Vec4i l = final_line_list[i];
          if(i==maxi) {
             line( sImg4, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,255,0), LINE_WIDTH, LINE_AA);
          } else {
             line( sImg4, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255,0,0), LINE_WIDTH, LINE_AA);
          }
      }

      // shov line limits
      circle( sImg4, c, r * diff1LowerBound, Scalar(55,0,0), LINE_WIDTH, 8, 0 );
      circle( sImg4, c, r * diff1UpperBound, Scalar(55,0,0), LINE_WIDTH, 8, 0 );
      circle( sImg4, c, r * diff2LowerBound, Scalar(0,0,55), LINE_WIDTH, 8, 0 );
      circle( sImg4, c, r * diff2UpperBound, Scalar(0,0,55), LINE_WIDTH, 8, 0 );

      imwrite("calibration_step4.jpg", sImg4);
      sImg4.release();
    }

    // assumes the longest line is the best one
    int x1, y1, x2, y2;
    x1 = final_line_list[maxi][0];
    y1 = final_line_list[maxi][1];
    x2 = final_line_list[maxi][2];
    y2 = final_line_list[maxi][3];
 
    if(verbose) std::cout << "Checkpoint 0" << std::endl;

    // Calculate the reading value from line angle

    // use asin to determine angle
    // y2-y1 = sin(a)*l
    // use center instead of first point: more accurate
    float len = dist_2_pts(c.x,c.y,x2,y2);
    float final_angle = asin((c.y-y2)/len)*180/3.1415626 + 90;
    if(final_angle>360) final_angle -= 360.0;

    trace(final_angle);
    trace("Checkpoint 2");

    // Convert angle to value
#define angle2value(ang) ((((ang)-min_angle)*(max_value-min_value))/(max_angle-min_angle))+min_value
    float new_value = angle2value(final_angle);

    trace("Checkpoint 3");

    std::cout << std::fixed;
    std::cout.precision(2);
    std::cout<<new_value<<std::endl;

    return 0;
}
