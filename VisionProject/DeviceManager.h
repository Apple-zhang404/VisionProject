
#pragma once

#include<map>
#include"Camera.h"

//枚举我管理/创建这两个相机中的哪一个
enum {
	HK_CAM,
	BALSER_CAM
};

//我要建造一个单例模式，构造函数私有化，移动拷贝构造删除
//构建唯一单例用static magic，即构造函数私有化
class DeviceManager {
public:
	//构造相机，因为是管理类
	//参数是相机的类型，上面的枚举类
	Camera* creatCamIns(int camType);

	//添加要管理的设备，类型 地址
	void addDevice(Device* dev,int type);

	//获取管理设备中的设备
	Device* getDevice(int type);

	//释放所有设备
	void relaseDev();
public:
	//唯一获取单例的函数，且适配多线程，单例模式还很轻量
	static DeviceManager& getInstance();
private:
	DeviceManager() = default;
	virtual ~DeviceManager() = default;
	//删除拷贝
	DeviceManager(const DeviceManager&) = delete;
	DeviceManager& operator=(const DeviceManager&) = delete;
	//删除移动
	DeviceManager(DeviceManager&&) = delete;
	DeviceManager& operator=(DeviceManager&&) = delete;
private:
	//存放设备的容器，键：设备的类型 值::设备的地址
	//？？有一个问题这键是设备类型，那我相机这个类型有多个相机呢，不能只假设有俩个不捅破品牌的相机
	std::map<int, Device*> m_mapDev;

private:

};