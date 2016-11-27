#include <iostream> // for standard I/O
#include <string>   // for strings
#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp>  // Video write
#include "opencv2/imgproc/imgproc.hpp"
#include <string.h>
#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <regex>
#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>

using namespace log4cxx;
using namespace log4cxx::xml;
using namespace log4cxx::helpers;

using namespace std;
using namespace cv;

#define CFG_FILE "/home/johnkok/md.xml"
#define CFG_LOG  "/home/johnkok/log.xml"

int debug = 0;
static Mat MaxImg;
int MaxImgInit = 0;
int ImgDetected = 0;

typedef struct cfg_{
   unsigned int index;
   unsigned int roi_x_start;
   unsigned int roi_x_stop;
   unsigned int roi_y_start;
   unsigned int roi_y_stop;
   unsigned int threshold;
   unsigned int sampling;
}cfg;
cfg cam_cfg;

LoggerPtr logDefault(Logger::getLogger( "Default"));

int read_config(unsigned int cam_index){
xmlDocPtr doc;
xmlNodePtr cur, root, cur_2, cur_3;
unsigned int found = 0;

   doc = xmlReadFile(CFG_FILE, NULL, 0);
   if (doc == NULL) {
      LOG4CXX_ERROR(logDefault, "Failed to parse " << CFG_FILE);
      return -1;
   }

   root = xmlDocGetRootElement(doc);
   for (cur = root; cur; cur = cur->next) {
      if (cur->type == XML_ELEMENT_NODE && !strcmp((const char*)cur->name, "IOKO")){
         for (cur_2 = cur->children; cur_2; cur_2 = cur_2->next) {
            if (cur_2->type == XML_ELEMENT_NODE && !strcmp((const char*)cur_2->name, "camera")){
               for (cur_3 = cur_2->children; cur_3; cur_3 = cur_3->next) {
                  if (!strcmp((const char*)cur_3->name, "index")){
                     cam_cfg.index = atoi((const char*)cur_3->children->content);
                     if (cam_cfg.index == cam_index) found = 1;
                  }else if(!strcmp((const char*)cur_3->name, "x_low")){
                     cam_cfg.roi_x_start = atoi((const char*)cur_3->children->content);
                  }else if(!strcmp((const char*)cur_3->name, "x_high")){
                     cam_cfg.roi_x_stop = atoi((const char*)cur_3->children->content);
                  }else if(!strcmp((const char*)cur_3->name, "y_low")){
                     cam_cfg.roi_y_start = atoi((const char*)cur_3->children->content);
                  }else if(!strcmp((const char*)cur_3->name, "y_high")){
                     cam_cfg.roi_y_stop = atoi((const char*)cur_3->children->content);
                  }else if(!strcmp((const char*)cur_3->name, "sampling")){
                     cam_cfg.sampling = atoi((const char*)cur_3->children->content);
                  }else if(!strcmp((const char*)cur_3->name, "threshold")){
                     cam_cfg.threshold = atoi((const char*)cur_3->children->content);
                  }
               }
               if (found){ 
                  cout << "Using cfg (" << cam_cfg.roi_x_start << ":" << cam_cfg.roi_x_stop << ")"
                                    "(" << cam_cfg.roi_y_start << ":" << cam_cfg.roi_y_stop << ")"
                                    "(" << cam_cfg.sampling << ":" << cam_cfg.threshold << ")"
                                    " for cam " << cam_cfg.index << endl;
                  xmlFreeDoc(doc);
                  LOG4CXX_DEBUG(logDefault, "Using cfg (" << cam_cfg.roi_x_start << ":" << cam_cfg.roi_x_stop << ")"
                                    "(" << cam_cfg.roi_y_start << ":" << cam_cfg.roi_y_stop << ")"
                                    "(" << cam_cfg.sampling << ":" << cam_cfg.threshold << ")"
                                    " for cam " << cam_cfg.index );
                  return 0;
               }
            }
         }
      }
   }

   xmlFreeDoc(doc);
   LOG4CXX_ERROR(logDefault, "Configuration for cam " << cam_index << " not found in " << CFG_FILE);
   return -1;
}

int diffFrames (Mat img1, Mat img2, int cam = -1, long int cnt = 0, string src = "", long int tf = 0){
int histSize = 8;
float range[] = { 0, 256 } ;
const float* histRange = { range };
Mat hist;
Mat dst1,dst2;
Mat gdst1,gdst2;
Mat res;
vector<Mat> bgr_planes;
int threshold;

        blur( img1(Rect(cam_cfg.roi_x_start, cam_cfg.roi_y_start, cam_cfg.roi_x_stop, cam_cfg.roi_y_stop)), dst1, Size( 4, 4 ));
        blur( img2(Rect(cam_cfg.roi_x_start, cam_cfg.roi_y_start, cam_cfg.roi_x_stop, cam_cfg.roi_y_stop)), dst2, Size( 4, 4 ));

        cvtColor(dst1, gdst1, CV_BGR2GRAY);
        cvtColor(dst2, gdst2, CV_BGR2GRAY);
        absdiff(gdst1,gdst2,res);

        if (!MaxImgInit){
           res.copyTo(MaxImg );
           MaxImgInit = 1;
        }

        split( res, bgr_planes );

        cv::Size res_size = res.size();
        calcHist( &bgr_planes[0], 1, 0, Mat(), hist, 1, &histSize, &histRange, 1, 0 );
        threshold = res_size.height * res_size.width * (1000 - cam_cfg.threshold) / 1000;

        if (hist.at<float>(0,0)  < (float)threshold && 
           ( hist.at<float>(7,0)  || hist.at<float>(6,0)  || hist.at<float>(5,0)  /*|| hist.at<float>(4,0)*/)){

           max(MaxImg,res,MaxImg);   
           ImgDetected++;

           if (debug){
              String text;
              stringstream ss (stringstream::in | stringstream::out);
              ss.str("");
              ss << "0: " << hist.at<float>(0,0) ;
              text = ss.str();	
              putText(res	, text, cvPoint(30,25), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
              ss.str("");
              ss << "1: " << hist.at<float>(1,0) ;
              text = ss.str();	
              putText(res	, text, cvPoint(30,50), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
              ss.str("");
              ss << "2: " << hist.at<float>(2,0) ;
              text = ss.str();	
              putText(res	, text, cvPoint(30,75), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
              ss.str("");
              ss << "3: " << hist.at<float>(3,0) ;
              text = ss.str();	
              putText(res	, text, cvPoint(30,100), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
              ss.str("");
              ss << "4: " << hist.at<float>(4,0) ;
              text = ss.str();	
              putText(res	, text, cvPoint(30,125), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
              ss.str("");
              ss << "5: " << hist.at<float>(5,0) ;
              text = ss.str();	
              putText(res	, text, cvPoint(30,150), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
              ss.str("");
              ss << "6: " << hist.at<float>(6,0) ;
              text = ss.str();	
              putText(res	, text, cvPoint(30,175), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
              ss.str("");
              ss << "7: " << hist.at<float>(7,0) ;
              text = ss.str();	
              putText(res	, text, cvPoint(30,200), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
              ss.str("");
              if (tf) ss << "" << (cnt * 100) / tf << "%";
              text = ss.str();	
              putText(res	, text, cvPoint(30,230), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
              putText(res	, src, cvPoint(30,260), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);

              char fn[64];
              sprintf(fn, "mtn/cam%d/%sfr_%ld.jpg", cam, src.c_str(), cnt);
              imwrite (fn,res);
           }
           LOG4CXX_DEBUG(logDefault, "Frame: " << cnt << " Results: " << hist.at<float>(0,0) );
        }

	return hist.at<float>(0,0);
}

int main(int argc, char *argv[]){
bool status = 1;
long int count = 0;
int cam = -1;
Mat img[2];
char str_buffer[256];

    // Load XML configuration file using DOMConfigurator
    DOMConfigurator::configure("log.xml");


    if (argc < 2){
        LOG4CXX_ERROR(logDefault, "Not enough parameters (" << argc << ") !");
        cout << "Not enough parameters" << endl;
        cout << "Usage: md CAMx... " << endl;
        return -1;
    }

    if (argc > 2)
        debug = atoi(argv[2]);

    const string source = argv[1];

    VideoCapture inputVideo(source);              // Open input
    if (!inputVideo.isOpened()){
        LOG4CXX_ERROR(logDefault, "Could not open the input video: " << source );
        return -1;
    }

    if (source.compare(1, 3, "CAM")){
       String camera = source.substr(3,source.find("_")-3);
       cam = stoi(camera);
    }else{
       LOG4CXX_ERROR(logDefault, "Invalid filename " << source );
       return -1;
    }

    sprintf(str_buffer, "mkdir -p mtn/cam%d", cam);       
    if (system(str_buffer) < 0){
       LOG4CXX_ERROR(logDefault, "Error creating output directory");
       return -1;
    }

    if (read_config(cam) != 0){
       LOG4CXX_ERROR(logDefault, "Error reading configuration");
       return -1;
    }

    LOG4CXX_INFO(logDefault, "Processing file: " << source );

    int totalframe = inputVideo.get(CV_CAP_PROP_FRAME_COUNT);
    Size S = Size((int) inputVideo.get(CV_CAP_PROP_FRAME_WIDTH),    // Acquire input size
                  (int) inputVideo.get(CV_CAP_PROP_FRAME_HEIGHT));
    LOG4CXX_INFO(logDefault, "Camera: " << cam << " Frames: " << totalframe << " Size: " << S );

    inputVideo.read(img[0]);
    while (status){
       for (unsigned int i=0; i < cam_cfg.sampling  ; i++){
          status = inputVideo.read(img[1]);
          if (!status)
             break;
          count++;
       }
       if (status){
          diffFrames (img[0], img[1], cam, count, source, totalframe);
          img[1].copyTo(img[0]);
       }
   }

   if (MaxImgInit && ImgDetected){
      char fn[64];
      sprintf(fn, "mtn/cam%d/%s_max.jpg", cam, source.c_str());
      imwrite (fn,MaxImg);
   }

   return 0;
}



