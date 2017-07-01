/*
ading, copying, installing or using the software you agree to this
license. If you do not agree to this license, do not download, install,
copy or use the software.

                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2013, OpenCV Foundation, all rights reserved.
Third party copyrights are property of their respective owners.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the names of the copyright holders nor the names of the contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

This software is provided by the copyright holders and contributors "as is" and
any express or implied warranties, including, but not limited to, the implied
warranties of merchantability and fitness for a particular purpose are
disclaimed. In no event shall copyright holders or contributors be liable for
any direct, indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.
*/


#include <opencv2/highgui.hpp>
#include <opencv2/aruco.hpp>
#include "opencv2/opencv.hpp"
#include <iostream>

using namespace std;
using namespace cv;

namespace {
const char* about = "Basic marker detection";
const char* keys  =
		"{h        |       | Horizontal output resolution }"
		"{v        |       | Vertical output resolution }"
		"{i        |       | Input image }"
		"{o        |       | Output image }";
}

/**
 */
static bool readDetectorParameters(string filename, Ptr<aruco::DetectorParameters> &params) {
    FileStorage fs(filename, FileStorage::READ);
    if(!fs.isOpened())
        return false;
    fs["adaptiveThreshWinSizeMin"] >> params->adaptiveThreshWinSizeMin;
    fs["adaptiveThreshWinSizeMax"] >> params->adaptiveThreshWinSizeMax;
    fs["adaptiveThreshWinSizeStep"] >> params->adaptiveThreshWinSizeStep;
    fs["adaptiveThreshConstant"] >> params->adaptiveThreshConstant;
    fs["minMarkerPerimeterRate"] >> params->minMarkerPerimeterRate;
    fs["maxMarkerPerimeterRate"] >> params->maxMarkerPerimeterRate;
    fs["polygonalApproxAccuracyRate"] >> params->polygonalApproxAccuracyRate;
    fs["minCornerDistanceRate"] >> params->minCornerDistanceRate;
    fs["minDistanceToBorder"] >> params->minDistanceToBorder;
    fs["minMarkerDistanceRate"] >> params->minMarkerDistanceRate;
    fs["doCornerRefinement"] >> params->doCornerRefinement;
    fs["cornerRefinementWinSize"] >> params->cornerRefinementWinSize;
    fs["cornerRefinementMaxIterations"] >> params->cornerRefinementMaxIterations;
    fs["cornerRefinementMinAccuracy"] >> params->cornerRefinementMinAccuracy;
    fs["markerBorderBits"] >> params->markerBorderBits;
    fs["perspectiveRemovePixelPerCell"] >> params->perspectiveRemovePixelPerCell;
    fs["perspectiveRemoveIgnoredMarginPerCell"] >> params->perspectiveRemoveIgnoredMarginPerCell;
    fs["maxErroneousBitsInBorderRate"] >> params->maxErroneousBitsInBorderRate;
    fs["minOtsuStdDev"] >> params->minOtsuStdDev;
    fs["errorCorrectionRate"] >> params->errorCorrectionRate;
    return true;
}


Point2f GetCenterId( int id, vector< int > &ids, vector< vector< Point2f > > &corners)
{
	int iIndexFound = -1;
	for (int index = 0; index < ids.size(); index++)
	{
		if (ids[index]==id)
		{
			iIndexFound = index;
		}
	}
	if (iIndexFound >= 0)
	{
		Point p1 = Point(corners[iIndexFound][0].x, corners[iIndexFound][0].y);
		Point p2 = Point(corners[iIndexFound][1].x, corners[iIndexFound][1].y);
		Point p3 = Point(corners[iIndexFound][2].x, corners[iIndexFound][2].y);
		Point p4 = Point(corners[iIndexFound][3].x, corners[iIndexFound][3].y);
		return Point2f((p1+p2+p3+p4)/4);
	}
	return Point2f(0,0);
}

void overlayImage(const cv::Mat &background, const cv::Mat &foreground,
  cv::Mat &output, cv::Point2i location)
{
  background.copyTo(output);


  // start at the row indicated by location, or at row 0 if location.y is negative.
  for(int y = std::max(location.y , 0); y < background.rows; ++y)
  {
    int fY = y - location.y; // because of the translation

    // we are done of we have processed all rows of the foreground image.
    if(fY >= foreground.rows)
      break;

    // start at the column indicated by location,

    // or at column 0 if location.x is negative.
    for(int x = std::max(location.x, 0); x < background.cols; ++x)
    {
      int fX = x - location.x; // because of the translation.

      // we are done with this row if the column is outside of the foreground image.
      if(fX >= foreground.cols)
        break;

      // determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
      double opacity =
        ((double)foreground.data[fY * foreground.step + fX * foreground.channels() + 3])

        / 255.;


      // and now combine the background and foreground pixel, using the opacity,

      // but only if opacity > 0.
      for(int c = 0; opacity > 0 && c < output.channels(); ++c)
      {
        unsigned char foregroundPx =
          foreground.data[fY * foreground.step + fX * foreground.channels() + c];
        unsigned char backgroundPx =
          background.data[y * background.step + x * background.channels() + c];
        output.data[y*output.step + output.channels()*x + c] =
          backgroundPx * (1.-opacity) + foregroundPx * opacity;
      }
    }
  }
}

/**
 */
int main(int argc, char *argv[]) {
    CommandLineParser parser(argc, argv, keys);
    parser.about(about);

    // Get input image
    if(argc < 1) {
        parser.printMessage();
        return 1;
    }

    String inputFile = "../markers/input.jpg";
    if(parser.has("i")) {
    	inputFile = parser.get<String>("i");
    }
	printf("Input %s\n", inputFile.c_str());

    String outputFile = "./output.jpg";
    if(parser.has("o")) {
    	outputFile = parser.get<String>("o");
    }
	printf("Output %s\n", outputFile.c_str());

    int iVerticalResolution = 1080;
    if(parser.has("v")) {
    	iVerticalResolution = parser.get<int>("v");
    }
	printf("Vertical resolution %d\n", iVerticalResolution);

    int iHorizontalResolution = 1920;
    if(parser.has("h")) {
    	iHorizontalResolution = parser.get<int>("h");
    }
	printf("Horizontal resolution %d\n", iHorizontalResolution);

    Mat imageSource = cv::imread(inputFile, CV_LOAD_IMAGE_UNCHANGED);
    if (imageSource.rows == 0) {
    	cerr << "Invalid input image" << endl;
        return 2;
    }

    // Setup Aruco
    Ptr<aruco::DetectorParameters> detectorParams = aruco::DetectorParameters::create();
//    if(parser.has("dp")) {
//        bool readOk = readDetectorParameters(parser.get<string>("dp"), detectorParams);
//        if(!readOk) {
//            cerr << "Invalid detector parameters file" << endl;
//            return 0;
//        }
//    }
    detectorParams->doCornerRefinement = true; // do corner refinement in markers

    if(!parser.check()) {
        parser.printErrors();
        return 3;
    }

    // Dictionay id = 0
    Ptr<aruco::Dictionary> dictionary =
        aruco::getPredefinedDictionary(aruco::PREDEFINED_DICTIONARY_NAME(0));



    // Detect
    vector< int > ids;
    vector< vector< Point2f > > corners, rejected;
    // detect markers and estimate pose
    aruco::detectMarkers(imageSource, dictionary, corners, ids, detectorParams, rejected);

    // Markers
    //Mat imageMarkers;
    //imageSource.copyTo(imageMarkers);
    //aruco::drawDetectedMarkers(imageMarkers, corners, ids);
    //imshow("Markers", imageMarkers);

    if (ids.size() < 4)
    {
    	return 4;
    }

	// Four corners of destination image
	vector<Point2f> pts_dst;
	pts_dst.push_back(Point2f(0, 0));
	pts_dst.push_back(Point2f(iHorizontalResolution-1, 0));
	pts_dst.push_back(Point2f(0, iVerticalResolution-1));
	pts_dst.push_back(Point2f(iHorizontalResolution-1, iVerticalResolution-1));

	// Store markers image source
	vector<Point2f> pts_src;
	pts_src.push_back(GetCenterId(2, ids, corners));
	pts_src.push_back(GetCenterId(0, ids, corners));
	pts_src.push_back(GetCenterId(3, ids, corners));
	pts_src.push_back(GetCenterId(1, ids, corners));

	// Calculate Homography
	Mat h = findHomography(pts_src, pts_dst);

	// Output image
	Mat imageWarped;
	// Warp source image to destination based on homography
	warpPerspective(imageSource, imageWarped, h, imageSource.size());
	//imshow("Destination", imageWarped);



	cv::Rect roi;
	roi.width = iHorizontalResolution;
	roi.height = iVerticalResolution;
	cv::Mat imageOutput = imageWarped(roi);
	//imshow("Ouput", imageOutput);
	imwrite( outputFile, imageOutput );
	return 0;
}


