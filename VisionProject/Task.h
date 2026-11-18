#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include <string>

//Base64编解码：把二进制（比如JPG）转成可放进JSON的字符串
class Base64
{
public:
	Base64()
	{
		m_base64Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	}
	std::string Encode(const unsigned char* str, int bytes);
	std::string Decode(const char* str, int bytes);
private:
	std::string m_base64Table;
	static const char cstBase64pad = '=';
};

//执行任务类（老师最终版里保留的骨架，暂时只用到单例）
class Task
{
public:
	~Task();

	static void grapImageThdFunc(void* para);  //采集线程函数
	static void displayThdFunc(void* para);    //显示图像线程函数
	static void grapImageCallbackFuc(cv::Mat img); //采集回调函数
public:
	static Task* getInstance();      //获取实例对象的唯一接口
private:
	Task();
	static Task* m_pInstance;
private:
	int m_camId;    //相机号
	std::map<int, std::vector<cv::Mat>> m_mapImgDate;     //存放图像数据
};
