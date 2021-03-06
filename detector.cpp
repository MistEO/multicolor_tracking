#include "detector.h"
#include <iostream>
#include <cassert>

const char * WindowName = "Detector";

Detector & Detector::ins()
{
	static Detector unique_ins;
	return unique_ins;
}

Detector::Detector() : capture(0)
{
	if (!capture.isOpened()) {
		throw cv::Exception(
			-1, "The video capture isn't opened!",
			__FUNCTION__, __FILE__, __LINE__);
	}
	cv::namedWindow(WindowName, cv::WINDOW_NORMAL);
}

Detector::~Detector()
{
	capture.release();
}

void Detector::set_scale(double scale)
{
	this->scale = scale;
	scale_image.release();
	hsv_image.release();
	show_image.release();
}

int Detector::refresh(uint times)
{
	cv::Size capture_size(
		capture.get(cv::CAP_PROP_FRAME_WIDTH),
		capture.get(cv::CAP_PROP_FRAME_HEIGHT));
	int c = 0;
	for (uint i = 0; i != times; ++i) {
		c = cv::waitKey(40);
		capture >> source_image;
	}
	cv::resize(source_image, scale_image, capture_size);
	cv::cvtColor(scale_image, hsv_image, cv::COLOR_BGR2HSV);
	return c;
}

void Detector::insert(const std::string & name,
	bool mixed_flag, int count)
{
	object_map[name] = ObjectInfo(mixed_flag, count);
	load(name);
}

void Detector::erase(const std::string & name)
{
	object_map.erase(name);
}

void Detector::clear()
{
	object_map.clear();
}

bool Detector::exist(const std::string & name, const int order) const
{
	if (object_map.find(name) != object_map.end()
		&& object_map.at(name).count >= order) {
		return true;
	}
	return false;
}

void Detector::adjust_color(const std::string & object_name, int confirm_keycode, int cancel_keycode)
{
	if (!exist(object_name)) {
		throw cv::Exception(
			1, "There is no " + object_name + " in the map",
			__FUNCTION__, __FILE__, __LINE__);
	}
	using cv::createTrackbar;
	ObjectInfo & oinfo = object_map[object_name];
	const std::string adjust_window_name = object_name + " - Adjust Color";
	cv::namedWindow(adjust_window_name, cv::WINDOW_AUTOSIZE);
	createTrackbar("Low H", adjust_window_name, &oinfo.lower[0], 181);
	createTrackbar("Up H", adjust_window_name, &oinfo.upper[0], 181);
	if (oinfo.mixed) {
		createTrackbar("Low H 2", adjust_window_name, &oinfo.lower[1], 181);
		createTrackbar("Up H 2", adjust_window_name, &oinfo.upper[1], 181);
	}
	createTrackbar("Low S", adjust_window_name, &oinfo.lower[2], 256);
	createTrackbar("Up S", adjust_window_name, &oinfo.upper[2], 256);
	createTrackbar("Low V", adjust_window_name, &oinfo.lower[3], 256);
	createTrackbar("Up V", adjust_window_name, &oinfo.upper[3], 256);

	int c = 0;
	while (c != confirm_keycode && c != cancel_keycode) {
		c = refresh();
		oinfo.refresh_scalar();
		process(object_name, adjust_window_name);
	}
	if (c == confirm_keycode) {
		save(object_name);
	}

	cv::destroyWindow(adjust_window_name);
}

void Detector::process(const std::string & object_name, const std::string & window_name)
{
	using namespace cv;
	if (!exist(object_name)) {
		throw cv::Exception(
			1, "There is no " + object_name + " in the map",
			__FUNCTION__, __FILE__, __LINE__);
	}
	ObjectInfo & oinfo = object_map[object_name];
	Mat binary_image;
	//按范围二值化，区间前开后闭
	inRange(hsv_image, oinfo.get_lower(), oinfo.get_upper(), binary_image);
	//合并混合色的二值化图像
	if (oinfo.mixed) {
		Mat binary_image2;
		inRange(hsv_image, oinfo.get_lower(true), oinfo.get_upper(true), binary_image2);
		addWeighted(binary_image2, 1, binary_image, 1, 0, binary_image);
	}
	morphologyEx(binary_image, binary_image, cv::MORPH_OPEN,
		getStructuringElement(MORPH_RECT, Size(10 * scale, 10 * scale)));	//开操作降噪
	morphologyEx(binary_image, binary_image, cv::MORPH_CLOSE,
		getStructuringElement(MORPH_RECT, Size(10 * scale, 10 * scale)));	//闭操作降噪

	using std::vector;
	vector<vector<Point> > all_contours;
	vector<Vec4i> hierarchy;
	Mat contours_image = binary_image.clone();
	findContours(contours_image, all_contours, hierarchy,
		cv::RETR_TREE, cv::CHAIN_APPROX_NONE);		//查找轮廓
	oinfo.rect_set.clear();
	for (auto & i : all_contours) {
		oinfo.rect_set.insert(boundingRect(Mat(i)));//从轮廓提取矩形，并加入set
	}

	//画某一个颜色的show_image
	if (!window_name.empty()) {
		Mat show_image = Mat::zeros(scale_image.size(), scale_image.type());
		show_image = scale_image.clone();
		Mat bgr_binary_image;
		cvtColor(binary_image, bgr_binary_image, CV_GRAY2BGR);	//转换为三通道，以合并
		const double alpha = 0.5, beta = 0.5, gamma = 0.0;
		addWeighted(show_image, alpha, bgr_binary_image, beta, gamma, show_image);	//线性叠加
		line(show_image, cv::Point(show_image.cols / 2, 0),
			Point(show_image.cols / 2, show_image.rows),
			Scalar(255, 0, 0), 2);		//画图像中轴线
		if (!oinfo.rect_set.empty()) {
			for (auto i : oinfo.rect_set) {	//画所有矩形
				rectangle(show_image, i, cv::Scalar(0, 0, 0), 1);
			}
			int rectangle_count = 0;
			for (auto i : oinfo.rect_set) {
				rectangle(show_image, i, oinfo.get_average(), 2);	//画最大几个矩形
				if (++rectangle_count == oinfo.count) {
					break;
				}
			}
		}
		imshow(window_name, show_image);
	}
}

int Detector::get_rect_attr(const std::string & object_name, const int order, RectAttr attr) const
{
	if (!exist(object_name, order)) {
		throw cv::Exception(
			1, "There is no " + object_name + " in the map",
			__FUNCTION__, __FILE__, __LINE__);
	}
	for (auto i : object_map) {
		for (auto j : i.second.rect_set) {
			if (i.second.count == order) {
				switch (attr)
				{
				case LeftX:
					return j.x;
				case RightX:
					return j.br().x;
				case TopY:
					return j.y;
				case BottomY:
					return j.br().y;
				case CenterX:
					return (j.x + j.br().x) / 2;
				case CenterY:
					return (j.y + j.br().y) / 2;
				case Height:
					return j.height;
				case Width:
					return j.width;
				case Area:
					return j.area();
				default:
					return -1;
				}
			}
		}
	}
	return -2;
}

void Detector::show()
{
	show_image = scale_image.clone();
	for (auto i : object_map) {
		int rectangle_count = 0;
		for (auto j : i.second.rect_set) {
			cv::rectangle(show_image, j, i.second.get_average(), 2);
			if (++rectangle_count == i.second.count) {
				break;
			}
		}
	}
	cv::imshow(WindowName, show_image);
}

void Detector::load(const std::string & name)
{
	cv::FileStorage fs(name + ".yaml", cv::FileStorage::READ);
	if (!fs.isOpened()) {
		std::clog << "Cannot read file: " << name + ".yaml" << std::endl;
		return;
	}
	ObjectInfo & oinfo = object_map[name];
	cv::FileNode fn_lower = fs[name + "-lower"],
		fn_upper = fs[name + "-upper"];
	cv::FileNodeIterator it_lower = fn_lower.begin(),
		it_upper = fn_upper.begin();
	for (int i = 0; i != 4; ++i, ++it_lower, ++it_upper) {
		oinfo.lower[i] = static_cast<int>(*it_lower);
		oinfo.upper[i] = static_cast<int>(*it_upper);
	}
	oinfo.refresh_scalar();
	fs.release();
}

void Detector::save(const std::string & name)
{
	cv::FileStorage fs(name + ".yaml", cv::FileStorage::WRITE);
	ObjectInfo & oinfo = object_map[name];
	fs << name + "-lower" << "[" << oinfo.lower[0] << oinfo.lower[1] << oinfo.lower[2] << oinfo.lower[3] << "]";
	fs << name + "-upper" << "[" << oinfo.upper[0] << oinfo.upper[1] << oinfo.upper[2] << oinfo.upper[3] << "]";
	fs.release();
}
