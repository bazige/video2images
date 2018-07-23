#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/contrib/cpmtrib.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <direct.h>
#include <windows.h>

#define DEBUG

using namespace cv;
using namespace std;

void find_replace(char *str, char* orig, char *rep, char* output)
{
    char buffer[4096] = {0};
    char*p;
    sprintf(buffer, "%s", str);
    if(!(p=strstr(buffer, orig))){
	    sprintf(output, "%s", str);
	    return;
    }
    *p = '\0';
    sprintf(output, "%s%s%s", buffer, rep, p + strlen(orig));
}

void file_error(char *s)
{
	fprintf(stderr, "Couldn't open file: %s\n", s);
	exit(0);
}

int findInfrared(Mat image)
{
	int numThreshhold = image.rows*image.cols/5;
	int neualNum = 0;
	for(int j = 0; j < image.rows; j++)
	{
		for(int i = 0; i < image.cols; i++)
		{
			if((image.at<cv::Vec3d>(j, i)[0] != image.at<cv::Vec3d>(j, i)[1]) || (image.at<cv::Vec3d>(j, i)[0] != image.at<cv::Vec3d>(j, i)[2]) || (image.at<cv::Vec3d>(j, i)[1] != image.at<cv::Vec3d>(j, i)[2]))
			{
				nenualNum++;
			}
			if(neualNum > numThreshold)
			{
				return 0;
			}
		}
	}
	return 1;
}

int findInfraredInROI(mat image, int roi_w, int roi_h)
{
	HANDLE hdl = GetStdHandle(STD_OUTPUT_HANDLE);
	int w = image.cols;
	int h = image.rows;
	int c = image.channels();
	int pt_x, pt_y;
	unsigned char flag = 0, b;
	cv::Mat imgROI, yuvImg;
	cv::Mat img_y, img_u, img_v;
	int i,j;
	
	if((w > roi_w)&&(h > roi_h))
	{
		pt_x = (w - roi_w)/2;
		pt_y = (h - roi_h)/2;
	}
	else
	{
		printf("Image w=%d h=%d, ROI W and H set error!\n", w, h);
	}
	
	imgROI = image(Rect(pt_x, pt_y, roi_w, roi_h));
	
	bool condition1 = findInfrared(imgROI);
	
	cvtColor(imgROI, yuvImg, CV_BGR2YUV);
	
	split(yuvImg, yuv_vec);
	img_y = yuv_vec[0];
	img_u = yuv_vec[1];
	img_v = yuv_vec[2];
	
	Scalar avg2 = mean(img_u);
	Scalar avg3 = mean(img_v);
	
#ifdef DEBUG
	SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN);
	printf("avg_u=%f, avg_v=%f\n", avg2.val[0], avg3.val[0]);
	SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN);
	namedWindow("src", CV_WINDOW_NORMAL);
	imshow("src", image);
	imshow("u",img_u);
	
	if(flag==0)
		b = waitKey(0);
	else
		b = waitKey(1);
	
	if(b == 'b')
		flag = 0;
	else
		flag = 1;
#endif	
	
	if((avg2.val[0] == 128) && (avg3.val[0] == 128) && condition 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void get_filename(char *path, char *name)
{
	int i,j;
	for(i=0; path[i]; i++)
	{
#ifdef WIN32
		if(path[i] == '\\')
#else
		if(path[i] == '/')
#endif
			j = i+1;
	}
	strcpy(name, &path[j]);
}

int capture2Imgs(int argc, char **argv)
{
	printf("\nVideo path:\n");
	char getpath[300];
	char putpath[300];
	int numSpan =1;
	int show_flag = 0;
	
	scanf("%s", getpath);
	VideoCapture capture(getpath);
	cout << getpath << endl;
	
	string inPath(getpath);
	
	int npos1 = inPath.find_last_of("\\");
	int npos2 = inPath.find_last_of(".");
	
	string dirname = inPath.substr(0, npos2);
	_mkdir(dirname.c_str());
	string name = inPath.substr(npos1+1, npos2-4);
	string newname = name.substr(0, npos2 - npos1 - 1);
	cout << "name " << dirname << endl << newname << endl;
	printf("\nVideo frame interval:");
	scanf("%d", &numSpan);
	printf("\nThe second frame pos(2~%d):", numSpan);
	scanf("%d", &frame2pos);
	
	printf("\nShow image?1 for Y /0 for N");
	scanf("%d", &show_flag);
	cv::Mat image;
	int64 tic, toc;
	double time = 0;
	bool show_visualization = true;
	static int frameNum = 0;
	static int printNum = 0;
	
	int width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
	int height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	double fps = capture.get(CV_CAP_PROP_FPS);
	long int totalNumFrames = (long int)capture.get(CV_CAP_PROP_FRAME_COUNT);
	
	cout << "Resolution(width*height):" << width << "*" << height << endl;
	cout << "Total frames:" << totalNumFrames/numSpan * 2 << endl;
	cout << "fps:" << fps << endl;
	
	while(capture.isOpened())
	{
		capture >> image;
		frameNum ++;
		if(image.empty())
			break;
		if(((frameNum % numSpan) != 1) && ((frameNum % numSpan) != frame2pos))
			continue;
		
		printNum ++;
		
		cout << "frameIdx: " << printNum << endl;
		if(printNum % 2 == 1)
			sprintf(putpath, "%s\\%s_%06da.jpg", dirname.c_str(), newname.c_str(), (printNum + 1)/2);
		else
			sprintf(putpath, "%s\\%s_%06db.jpg", dirname.c_str(), newname.c_str(), (printNum + 1)/2);
		cout << putpath << endl;
		
		imwrite(string(putpath), image);
		int key;
		if(show_flag)
		{
			if(image.cols*image.rows >= 1080*720)
				resize(image, image, Size(), 0.5, 0.5);
			imshow("show", image);
			key = waitKey(2);
			if(key == 27)
			{
				destroyAllWindows();
				show_flag = 0;
			}
		}
	}
	return 0;
}
	
	
	
