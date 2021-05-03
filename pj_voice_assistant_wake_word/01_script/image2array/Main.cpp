/*** Include ***/
/* for general */
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

#include <fstream>
#include <iostream>
#include <iterator>

/* for OpenCV */
#include <opencv2/opencv.hpp>

/*** Macro ***/
/* Settings */
static const int32_t WIDTH = 100;
static const int32_t HEIGHT = 100;

int32_t main()
{
	std::string filename = "color_bar.jpg";
	cv::VideoCapture cap(filename);

	cv::Mat resultMat = cv::Mat::zeros(cv::Size(WIDTH, HEIGHT), CV_8UC3);
	std::ofstream ofs("logo_data.bin", std::ios::out | std::ios::binary);
	std::ofstream ofsCode("logo_data.h", std::ios::out);
	ofsCode << std::showbase;
	ofsCode << "constexpr int32_t kLogoWidth = 100;\n";
	ofsCode << "constexpr int32_t kLogoHeight = 100;\n";
	ofsCode << "constexpr uint8_t logo_data[][2 * kLogoWidth * kLogoHeight] = {\n\n";

	int index = 0;
	while (1) {
		cv::Mat inputMat;
		if (cap.isOpened()) {
			cap.read(inputMat);
		} else {
			inputMat = cv::imread(filename);
		}
		if (inputMat.empty()) break;


		double scale = (std::min)(static_cast<double>(WIDTH) / inputMat.cols, static_cast<double>(HEIGHT) / inputMat.rows);
		cv::resize(inputMat, inputMat, cv::Size(), scale, scale);
		inputMat.copyTo(resultMat(cv::Rect((resultMat.cols - inputMat.cols) / 2, (resultMat.rows - inputMat.rows) / 2, inputMat.cols, inputMat.rows)));
		cv::imshow("resultMat", resultMat);
		cv::Mat mat565;
		//cv::cvtColor(resultMat, mat565, cv::COLOR_BGR2BGR565);
		cv::cvtColor(resultMat, mat565, cv::COLOR_RGB2BGR565);	// ?

		if (cv::waitKey(1) == 'q') break;

		//std::vector<uint8_t> resultData;
		//for (int32_t page = 0; page < PAGE_NUM; page++) {
		//	for (int32_t x = 0; x < WIDTH; x++) {
		//		uint8_t pageData = 0;
		//		for (int32_t i = 0; i < PAGE_SIZE; i++) {
		//			int32_t index = (page * PAGE_SIZE + i) * WIDTH + x;
		//			uint8_t onoff = (resultMat.data[index] > THRESHOLD_BLIGHTNESS) ? 1 : 0;
		//			pageData |= onoff << i;
		//		}
		//		resultData.push_back(pageData);
		//	}
		//}
		//(void)ofs.write(reinterpret_cast<char*>(resultData.data()), resultData.size());
		
		ofsCode << "{ ";
		for (int i = 0; i < mat565.total(); i++) {
			ofsCode << std::hex << static_cast<int32_t>(mat565.data[2 * i + 1]) << ", ";
			ofsCode << std::hex << static_cast<int32_t>(mat565.data[2 * i + 0]) << ", ";
		}
		ofsCode << " },\n";

		if (inputMat.empty()) break;
	}

	ofsCode << "\n\n};\n";
	ofsCode.close();
	ofs.close();

	return 0;
}
