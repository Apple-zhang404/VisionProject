
#pragma once

//纯虚类，规范设备用的
//有许多设备，相机，光源，等等
class Device {
public:
	Device();
	virtual	~Device();
	//设备初始化
	virtual bool init() = 0;
	//获取错误码，初始化时可能错误
	virtual bool getError() = 0;
	//设备使能，让设开机
	virtual bool enable() = 0;
	//设备失能，让设备关机
	virtual bool disable() = 0;
};