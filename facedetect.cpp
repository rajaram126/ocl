#include "opencv2/contrib/contrib.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/ocl/ocl.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>

#define VIDEO_DEVICE "/dev/video1"
#define FRAME_WIDTH  640
#define FRAME_HEIGHT 480

#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 3)

using namespace cv;
using namespace std;

static void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';') {
    std::ifstream file(filename.c_str(), ifstream::in);
    string line, path, classlabel;
    while (getline(file, line)) {
        stringstream liness(line);
        getline(liness, path, separator);
        getline(liness, classlabel);
        if(!path.empty() && !classlabel.empty()) {
            images.push_back(imread(path, 0));
            labels.push_back(atoi(classlabel.c_str()));
        }
    }
}

int main(int argc, const char *argv[]) {
    // Check for valid command line arguments, print usage
    // if no arguments were given.
    if (argc != 4) {
        cout << "usage: " << argv[0] << " </path/to/haar_cascade> </path/to/csv.ext> </path/to/device id>" << endl;
        exit(1);
    }
    string fn_haar = string(argv[1]);
    string fn_csv = string(argv[2]);
    int deviceId = atoi(argv[3]);
    vector<Mat> images;
    vector<int> labels;
    try {
        read_csv(fn_csv, images, labels);
    } catch (cv::Exception& e) {
        cerr << "Error opening file \"";
        exit(1);
    }
        struct v4l2_capability vid_caps;
        struct v4l2_format vid_format;

        __u8 buffer[FRAME_SIZE];
        __u8 check_buffer[FRAME_SIZE];

        int i;
        for (i = 0; i < FRAME_SIZE; ++i) {
                buffer[i] = i % 2;
                check_buffer[i] = 0;
        }

        int fdwr = open(VIDEO_DEVICE, O_RDWR);
        assert(fdwr >= 0);

        int ret_code = ioctl(fdwr, VIDIOC_QUERYCAP, &vid_caps);
        assert(ret_code != -1);

        memset(&vid_format, 0, sizeof(vid_format));

        vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        vid_format.fmt.pix.width = FRAME_WIDTH;
        vid_format.fmt.pix.height = FRAME_HEIGHT;
        vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
        vid_format.fmt.pix.sizeimage = FRAME_WIDTH * FRAME_HEIGHT * 3;
        vid_format.fmt.pix.field = V4L2_FIELD_NONE;
        vid_format.fmt.pix.bytesperline = FRAME_WIDTH * 3;
        vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
        ret_code = ioctl(fdwr, VIDIOC_S_FMT, &vid_format);
        assert(ret_code != -1);

    int im_width = images[0].cols;
    int im_height = images[0].rows;
    Ptr<FaceRecognizer> model = createFisherFaceRecognizer();
    model->train(images, labels);
    ocl::OclCascadeClassifier haar_cascade;
    haar_cascade.load(fn_haar);
    VideoCapture cap(deviceId);
    if(!cap.isOpened()) {
        cerr << "Capture Device ID " << deviceId << "cannot be opened." << endl;
        return -1;
    }
    Mat frame;
    for(;;) {
        cap >> frame;
        Mat original = frame.clone();
        ocl::oclMat original_1(frame.clone());
        ocl::oclMat gray;
        ocl::cvtColor(original_1, gray, CV_BGR2GRAY);
        vector< Rect_<int> > faces;
        haar_cascade.detectMultiScale(gray, faces, 1.1,
                              3, 0
                              |CV_HAAR_SCALE_IMAGE
                              , Size(30,30), Size(0, 0));
        for(int i = 0; i < faces.size(); i++) {
            Rect face_i = faces[i];
            ocl::oclMat face = gray(face_i);
            ocl::oclMat face_resized;
            Mat face_resized2;
            ocl::resize(face, face_resized, Size(im_width, im_height), 1.0, 1.0,  INTER_LINEAR);
            face_resized2 = face_resized;
            int prediction = model->predict(face_resized2);
            rectangle(original, face_i, CV_RGB(0, 255,0), 1);
            string box_text = format("Prediction = %d", prediction);
            int pos_x = std::max(face_i.tl().x - 10, 0);
            int pos_y = std::max(face_i.tl().y - 10, 0);
            putText(original, box_text, Point(pos_x, pos_y), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0,255,0), 2.0);
        }
        imshow("face_recognizer", original);
	cvtColor(original, original, CV_RGB2BGR);
	IplImage frame= original;
	 IplImage* des_img;
    des_img=cvCreateImage(cvSize(640,480),(&frame)->depth,(&frame)->nChannels);
    cvResize(&frame,des_img,CV_INTER_LINEAR);

	write(fdwr, des_img->imageData, FRAME_SIZE);
        
        char key = (char) waitKey(20);
        if(key == 27)
            break;
    }
    return 0;
}
