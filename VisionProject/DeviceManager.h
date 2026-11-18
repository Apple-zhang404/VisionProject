#pragma once

#include<map>
#include<mutex>
#include"Camera.h"

//相机品牌枚举：创建相机时用
enum {
	HK_CAM,
	BALSER_CAM
};

//设备管理器（单例）
//职责：
//  1) 根据品牌创建相机对象（工厂）
//  2) 按编号管理已创建的设备（1~4号相机）
//  3) 统一释放，避免泄漏
//为什么用单例：全局只有一份设备表，任何模块拿到的都是同一批对象
class DeviceManager {
public:
	//工厂：按品牌创建相机实例，返回的指针交给 addDevice 管理
	Camera* creatCamIns(int camType);

	//按编号登记设备（编号从1开始，对应1~4号相机）
	//重复登记的编号会被拒绝并打印提示
	void addDevice(Device* dev, int type);

	//按编号取设备，没有则返回 nullptr
	//注意：这里不能用 m_mapDev[type]，因为 std::map 的 operator[] 在键不存在时会插入空项
	Device* getDevice(int type);

	//释放所有设备并清空表
	void relaseDev();

	//当前管理了多少台设备
	int size();

public:
	//唯一获取单例的接口。C++11 起局部静态变量初始化是线程安全的
	static DeviceManager& getInstance();
private:
	DeviceManager() = default;
	virtual ~DeviceManager() = default;
	DeviceManager(const DeviceManager&) = delete;
	DeviceManager& operator=(const DeviceManager&) = delete;
	DeviceManager(DeviceManager&&) = delete;
	DeviceManager& operator=(DeviceManager&&) = delete;
private:
	//键：设备编号（1~4号相机）；值：设备指针
	std::map<int, Device*> m_mapDev;
	std::mutex m_mtx;   //设备表可能被多个线程访问，加锁保护
};
