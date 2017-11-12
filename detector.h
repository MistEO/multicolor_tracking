#pragma once

#include <opencv2\opencv.hpp>
#include <string>
#include <map>
#include <utility>

#include "objectinfo.h"

//��������
enum RectAttr {
	LeftX, RightX, TopY, BottomY, CenterX, CenterY,
	Height, Width, Area
};

//ͼ�����࣬��̬����ģʽ������Detector::ins()��ȡ��������
class Detector {
public:
	//��ֹ�����͸�ֵ
	Detector(const Detector&) = delete;
	Detector & operator=(const Detector&) = delete;

	//��ȡ��̬����
	static Detector & ins();
	
	//����ͼ�����ţ�������Сͼ����������ܣ�����refresh��������
	void set_scale(double scale);
	//ˢ��֡
	int refresh(uint times = 1);

	//�����µ�׷��ɫ
	void insert(const std::string & name,
		bool mixed_flag = false, int count = 1);
	//ɾ��ĳ��׷��ɫ
	void erase(const std::string & name);
	//�������
	void clear();
	//����ĳ��׷��ɫ�Ƿ����
	bool exist(const std::string & name, const int order = 1) const;

	//�ֶ�����ĳ��׷��ɫHSVֵ
	void adjust_color(const std::string & object_name,
		int confirm_keycode = 13, int cancel_keycode = 27);
	//����ĳ��׷��ɫ
	void process(const std::string & object_name, const std::string & window_name = "");
	//��ȡ��������ֵ
	int get_rect_attr(const std::string & object_name, const int order, RectAttr attr) const;

	//��ʾ����
	void show();
	
private:
	//��ȡ�ͱ����HSVֵ��YMAL�ļ�
	void load(const std::string & name);
	void save(const std::string & name);

	//˽�л��������������
	Detector();
	~Detector();

	cv::VideoCapture capture;
	//���ƺ���ɫ��Ϣ��ֵ��
	std::map<std::string, ObjectInfo> object_map;
	//���ű���
	double scale = 1.0;
	//ԭͼ�����ź�ͼ��hsvͼ����ʾ��ͼ��
	cv::Mat source_image, scale_image, hsv_image, show_image;
};