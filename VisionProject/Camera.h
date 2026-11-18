
#pragma once

#include"Device.h"
class Camera:public Device{
public:
    Camera();
    virtual  ~Camera();

    virtual bool openCam() = 0;
    /*
     * 关闭相机
     */
    virtual bool closeCam() = 0;
    /*
     * 单帧采集
     */
    virtual bool getFrame() = 0;
    /*
     * 连续采集
     */
    virtual bool sanpcap() = 0;
    /*
     * 停止采集
     */
    virtual bool stopCap() = 0;

    virtual bool init() override { return true; }
    virtual bool getError() override { return false; }
    virtual bool enable() override { return true; }
    virtual bool disable() override { return true; }
};
