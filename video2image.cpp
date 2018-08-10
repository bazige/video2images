#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "opencv2/contrib/contrib.hpp"
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
	int numThreshold = image.rows*image.cols/5;
	int neualNum = 0;
	for(int j = 0; j < image.rows; j++)
	{
		for(int i = 0; i < image.cols; i++)
		{
			if((image.at<cv::Vec3b>(j, i)[0] != image.at<cv::Vec3b>(j, i)[1]) || (image.at<cv::Vec3b>(j, i)[0] != image.at<cv::Vec3b>(j, i)[2]) || (image.at<cv::Vec3b>(j, i)[1] != image.at<cv::Vec3b>(j, i)[2]))
			{
				neualNum++;
			}
			if(neualNum > numThreshold)
			{
				return 0;
			}
		}
	}
	return 1;
}

int findInfraredInROI(Mat image, int roi_w, int roi_h)
{
	HANDLE hdl = GetStdHandle(STD_OUTPUT_HANDLE);
	int w = image.cols;
	int h = image.rows;
	int c = image.channels();
	int pt_x, pt_y;
	unsigned char flag = 0, b;
	cv::Mat imgROI, yuvImg;
	cv::Mat img_y, img_u, img_v;
	vector<cv::Mat> yuv_vec;
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
	
	if((avg2.val[0] == 128) && (avg3.val[0] == 128) && condition1)
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
	int i,j=0;
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
	int numSpan = 1;
	int frame2pos = 1;
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

int captureImg(char *getpath, int numSpan, int show_flag)
{
	char putpath[300];

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
		if((frameNum % numSpan) != 1)
			continue;
		
		printNum ++;
		
		cout << "frameIdx: " << printNum << endl;
		sprintf(putpath, "%s\\%s_%06da.jpg", dirname.c_str(), newname.c_str(), printNum);
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


int captureImage(int argc, char **argv)
{
	FILE *fpList = NULL;
	printf("\nVideo or Image path List:\n");
	char getpath[300];
	char putpath[300];
	char strLine[1024];
	int numSpan =1;
	int show_flag = 0;
	int num = 0;
	
	scanf("%s", getpath);
	if(strstr(getpath,".dav")||strstr(getpath,".avi"))
	{
		printf("\nVideo interval:");
		scanf("%d", &numSpan);
		
		printf("\nShow image?1 for Y /0 for N");
		scanf("%d", &show_flag);
		captureImg(getpath, numSpan, show_flag);
	}
	else if(strstr(getpath,".txt")||strstr(getpath,".list"))
	{
		printf("\nVideo frame interval:");
		scanf("%d", &numSpan);
		
		printf("\nShow image?1 for Y /0 for N");
		scanf("%d", &show_flag);
		
		if((fpList = fopen(getpath, "r")) == NULL)
		{
			printf("readTestList:error\n");
			fclose(fpList);
			return -1;
		}
		while(!feof(fpList))
		{
			fgets(strLine, 1024, fpList);
			if(strLine[strlen(strLine) - 1] == '\n')
				strLine[strlen(strLine) - 1] = '\0';
			
			captureImg(strLine, numSpan, show_flag);
		}
	}
	else
	{
		printf("Input Video or Video Path error!\n");
		return -1;
	}
	return 0;
}

int captureInfrared_rgb(int argc, char **argv)
{
	HANDLE hdl = GetStdHandle(STD_OUTPUT_HANDLE);
	FILE *fp = NULL;
	fp = fopen("d:\\infrared.txt", "w");
	printf("\nVideo Path List:");
	cv::Mat image;
	FILE *fpList = NULL;
	char strLine[1024];
	char getpath[300];
	char putpath[300];
	int numSpan = 1;
	int show_flag = 0;
	int roi_w = 300;
	int roi_h = 200;
	scanf("%s", getpath);
	if((fpList = fopen(getpath, "r")) == NULL)
	{
		printf("readTestList:error\n");
		fclose(fpList);
		return -1;
	}
	int num = 0;
	while(!feof(fpList))
	{
		num += 1;
		cout << "num" << num << endl << endl;
		fgets(strLine, 1024, fpList);			
		if(strLine[strlen(strLine) - 1] == '\n')
				strLine[strlen(strLine) - 1] = '\0';
		
		string inPath(strLine);
		int npos1 = inPath.find_last_of("\\");
		string dirname = inPath.substr(0, npos1);
		string dirnameC = dirname;
		string vname = inPath.substr(npos1, string::npos);
		string infrDir = dirname.append("\\Infrared");
		string otherDir = dirnameC.append("\\Other");
		_mkdir(infrDir.c_str());
		_mkdir(otherDir.c_str());
		
		VideoCapture capture(strLine);
		if(capture.isOpened())
		{
			capture >> image;
			if(findInfrared(image))
			{
				string dstName = infrDir.append(vname);
				printf("videoName:%s,",strLine);
				SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_INTENSITY);
				printf("Infrared\n", strLine);
				SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
				fprintf(fp, "%s\n", strLine);
			}
			else
			{
				string dstName = otherDir.append(vname);
				printf("videoName:%s,",strLine);
				SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_INTENSITY);
				printf("Color\n", strLine);
				SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
			}
		}
		else
		{
			cout << "cannot open" << inPath << endl;
		}
	}
	fclose(fp);
	return 0;
}
			
int captureInfrared_yuv(int argc, char **argv)
{
	HANDLE hdl = GetStdHandle(STD_OUTPUT_HANDLE);
	FILE *fp = NULL;
	fp = fopen("d:\\infrared.txt", "w");
	printf("\nVideo Path List:");
	cv::Mat image;
	FILE *fpList = NULL;
	char strLine[1024];
	char getpath[300];
	char putpath[300];

	int numSpan = 75;
	int show_flag = 0;
	int roi_w = 300;
	int roi_h = 200;
	int frameNum = 0;
	bool grayVideoFlg = true;
	
	scanf("%s", getpath);
	if((fpList = fopen(getpath, "r")) == NULL)
	{
		printf("readTestList:error\n");
		fclose(fpList);
		return -1;
	}
	int num = 0;
	while(!feof(fpList))
	{
		num += 1;
		cout << "\nvideo:" << num << endl << endl;
		fgets(strLine, 1024, fpList);			
		if(strLine[strlen(strLine) - 1] == '\n')
			strLine[strlen(strLine) - 1] = '\0';
		
		string inPath(strLine);
		int npos1 = inPath.find_last_of("\\");
		string dirname = inPath.substr(0, npos1);
		string dirnameC = dirname;
		string vname = inPath.substr(npos1, string::npos);
		string infrDir = dirname.append("\\Infrared");
		string otherDir = dirnameC.append("\\Other");
#ifndef DEBUG		
		_mkdir(infrDir.c_str());
		_mkdir(otherDir.c_str());
#endif
		VideoCapture capture(strLine);
		
		double fps = capture.get(CV_CAP_PROP_FPS);
		if(fps != 0)
			numSpan = fps*3;
		else
			numSpan = 75;
#ifdef DEBUG
		if(capture.isOpened())
		{
			frameNum ++;
			capture >> image;
			if(findInfraredInROI(image, roi_w, roi_h) == 0)
			{
				grayVideoFlg = false;
			}
			else
			{
				grayVideoFlg =true;
			}
		}
#else	
		while(capture.isOpened())
		{
			frameNum ++;
			capture >> image;
			if((frameNum%numSpan) != 1)
				continue;
			
			if(findInfraredInROI(image, roi_w, roi_h) == 0)
			{
				grayVideoFlg = false;
				break;
			}
			grayVideoFlg =true;
		}
#endif			
		if(grayVideoFlg)
		{
			string dstName = infrDir.append(vname);
			printf("videoName:%s,",strLine);
			SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_INTENSITY);
			printf("Infrared\n");
			SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
			fprintf(fp, "%s\n", strLine);
		}
		else
		{
			string dstName = otherDir.append(vname);
			printf("videoName:%s,",strLine);
			SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_INTENSITY);
			printf("Color\n");
			SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
		}
	}
	fclose(fp);
	return 0;
}

			
typedef struct{
	int type;
	float ptx;
	float pty;
	float sx;
	float sy;
}det_box[40];
			
void drawBox(int argc, char **argv)
{
	#define BUFLEN 1024
	HANDLE hdl = GetStdHandle(STD_OUTPUT_HANDLE);
	FILE *fp, *ft;
	int i;
	char b, *p;
	char strLine[1024];
	char fpath[500];
	char pathName[1024];
	char newDirName[1024];
	char newImgName[1024];
	char newImg[1024];
	char buff[BUFLEN];
	
	IplImage *showimage;
	int flag = 0;
	int view_mode;
	det_box box;
	
	SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
	printf("YoloTxt label:\n");
	printf("   [class]  [box_center_x]  [box_center_y]  [box_w]  [box_h]\n\n");
	SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
	
	printf("Please Enter Image List Path:\n");
	scanf("%s", fpath);
	if((fp = fopen(fpath, "r")) == NULL)
	{
		printf("%s\n", fpath);
		printf("Image List Error!\n");
		fclose(fp);
		return ;
	}
	
	fflush(stdin);
	printf("Show box or Save:0->show 1->save\n");
	scanf("%d", &view_mode);
	while((view_mode != 0)&&(view_mode != 1))
	{
		printf("Please Enter 0 or 1!\n");
		scanf("%d", &view_mode);
	}
	
	while(!feof(fp))
	{
		fgets(strLine, 1024, fp);			
		if(strLine[strlen(strLine) - 1] == '\n')
			strLine[strlen(strLine) - 1] = '\0';
		printf("%s\n", strLine);
		
		showimage = cvLoadImage(strLine, 1);
		int w = showimage->width;
		int h = showimage->height;
		
		p = strrchr(strLine, '\\');
		p = p + 1;
		
		if(view_mode == 1)
		{
			sprintf(newDirName, "%s\\draw_label\\", "Y:\\face_person\\FocusData");
			mkdir(newDirName);
			sprintf(newImg, "%s%s", newDirName, p);
		}
		
		int nboxes = 0;
		char labelpath[4096];
		find_replace(strLine, "images", "labels", labelpath);
		find_replace(labelpath, "JPEGImages", "labels", labelpath);
		find_replace(labelpath, ".jpg", ".txt", labelpath);
		find_replace(labelpath, ".JPEG", ".txt", labelpath);
		find_replace(labelpath, ".png", ".txt", labelpath);
		
		if((ft = fopen(labelpath, "r"))==NULL)
		{
			printf("LabelPath is error!\n");	
			return;
		}
		
		while(fgets(buff, BUFLEN, ft))
		{
			int j = 0;
			char *delim = " ";
			char *p = strtok(buff, delim);
			while(p!=NULL)
			{
				if(j%5 == 0)
					box[nboxes].type = atoi(p);
				else if(j%5 == 1)
					box[nboxes].ptx = atof(p);
				else if(j%5 == 2)
					box[nboxes].pty = atof(p);
				else if(j%5 == 3)
					box[nboxes].sx = atof(p);
				else
					box[nboxes].sy = atof(p);
				
				p = strtok(NULL, delim);
				j ++;
			}	
			nboxes ++;
		}
			  
		for(i = 0; i < nboxes; ++i)
		{
			CvPoint p0, p1;
			p0.x = w*(box[i].ptx - box[i].sx/2);
			p0.y = h*(box[i].pty - box[i].sy/2);
			p1.x = w*(box[i].ptx + box[i].sx/2);
			p1.y = h*(box[i].pty + box[i].sy/2);
			
			if(box[i].type == 0)
				cvRectangle(showimage, p0, p1, CV_RGB(255, 0, 0), 2, 8, 0);
			else if(box[i].type == 1)
				cvRectangle(showimage, p0, p1, CV_RGB(0, 0, 255), 2, 8, 0);
			else
				cvRectangle(showimage, p0, p1, CV_RGB(0, 128, 128), 2, 8, 0);
		}
		
		if(view_mode == 1)
		{
			cvSaveImage(newImg, showimage);
		}
		else
		{		  
			cvNamedWindow("drawBox", CV_WINDOW_AUTOSIZE);
			cvShowImage("drawBox", showimage);

			if(flag==0)
				b = cvWaitKey(0);
			else
				b = cvWaitKey(1);

			if(b == 'b')
				flag = 1;
			if(b == 'c')
				flag = 0;
			if(b == 'q')
				break;
		
			cvDestroyAllWindows();
		}
		fclose(ft);
	}
	fclose(fp);
}
			
			
void showImage(int argc, char **argv)
{
	HANDLE hdl = GetStdHandle(STD_OUTPUT_HANDLE);
	FILE *fp;
	char b;
	char fpath[1024];
	char strLine[1024];
	char replacePath[1024];
	
	cv::Mat showimage;
	int flag = 0;
	int colorSpace;
	int selSpace;
	SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
	printf("\n Note:\n   Showing image include: .jpg(JPG) .png(PNG) .bmp(BMP) .yuv(YUV)\n");
	printf("   YUV image should convert to BGR\n\n");
	SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);

	fflush(stdin);
	printf("Please Enter Image List Path:\n");
	scanf("%s", fpath);
	if((fp = fopen(fpath, "r")) == NULL)
	{
		printf("%s\n", fpath);
		printf("Image List Error!\n");
		fclose(fp);
		return ;
	}
	
	fflush(stdin);
	printf("Please select ColorSpace:(not work for RGB image)\n    0->YUV2BGR, 1->NV21, 2->NV12, 3->UYVY, 4->YVYU, 5->YUYV\n");
	scanf("%d", colorSpace);
	switch(colorSpace)
	{
		case 0: selSpace = COLOR_YUV2BGR; break;
		case 1: selSpace = COLOR_YUV2BGR_NV21; break;
		case 2: selSpace = COLOR_YUV2BGR_NV12; break;
		case 3: selSpace = COLOR_YUV2BGR_UYVY; break;
		case 4: selSpace = COLOR_YUV2BGR_YVYU; break;
		case 5: selSpace = COLOR_YUV2BGR_YUYV; break;
		default: selSpace = COLOR_YUV2BGR_UYVY; break;
	}
	
	while(!feof(fp))
	{
		fgets(strLine, 1024, fp);			
		if(strLine[strlen(strLine) - 1] == '\n')
			strLine[strlen(strLine) - 1] = '\0';
		printf("%s\n", strLine);
		
		if(strstr(strLine, ".jpg")||strstr(strLine, ".JPG")||strstr(strLine, ".bmp")||strstr(strLine, ".png"))
		{
			showimage = imread(strLine, 1);
			int w = showimage.size().width;
			int h = showimage.size().height;
		
			if(w*h >= 1080*720)
				resize(showimage, showimage, Size(), 0.5, 0.5);
		
			imshow("drawBox", showimage);
		}
		else if(strstr(strLine, ".yuv")||strstr(strLine, ".YUV"))
		{
			int w = 2560;
			int h = 1440;
			int c = 3;
			cv::Mat yuvImg;
			cv::Mat rgbImg;
			int frame_size = w*h*c/2;
			FILE* fp;
			if(!(fp = fopen(strLine, "rb+")))
			{
				printf("YUV file open error!\n");
			}
			
			unsigned char* pYuvBuf = new unsigned char[frame_size];
			fread(pYuvBuf, frame_size*sizeof(unsigned char), 1, fp);
			
			yuvImg.create(h*3/2, w, CV_8UC1);
			memcpy(yuvImg.data, pYuvBuf, frame_size*sizeof(unsigned char));
			cv::cvtColor(yuvImg, rgbImg, selSpace);
			
			//imshow("yuv2rgb", rgbImg);
			find_replace(strLine, ".yuv", ".jpg", replacePath);
			imwrite(replacePath, rgbImg);
		}

		if(flag==0)
			b = cvWaitKey(0);
		else
			b = cvWaitKey(1);

		if(b == 'b')
			flag = 0;
		else
			flag = 1;
		
	}
	fclose(fp);
}			
			  
void imageResize(int argc, char **argv)
{
	HANDLE hdl = GetStdHandle(STD_OUTPUT_HANDLE);
	FILE *fp;
	Mat srcImg, dstImg;
	float w, h, w_new, h_new;
	float arg1, arg2, r;
	float ratio;
	char mode;
	char name[100];
	char savePath[100];
	char fpath[1024];
	char strLine[1024];
	char tempPath[100];
	char imgCover=NULL;
	
	printf("Please Enter Image List Path:\n");
	scanf("%s", fpath);
	if((fp = fopen(fpath, "r")) == NULL)
	{
		printf("Image List Error!\n");
		fclose(fp);
		return;
	}
	
	fflush(stdin);
	SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
	printf("Please Enter resized width or ratio:\n");
	printf("    1.fixed size: eg. w 540\n    2.fixed ratio: eg. r 0.5\n\n");
	SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
	scanf("%c %f", &mode, &arg1);
	if(mode == 'w')
	{
		w_new = arg1;
		ratio = 0.0;
	}
	else if(mode == 'r')
	{
		w_new = 0.0;
		ratio = arg1;
	}
	else
	{
		printf("Resize mode set error!\n");
		return;
	}		
	
	fflush(stdin);
	printf("Cover Old Image File? Y/N\n");
	scanf("%c", &imgCover);
	if(imgCover == 'N' || imgCover == 'n')
	{
		fflush(stdin);
		printf("Please Enter the Image Save Path:\n");
		scanf("%s", savePath);
	}
	
	while(!feof(fp))
	{
		fgets(strLine, 1024, fp);			
		if(strLine[strlen(strLine) - 1] == '\n')
			strLine[strlen(strLine) - 1] = '\0';
		printf("%s\n", strLine);
		
		srcImg = imread(strLine);
		w = srcImg.size().width;
		h = srcImg.size().height;
		
		if(mode == 'w')
		{
			r = h/w;
			h_new = floor(r*w_new);
			if(((int)h_new%2) != 0)
				h_new = h_new + 1;
			if(h_new > h)
				h_new = h;
		}
		else
		{
			w_new = floor(w*ratio);
			h_new = floor(h*ratio);
			if(((int)w_new%2) != 0)
				w_new = w_new + 1;
			if(((int)h_new%2) != 0)
				h_new = h_new + 1;
			if(w_new>w)
				w_new = w;
			if(h_new>h)
				h_new = h;
		}
		
		resize(srcImg, dstImg, Size(w_new, h_new), 0, 0, INTER_LINEAR);
		
		get_filename(strLine, name);
		
		if(imgCover == 'N' || imgCover == 'n')
		{
			strcpy(tempPath, savePath);
			strcat(tempPath, name);
			printf("tempPath=%s\n", tempPath);
			imwrite(tempPath, dstImg);
		}
		else
			imwrite(strLine, dstImg);
		
#if 0
		imshow("Resized Image", dstImg);
		waitKey();
#endif
	}
	fclose(fp);
}

void imageRoiCut_labelCorrect(int argc, char **argv)
{
//#define SHOW_IMG
#define SAVE_OPT_DIR
	
	#define BUFF_LEN 500
	HANDLE hdl = GetStdHandle(STD_OUTPUT_HANDLE);
	FILE *fp, *ft, *ft_n;
	Mat srcImg, dstImg;
	Mat show_img, dst_show;
	int w, h, w_new, stride;
	int ROI_startx, ROI_endx;
	int i,k,num;
	char buff[BUFF_LEN];
	det_box box;
	
	float wh_thresh = 1.0/8;
	
	char *p, *pp;
	char fpath[500];
	char name[500];
	char strLine[500];
	char dirName[500];
	char filename[500];
	char labelfilename[500];
	char labelpath[500];
	memset(buff, 0, BUFF_LEN);
	memset(fpath, 0, 500);
	memset(strLine, 0, 500);
	memset(labelpath, 0, 500);
	SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
	printf("\n Note:\n   Copy image's ROI zone and Save\n");
	printf("   ROI's height is equal to image's height, while its width is overlapped.\n\n");
	SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
	
	printf("Please Enter the image List Path:\n");
	scanf("%s", fpath);
	if((fp == fopen(fpath, "r"))== NULL)
	{
		printf("%s\n", fpath);
		printf("Image List Error!\n");
		fclose(fp);
		return;
	}
	fflush(stdin);
	printf("Please input the cut ROI image nums:\n");
	scanf("%d", &num);
	
	while(!feof(fp))
	{
		fgets(strLine, 1024, fp);
		if(strLine[strlen(strLine)-1] == '\n')
			strLine[strlen(strLine) - 1] = '\0';
		printf("%s\n", strLine);
		
		srcImg = imread(strLine);
		w = srcImg.size().width;
		h = srcImg.size().height;
		
		w_new = w / num;
		stride = w_new / 5;
		
		//label correct
		int nboxes = 0;
		find_replace(strLine, "images", "labels", labelpath);
		find_replace(labelpath, "JPEGImages", "labels", labelpath);
		find_replace(labelpath, ".jpg", ".txt", labelpath);
		find_replace(labelpath, ".JPEG", ".txt", labelpath);
		find_replace(labelpath, ".png", ".txt", labelpath);
		
		if((ft = fopen(labelpath, "r"))==NULL)
		{
			SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_INTENSITY);
			printf("Label open ERROR, label doesn't match with image!\n");
			SetConsoleTextAttribute(hdl, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
			return;
		}
		
		while(fgets(buff, BUFF_LEN, ft))
		{
			int j=0;
			char *delim = " ";
			char *p = strtok(buff, delim);
			while(p!= NULL)
			{
				if(j%5==0)
					box[nboxes].type = atoi(p);
				else if(j%5==1)
					box[nboxes].ptx = atof(p);
				else if(j%5==2)
					box[nboxes].pty = atof(p);
				else if(j%5==3)
					box[nboxes].sx = atof(p);
				else
					box[nboxes].sy = atof(p);
				
				p = strtok(NULL, delim);
				j++;
			}
			nboxes++;
		}
		fclose(ft);
		
		//roi image start and end point caculate
		for(i=0; i<num; i++)
		{
			if(i == 0)
			{
				ROI_startx = 0;
				ROI_endx = w_new + stride;
			}
			else if(i==num-1)
			{
				ROI_startx = w_new*i - stride;
				ROI_endx = w;
			}
			else
			{
				ROI_startx = w_new*i - stride/2;
				ROI_endx = w_new*(i + 1) + stride/2;
			}
			
			Rect rect(ROI_startx, 0, ROI_endx - ROI_startx, h);
			dstImg = srcImg(rect);
			
			memset(name, 0, 500);
			memset(dirName, 0, 500);
			memset(filename, 0, 500);
			
			p = strrchr(strLine, '\\');
			memcpy(dirName, strLine, strlen(strLine)-strlen(p));
			sprintf(dirName, "%s\\roi_cut\\", dirName);
#ifndef SAVE_OPT_DIR
			mkdir(dirName);
#endif
			pp = strrchr(strLine, '.');
			
			memcpy(name, p + 1, strlen(p)-strlen(pp) - 1);
#ifndef SAVE_OPT_DIR
			sprintf(filename, "%s%s_%d.jpg", dirName, name, i);
			printf("filename=%s\n", filename);
			imwrite(filename, dstImg);
#else		
			char savePath[100] = "Y:\\face_person\\FocusData\\train_data_ROI\\img\\";
			sprintf(filename, "%s%s_%d.jpg", savePath, name, i);
			imwrite(filename, dstImg);
#endif
			
#ifdef SHOW_IMG
			srcImg.copyTo(show_img);
			dstImg.copyTo(dst_show);
			
			for(k=0; k<nboxes; k++)
			{
				CvPoint p0, p1;
				p0.x = w*(box[k].ptx - box[k].sx/2);
				p0.y = h*(box[k].pty - box[k].sy/2);
				p1.x = w*(box[k].ptx + box[k].sx/2);
				p1.y = h*(box[k].pty + box[k].sy/2);
				
				if(box[k].type == 0)
					cv::rectangle(show_img, p0, p1, CV_RGB(255, 0, 0), 2, 8, 0);
				else if(box[k].type == 1)
					cv::rectangle(show_img, p0, p1, CV_RGB(0, 0, 255), 2, 8, 0);
				else
					cv::rectangle(show_img, p0, p1, CV_RGB(0, 128, 128), 2, 8, 0);
			}
			
			cv::rectangle(show_img, rect, CV_RGB(255, 0, 0), 2, 8, 0);
			imshow("Orig", show_img);
#endif
			//select label box according to sub-ROIimage
			sprintf(labelfilename, "%s%s_%d.txt", savePath, name, i);
			if((ft_n = fopen(labelfilename, "w"))==NULL)
			{
				printf("Save Path of label and image is not correct!\n");
				return;
			}
			
			for(k=0; k<nboxes; k++)
			{
				//label boxes' coordinates refer to original Image
				int box_left = (box[k].ptx - box[k].sx/2.0)*w;
				int box_right = (box[k].ptx + box[k].sx/2.0)*w;
				int box_w = box[k].sx*w;
				int box_h = box[k].sy*w;
				if(box_left < 0) box_left = 0;
				if(box_right > w - 1) box_right = w - 1;
				float subBox_ptx, subBox_sx;
				int subBox_w;
				
				//label box in sub-image(ROI-Image)
				if((box_left >= ROI_startx)&&(box_right <= ROI_endx))
				{
					subBox_ptx = (float)(box_left - ROI_startx + box_w/2.0)/(ROI_endx - ROI_startx);
					subBox_sx = (float)box_w/(ROI_endx - ROI_startx);
					
					fprintf(ft_n, "%d %f %f %f %f\n", box[k].type, subBox_ptx, box[k].pty, subBox_sx, box[k].sy);
#ifdef SHOW_IMG					
					CvPoint pb0, pb1;
					pb0.x = (subBox_ptx - subBox_sx/2)*(ROI_endx - ROI_startx);
					pb0.y = h*(box[k].pty - box[k].sy/2);
					pb1.x = (subBox_ptx + subBox_sx/2)*(ROI_endx - ROI_startx);
					pb1.y = h*(box[k].pty + box[k].sy/2);

					if(box[k].type == 0)
						cv::rectangle(dst_show, pb0, pb1, CV_RGB(255, 0, 0), 2, 8, 0);
					else if(box[k].type == 1)
						cv::rectangle(dst_show, pb0, pb1, CV_RGB(0, 0, 255), 2, 8, 0);
					else
						cv::rectangle(dst_show, pb0, pb1, CV_RGB(0, 128, 128), 2, 8, 0);
#endif					
				}
				//label box expand to other sub-image
				else if(((box_left < ROI_startx)&&(box_right >= ROI_startx))||((box_left <= ROI_endx)&&(box_right > ROI_endx)))
				{
					if((box_left < ROI_startx)&&(box_right >= ROI_startx))
					{
						subBox_w = box_right - ROI_startx;
						if((float)subBox_w/box_h >= wh_thresh)
						{
							subBox_ptx = (float)(subBox_w/2)/(ROI_endx - ROI_startx);
							subBox_sx = (float)subBox_w/(ROI_endx - ROI_startx);
							
							fprintf(ft_n, "%d %f %f %f %f\n", box[k].type, subBox_ptx, box[k].pty, subBox_sx, box[k].sy);
#ifdef SHOW_IMG					
							CvPoint pb0, pb1;
							pb0.x = (subBox_ptx - subBox_sx/2)*(ROI_endx - ROI_startx);
							pb0.y = h*(box[k].pty - box[k].sy/2);
							pb1.x = (subBox_ptx + subBox_sx/2)*(ROI_endx - ROI_startx);
							pb1.y = h*(box[k].pty + box[k].sy/2);

							if(box[k].type == 0)
								cv::rectangle(dst_show, pb0, pb1, CV_RGB(255, 0, 0), 2, 8, 0);
							else if(box[k].type == 1)
								cv::rectangle(dst_show, pb0, pb1, CV_RGB(0, 0, 255), 2, 8, 0);
							else
								cv::rectangle(dst_show, pb0, pb1, CV_RGB(0, 128, 128), 2, 8, 0);
#endif	
						}
					}
					if((box_left <= ROI_endx)&&(box_right > ROI_endx))
					{
						subBox_w = ROI_endx - box_left;
						if((float)subBox_w/box_h >= wh_thresh)
						{
							subBox_ptx = (float)(ROI_endx - subBox_w/2.0)/(ROI_endx - ROI_startx);
							subBox_sx = (float)subBox_w/(ROI_endx - ROI_startx);
							
							fprintf(ft_n, "%d %f %f %f %f\n", box[k].type, subBox_ptx, box[k].pty, subBox_sx, box[k].sy);
#ifdef SHOW_IMG					
							CvPoint pb0, pb1;
							pb0.x = (subBox_ptx - subBox_sx/2)*(ROI_endx - ROI_startx);
							pb0.y = h*(box[k].pty - box[k].sy/2);
							pb1.x = (subBox_ptx + subBox_sx/2)*(ROI_endx - ROI_startx);
							pb1.y = h*(box[k].pty + box[k].sy/2);

							if(box[k].type == 0)
								cv::rectangle(dst_show, pb0, pb1, CV_RGB(255, 0, 0), 2, 8, 0);
							else if(box[k].type == 1)
								cv::rectangle(dst_show, pb0, pb1, CV_RGB(0, 0, 255), 2, 8, 0);
							else
								cv::rectangle(dst_show, pb0, pb1, CV_RGB(0, 128, 128), 2, 8, 0);
#endif	
						}
					}
				}
			}
			fclose(ft_n);
#ifdef SHOW_IMG
			imshow("ROI", dst_show);
			waitKey();
#endif			
		}
	}
	fclose(fp);
}

int main(int argc, char **argv)
{
	printf("0: video -> image\n");
	printf("1: Extract 2 continuous-frames from video\n");
	printf("2: Infrared Video distinguish(RGB)\n");
	printf("3: Infrared Video distinguish(YUV)\n");	
	printf("4: Draw box in label image\n");
	printf("5: Preview image\n");
	printf("6: Image Resize(Letter box)\n");
	printf("7: Image ROI cut, and it's label will be modified.\n");
	printf("Please Select tool's index: 0 or 1 or 2 or 3 or 4 or 5 or 6 or 7:");
	int model = 0;
	scanf("%d", &model);
	if(model == 1)
		capture2Imgs(argc, argv);
	else if(model == 2)
		captureInfrared_rgb(argc, argv);
	else if(model == 3)
		captureInfrared_yuv(argc, argv);
	else if(model == 4)
		drawBox(argc, argv);
	else if(model == 5)
		showImage(argc, argv);
	else if(model == 6)
		imageResize(argc, argv);
	else if(model == 7)
		imageRoiCut_labelCorrect(argc, argv);
	else
		captureImage(argc, argv);
	return 0;
}
