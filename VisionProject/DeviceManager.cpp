#include "DeviceManager.h"
#include "HK_Camera.h"
#include"Balser_Cam.h"
#include<iostream>
Camera* DeviceManager::creatCamIns(int camType)
{
    Camera* ptr = nullptr;
    if (camType == HK_CAM) {
        ptr = new HK_Camera();
    }
    else if(camType==BALSER_CAM){
        ptr = new Balser_Cam();
    }
    return ptr;
}

void DeviceManager::addDevice(Device* dev, int type)
{
    if (dev == nullptr) {
        std::cerr << "设备地址为空" << std::endl;
        return;
    }
    //一个pair对，键：是m_mapDev的迭代器，值是bool,
    // 第一方面可以直接当迭代器，第二方面可以看出我是否插入成功
    std::pair<std::map<int, Device*>::iterator, bool> ret;
    ret = m_mapDev.insert(std::pair<int, Device*>(type, dev));//已pair对插入，？？不知为何直接插入不行
    if (!ret.second) {
        std:: cout << "此设备已经添加注册" << std::endl;
    }
    return;
}

Device* DeviceManager::getDevice(int type)
{
    return m_mapDev[type];
}

void DeviceManager::relaseDev()
{
    std::map<int, Device*>::iterator it;
    it = m_mapDev.begin();
    for (; it != m_mapDev.end(); it++) {
        if (it->second != nullptr) {
            delete it->second;
            it->second = nullptr;
        }
    }
    m_mapDev.clear();
}

DeviceManager& DeviceManager::getInstance()
{
    static DeviceManager instance;
    return instance;
}
