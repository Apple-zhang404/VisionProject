#include "HK_Camera.h"
#include <iostream>
#include <cstdio>
#include <fstream>   //新增：用 ifstream 判断文件是否存在（避免引入 windows.h）

//新增：静态成员定义
MV_CC_DEVICE_INFO_LIST HK_Camera::stDeviceList = { 0 };

HK_Camera::HK_Camera() {
}

HK_Camera::~HK_Camera()
{
	//新增：析构时销毁句柄，避免泄漏
	if (handle != nullptr) {
		if (m_bcamopen) {
			MV_CC_CloseDevice(handle);
		}
		MV_CC_DestroyHandle(handle);
		handle = nullptr;
	}
}

//新增：枚举相机（GigE + USB），结果存在 stDeviceList 里
int HK_Camera::getEnumDevices()
{
	memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
	int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
	if (MV_OK != nRet) {
		std::cout << "枚举相机失败, nRet = " << nRet << std::endl;
		return 0;
	}
	std::cout << "枚举到相机数量: " << stDeviceList.nDeviceNum << std::endl;
	return (int)stDeviceList.nDeviceNum;
}

//新增：按序号创建句柄（nIndex 从 0 开始）
void* HK_Camera::createHandle(int nIndex)
{
	if (stDeviceList.nDeviceNum <= 0) {
		std::cout << "没有可用的相机设备" << std::endl;
		return nullptr;
	}
	if (nIndex < 0 || nIndex >= (int)stDeviceList.nDeviceNum) {
		std::cout << "相机序号越界: " << nIndex << std::endl;
		return nullptr;
	}
	pCamInfo = stDeviceList.pDeviceInfo[nIndex];        //记下这台设备的信息
	int nRet = MV_CC_CreateHandle(&handle, pCamInfo);
	if (MV_OK != nRet) {
		std::cout << "创建句柄失败! nRet = " << nRet << std::endl;
		handle = nullptr;
		return nullptr;
	}
	return handle;
}

bool HK_Camera::openCam()
{
	if (!m_bcamopen) {
		if (m_boffLine) {//离线模式：不用管句柄
			m_bcamopen = true;
			return true;
		}
		//在线模式：真正打开设备
		if (handle == nullptr) {
			std::cout << "相机句柄为空，先调用 createHandle" << std::endl;
			return false;
		}
		int nRet = MV_CC_OpenDevice(handle);
		if (MV_OK != nRet) {
			std::cout << "打开相机失败! nRet = " << nRet << std::endl;
			m_bcamopen = false;
			return false;
		}
		//千兆网相机设置一下最佳包大小
		if (pCamInfo != nullptr && pCamInfo->nTLayerType == MV_GIGE_DEVICE) {
			int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
			if (nPacketSize > 0) {
				MV_CC_SetIntValue(handle, "GevSCPSPacketSize", nPacketSize);
			}
		}
		m_bcamopen = true;
	}
	return m_bcamopen;
}

bool HK_Camera::closeCam()
{
	if (!m_bcamopen) {
		return false;
	}
	if (!m_boffLine && handle != nullptr) {
		MV_CC_CloseDevice(handle);
	}
	m_bcamopen = false;
	return true;
}

bool HK_Camera::getFrame()
{
	if (!m_bcamopen) {
		return false;
	}
	if (m_boffLine) {//离线采集：从文件夹读图
		if (m_imageFilePath.empty()) {
			std::cout << "离线采集路径为空" << std::endl;
			return false;
		}
		char filename[512];
		snprintf(filename, sizeof(filename), "%s/picture_%d.png", m_imageFilePath.c_str(), m_nImgIndex);//拼出完整文件名，注意带扩展名
		{//新增：先判断文件在不在，报错更好懂
			std::ifstream probe(filename);
			if (!probe.good()) {
				std::cout << "找不到离线图片: " << filename
					<< "（目录里需要有 picture_1.png ~ picture_N.png）" << std::endl;
				return false;
			}
		}
		m_imgData = cv::imread(filename);//此时把照片从文件中存到了该对像中
		if (m_imgData.empty()) {
			std::cout << "读取图片失败: " << filename << std::endl;
			return false;
		}
		m_nImgIndex++;
		if (m_nImgIndex > 7) m_nImgIndex = 1;
	}
	else {//新增：在线采集，从相机取一帧
		if (handle == nullptr) {
			return false;
		}
		MV_FRAME_OUT stOut = { 0 };
		int nRet = MV_CC_GetImageBuffer(handle, &stOut, 1000);
		if (MV_OK != nRet) {
			std::cout << "采集图像失败! nRet = " << nRet << std::endl;
			return false;
		}
		bool bOk = Convert2Mat(&stOut.stFrameInfo, stOut.pBufAddr);
		MV_CC_FreeImageBuffer(handle, &stOut);
		if (!bOk) {
			return false;
		}
	}
	return true;
}

bool HK_Camera::sanpcap()
{
	if (!m_bcamopen) {
		return false;
	}
	//新增：进循环前先把停止标志清掉。否则之前 stopSnap() 留下的 true 会让这一路
	//      一进循环就立刻退出（表现出来就是"只有第1路没画面"）
	m_bstopSnap = false;
	if (m_boffLine) {
		while (!m_bstopSnap) {//用循环一直采集图片入队列
			char filename[512];
			snprintf(filename, sizeof(filename), "%s/picture_%d.png", m_imageFilePath.c_str(), m_nImgIndex);//拼出完整文件名，注意带扩展名
			{
				std::ifstream probe(filename);
				if (!probe.good()) {
					std::cout << "找不到离线图片: " << filename << std::endl;
					return false;
				}
			}
			m_imgData = cv::imread(filename);//此时把照片从文件中存到了该对像中
			if (m_imgData.empty()) {
				std::cout << "读取图片失败: " << filename << std::endl;
				return false;
			}
			m_nImgIndex++;
			if (m_nImgIndex > 7) m_nImgIndex = 1;
			m_callback_pushImg(m_imgData, m_pmainWin);//回调函数，像队列里面存图片用的
		}
		m_bstopSnap = false;//退出循环之后停止采集，但是标志位会一直是停止采集，
		//所以我初始化一下标志位继续采集，这样下次再摁连续采集的话化还是停止采集
	}
	else {//新增：在线连续采集，一直取帧然后回调入队
		if (handle == nullptr) {
			return false;
		}
		while (!m_bstopSnap) {
			MV_FRAME_OUT stOut = { 0 };
			int nRet = MV_CC_GetImageBuffer(handle, &stOut, 1000);
			if (MV_OK != nRet) {
				continue;   //超时就再来一次
			}
			if (Convert2Mat(&stOut.stFrameInfo, stOut.pBufAddr)) {
				m_callback_pushImg(m_imgData, m_pmainWin);
			}
			MV_CC_FreeImageBuffer(handle, &stOut);
		}
		m_bstopSnap = false;
	}
	return true;
}

//新增：RGB -> BGR（海康给的是RGB，OpenCV要BGR）
void HK_Camera::RGB2BGR(unsigned char* pRgbData, unsigned int nWidth, unsigned int nHeight)
{
	int nDataSize = (int)(nWidth * nHeight * 3);
	for (int i = 0; i < nDataSize; i += 3) {
		unsigned char tmp = pRgbData[i];
		pRgbData[i] = pRgbData[i + 2];
		pRgbData[i + 2] = tmp;
	}
}

//新增：把SDK的一帧数据转成 cv::Mat
bool HK_Camera::Convert2Mat(MV_FRAME_OUT_INFO_EX* pstImageInfo, unsigned char* pData)
{
	if (pstImageInfo == nullptr || pData == nullptr) {
		return false;
	}
	if (PixelType_Gvsp_Mono8 == pstImageInfo->enPixelType) {//黑白
		m_imgData = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC1, pData).clone();
	}
	else if (PixelType_Gvsp_RGB8_Packed == pstImageInfo->enPixelType) {//彩色
		RGB2BGR(pData, pstImageInfo->nWidth, pstImageInfo->nHeight);
		m_imgData = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC3, pData).clone();
	}
	else {
		std::cout << "不支持的像素格式: " << pstImageInfo->enPixelType << std::endl;
		return false;
	}
	return !m_imgData.empty();
}
