#include "DeviceManager.h"
#include "HK_Camera.h"
#include"Balser_Cam.h"
#include<iostream>
#include"Logger.h"

//工厂：按品牌创建相机对象
//返回裸指针是有意的——所有权交给 DeviceManager，由 relaseDev() 统一 delete
Camera* DeviceManager::creatCamIns(int camType)
{
    Camera* ptr = nullptr;
    if (camType == HK_CAM) {
        ptr = new HK_Camera();
    }
    else if (camType == BALSER_CAM) {
        ptr = new Balser_Cam();
    }
    else {
        LOG_ERROR(QString("未知的相机品牌: %1").arg(camType));
        return nullptr;
    }
    return ptr;
}

//登记设备。用 insert 而不是 operator[]：operator[] 在键已存在时会覆盖，
//而 insert 能通过返回值告诉我们"是不是已经登记过"
void DeviceManager::addDevice(Device* dev, int type)
{
    if (dev == nullptr) {
        LOG_ERROR(QString("设备地址为空，编号: %1").arg(type));
        return;
    }
    std::lock_guard<std::mutex> lk(m_mtx);
    std::pair<std::map<int, Device*>::iterator, bool> ret =
        m_mapDev.insert(std::pair<int, Device*>(type, dev));
    if (!ret.second) {
        LOG_WARN(QString("设备编号 %1 已存在，忽略本次登记").arg(type));
    }
}

//按编号取设备：用 find，找不到就返回 nullptr，不会往表里插空项
Device* DeviceManager::getDevice(int type)
{
    std::lock_guard<std::mutex> lk(m_mtx);
    std::map<int, Device*>::iterator it = m_mapDev.find(type);
    if (it == m_mapDev.end()) {
        return nullptr;
    }
    return it->second;
}

//释放所有设备（进程退出前调用，避免内存泄漏）
void DeviceManager::relaseDev()
{
    std::lock_guard<std::mutex> lk(m_mtx);
    for (std::map<int, Device*>::iterator it = m_mapDev.begin(); it != m_mapDev.end(); ++it) {
        if (it->second != nullptr) {
            delete it->second;
            it->second = nullptr;
        }
    }
    m_mapDev.clear();
}

int DeviceManager::size()
{
    std::lock_guard<std::mutex> lk(m_mtx);
    return (int)m_mapDev.size();
}

DeviceManager& DeviceManager::getInstance()
{
    static DeviceManager instance;   //局部静态：C++11 保证线程安全的懒初始化
    return instance;
}
