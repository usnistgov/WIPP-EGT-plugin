//
// Created by gerardin on 11/8/19.
//

#ifndef NEWEGT_OPENCV_CPP
#define NEWEGT_OPENCV_CPP

#include "gtest/gtest.h"
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/imgproc.hpp>

//TEST(OPENCV,BORDER_COPY) {
//
//}

using namespace std;
using namespace cv;

int main() {
    uint8_t data[] = {1,2,3,4,5,6,7,8,9};
    cv::Mat mat =  cv::Mat(3, 3, CV_8U, &data);
    cv::Mat dst = cv::Mat(4, 4, CV_8U);

    //a "sharpen" kernel
    int8_t k[] = {0,-1,0,-1,5,-1,0,-1,0};
    cv::Mat kernel = cv::Mat(3, 3, CV_8S, &k);

    cout << "M = "<< endl << " "  << mat << endl << endl;
    cout << "kernel" << kernel << endl;

    filter2D(mat, dst, CV_8U ,kernel, Point(0,0),0,  BORDER_REFLECT_101 );

    cout << "M = "<< endl << " "  << dst << endl << endl;

    cv::Mat output = cv::Mat(4, 4, CV_8U);

    copyMakeBorder(mat,output,2,2,2,2,BORDER_REFLECT_101);

    cout << "M = "<< endl << " "  << output << endl << endl;

    const int border = 2;
    int borderType = BORDER_REFLECT_101 | BORDER_ISOLATED;
    const cv::Scalar value(0, 0, 0);
    const cv::Rect roi(1,1, mat.rows - 1, mat.rows - 1);
    cv::copyMakeBorder(mat(roi), output, border, border, border, border, borderType);

    cout << "M = "<< endl << " "  << output << endl << endl;

    borderType = BORDER_REFLECT_101;
    cv::copyMakeBorder(mat(roi), output, border, border, border, border, borderType);
    cout << "M = "<< endl << " "  << output << endl << endl;

    borderType = BORDER_WRAP;
    cv::copyMakeBorder(mat(roi), output, border, border, border, border, borderType);
    cout << "M = "<< endl << " "  << output << endl << endl;

    borderType = BORDER_WRAP | BORDER_ISOLATED;
    cv::copyMakeBorder(mat(roi), output, border, border, border, border, borderType);
    cout << "M = "<< endl << " "  << output << endl << endl;

    //===============================
    //mat will be written of

    borderType = BORDER_WRAP | BORDER_ISOLATED;
    cout << "M - before = "<< endl << " "  << mat << endl << endl;
    cv::copyMakeBorder(mat(roi), mat, border, border, border, border, borderType);
    cout << "M - after = "<< endl << " "  << mat << endl << endl;

    //================================

    uint8_t data2[] = {1,2,3,4,5,6,7,8};
    cv::Mat mat2 =  cv::Mat(2, 4, CV_8U, &data2);
    cv::Mat output2;
    copyMakeBorder(mat2,output2,2,2,2,2,BORDER_REFLECT_101);

    cout << "M2 = "<< endl << " "  << output2 << endl << endl;

    copyMakeBorder(mat2,output2,2,2,2,2,BORDER_REFLECT);

    cout << "M2 = "<< endl << " "  << output2 << endl << endl;

    copyMakeBorder(mat2,output2,2,2,2,2,BORDER_WRAP);

    cout << "M2 = "<< endl << " "  << output2 << endl << endl;

    mat2.at<uint8_t>(0,0) = 2;

    cout << "Mat = "<< endl << " "  << mat2 << endl << endl;
    cout << "M2 = "<< endl << " "  << output2 << endl << endl;


    /**
     * MATRIX INITIALIZATION
     * If we store several channel of 8bits unsigned it is stored on disk a particular way (maybe as 32bits integer?)
     * In order to retrieve the value, we have to use Vec3b (b seems to stand for 'byte').
     */
    Mat M(3,3, CV_8UC3, Scalar(0,0,255));
    cout << "M = " << endl << " " << M << endl << endl;
    cout << "M(1,1) = " << M.at<Vec3b>(1,1)  << endl;
 cout << "M (numpy)   = " << endl << format(M, Formatter::FMT_NUMPY ) << endl << endl;
    cout << "M(1,1) (numpy)   = " << endl << format(M.at<Vec3b>(1,1), Formatter::FMT_NUMPY ) << endl << endl;


    /**
     * For performance reason, you
     */
    uint8_t* pixelPtr = (uint8_t*)M.data;
    int cn = M.channels();
    Scalar_<uint8_t> bgrPixel;

    for(int i = 0; i < M.rows; i++)
    {
        for(int j = 0; j < M.cols; j++)
        {
            bgrPixel.val[0] = pixelPtr[i*M.cols*cn + j*cn + 0]; // B
            bgrPixel.val[1] = pixelPtr[i*M.cols*cn + j*cn + 1]; // G
            bgrPixel.val[2] = pixelPtr[i*M.cols*cn + j*cn + 2]; // R

            cout << bgrPixel << endl;
        }
    }


}

#endif //NEWEGT_OPENCV_CPP
