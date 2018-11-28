/*
 * ImagePreprocessor.h
 *
 *  Created on: Nov 28, 2018
 *      Author: sujiwo
 */

#ifndef VMML_SRC_IMAGEPREPROCESSOR_H_
#define VMML_SRC_IMAGEPREPROCESSOR_H_

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>


class ImagePreprocessor
{
public:

	enum ProcessMode {
		AS_IS = 0,
		AGC = 1,
		ILLUMINATI = 2
	};

	ImagePreprocessor(ProcessMode m);
	virtual ~ImagePreprocessor();

	cv::Mat preprocess(const cv::Mat &src);
	void preprocess(cv::Mat &srcInplace);

	void setMask (cv::Mat &maskSrc);
	inline void setIAlpha (const float &a)
	{ iAlpha = a; }

	static float detectSmear (cv::Mat &rgbImage, const float tolerance);
	static cv::Mat cdf (cv::Mat &grayImage, cv::Mat mask=cv::Mat());
	static cv::Mat setGamma (cv::Mat &grayImage, const float gamma, bool LUT_only=false);
	static cv::Mat autoAdjustGammaRGB (cv::Mat &rgbImage, cv::Mat mask=cv::Mat());
	static cv::Mat autoAdjustGammaRGB (cv::Mat &rgbImage, std::vector<cv::Mat> &rgbBuf, cv::Mat mask=cv::Mat());
	static cv::Mat autoAdjustGammaMono (cv::Mat &grayImage, float *gamma=NULL, cv::Mat mask=cv::Mat());
	static cv::Mat toIlluminatiInvariant (const cv::Mat &rgbImage, const float alpha);

protected:

	ProcessMode pMode;
	cv::Mat mask;

	float iAlpha;
};


cv::Mat histogram (cv::Mat &inputMono, cv::Mat mask=cv::Mat());

#endif /* VMML_SRC_IMAGEPREPROCESSOR_H_ */
