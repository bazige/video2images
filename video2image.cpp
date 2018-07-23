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
