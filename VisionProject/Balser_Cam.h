
#pragma once

#include "Camera.h"
class Balser_Cam :public Camera
{   
public:
    Balser_Cam();
    virtual ~Balser_Cam();
    /// <summary>
  /// 打开相机
  /// 反回是否打开成功
  /// </summary>
  /// <returns></returns>
    virtual bool openCam();
    /*
     * 关闭相机
     * 返回是否关闭成功
     */
    virtual bool closeCam();
    /*
     * 单帧采集
     */
    virtual bool getFrame();
    /*
     * 连续采集
     */
    virtual bool sanpcap();
    /*
     * 停止采集
     * 返回值：是否停止成功
     */
    virtual bool stopCap();
private:
    bool m_bcamopen;//相机是否关闭
};

