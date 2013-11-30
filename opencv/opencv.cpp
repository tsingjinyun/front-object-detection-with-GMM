#include "StdAfx.h"
#include<stdio.h>
#include<cv.h>
#include<cxcore.h>
#include<highgui.h>
#include<cvaux.h>
/*
首先初始化预先定义的几个高斯模型，对高斯模型中的参数进行初始化，并求出之后将要用到的参数；
其次，对于每一帧中的每一个像素进行处理，看其是否匹配某个模型，若匹配，则将其归入该模型中，并对该模型根据新的像素值进行更新，
														   若不匹配，则以该像素建立一个高斯模型，初始化参数，代替原有模型中最不可能的模型。
最后选择前面几个最有可能的模型作为背景模型，为背景目标提取做铺垫
*/
typedef struct MyCvGaussBGValues
{
    float match_sum;
    float weight;
    float mean[3];
    float variance[3];
}
MyCvGaussBGValues;

static void updateBackground(CvGaussBGModel* bg_model)
{
    int K = bg_model->params.n_gauss;
    int nchannels = bg_model->background->nChannels;
    int height = bg_model->background->height;
    int width = bg_model->background->width;
    MyCvGaussBGValues *g_point = (MyCvGaussBGValues *) ((CvMat*)(bg_model->g_point))->data.ptr;
    MyCvGaussBGValues *mptr = g_point;

    for(int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++, mptr+=K)
        {
						int pos = bg_model->background->widthStep*y + x*nchannels;
            float mean[3] = {0.0, 0.0, 0.0};

            for(int k=0; k<K; k++)
            {
                for(int m=0; m<nchannels; m++)
                {
                    mean[m] += mptr[k].weight * mptr[k].mean[m];
                }
            }

            for(int m=0; m<nchannels; m++)
            {
                bg_model->background->imageData[pos+m] = (uchar) (mean[m]+0.5);
            }
        }
    }
}
int main(int argc,char **argv)
{
	IplImage *pFrame=NULL;
	IplImage *pFrImg=NULL;
	IplImage *pBkImg=NULL;
	CvCapture *pCapture=NULL;
	CvBGStatModel *bg_model=NULL;  //初始化高斯混合模型参数
	int nFrmNum=0;
	//int region_count=0;
	
	cvNamedWindow("video",1);
	cvNamedWindow("background",1);
	cvNamedWindow("foreground",1);
	cvMoveWindow("video",30,0);
	cvMoveWindow("background",450,0);
	cvMoveWindow("foreground",900,0);
	//pCapture=cvCaptureFromFile(argv[1]);
	if( argc > 2 )  
	{
	fprintf(stderr, "Usage: bkgrd [video_file_name]/n");
	return -1;
	}
	//打开视频文件
	if(argc == 2)
	if( !(pCapture = cvCaptureFromFile(argv[1]))) 
	{
		fprintf(stderr, "Can not open video file %s/n", argv[1]);
		return -2;
	}
	if (argc == 1)
	if( !(pCapture = cvCaptureFromCAM(-1)))
	{
		fprintf(stderr, "Can not open camera./n");
		return -2;}  
	while(pFrame=cvQueryFrame(pCapture))
	{
		nFrmNum++;
		if(nFrmNum==1)
			{
				 //高斯背景建模，pFrame可以是多通道图像也可以是单通道图像
				pBkImg=cvCreateImage(cvGetSize(pFrame),IPL_DEPTH_8U,3);
				pFrImg=cvCreateImage(cvGetSize(pFrame),IPL_DEPTH_8U,1);
				//cvCreateGaussianBGModel函数返回值为CvBGStatModel*，
				//需要强制转换成CvGaussBGModel*
				bg_model=(CvBGStatModel*)cvCreateGaussianBGModel(pFrame,0);
			}
		else
			{
				//更新高斯模型
				cvUpdateBGStatModel(pFrame,(CvBGStatModel*)bg_model);
				updateBackground((CvGaussBGModel*)bg_model);
				cvClearMemStorage(bg_model->storage);
	    		 //pFrImg为前景图像，只能为单通道
				cvCopy(bg_model->foreground,pFrImg,0);
				//pBkImg为背景图像，可以为单通道或与pFrame通道数相同
				cvCopy(bg_model->background,pBkImg,0);
			}
		cvShowImage("video",pFrame);
		cvShowImage("background",pBkImg);
		cvShowImage("foreground",pFrImg);
		if(cvWaitKey(22)>=0)
			break;
	}
  
	cvReleaseBGStatModel((CvBGStatModel**)&bg_model);
	cvDestroyAllWindows();
	cvReleaseImage(&pFrImg);
	cvReleaseImage(&pBkImg);
	cvReleaseCapture(&pCapture);
	return 0;
}
