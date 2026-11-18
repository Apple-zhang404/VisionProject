

#pragma once

#include"Camera.h"
#include<opencv2/opencv.hpp>
#include"MvCameraControl.h"   //新增：海康相机SDK

//这里不能写 using namespace cv; 否则 cv::ACCESS_MASK 会和 Windows.h 里的 ACCESS_MASK 冲突

typedef void (*callback_pushImg)(cv::Mat, void*);
class VisionProject;

class HK_Camera :public Camera {
public:
    HK_Camera();
    virtual  ~HK_Camera();
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
     * 停止采集（停止连续采集）
     * 返回值：是否停止成功
     */
    virtual bool stopCap() {
        return stopCapture();   //新增：统一走 stopCapture
    }
    //新增：连续采集的启停由相机自己管，调用方不直接碰 m_bstopSnap
    bool startCapture() {
        m_bstopSnap = false;    //开始前清标志，避免上一次停止留下的 true
        return true;
    }
    bool stopCapture() {
        m_bstopSnap = true;
        return true;
    }
    /*
    * 获取当前相机是否处于打开状态
    */
    virtual bool isOpen() {
        return m_bcamopen;
    }
public:
    //新增：枚举相机（静态，一次就够了），返回设备个数
    static int getEnumDevices();
    //新增：按序号给这一路相机创建句柄
    void* createHandle(int nIndex);

    //获取图像信息
    cv::Mat getImageData() {
        return  m_imgData;
    }
    //s设置离线采集文件路径
    void setOffLineGrapImgPath(cv::String imgPath) {
        m_imageFilePath = imgPath;
    }
    //设置是否离线采集，true离线采集
    void setOffLineFlags(bool mark) {
        m_boffLine = mark;
    }
    //设置回调函数,在主类构造函数中初始化
    //第一个参数是回调函数，我需要主类把添加图片进入主类的函数传给我，以便我相机添加一张图片主类添加一张进队
    //第二个参数是主类指针，因为传入的函数是静态函数，需要主类指针才能访问到队列
    void setGrapImageCallbackFunc(callback_pushImg callback, VisionProject* pmainWin) {
        m_callback_pushImg = callback;
        m_pmainWin = pmainWin;
    }
private:
    //新增：把SDK的RGB数据转成OpenCV的BGR
    void RGB2BGR(unsigned char* pRgbData, unsigned int nWidth, unsigned int nHeight);
    //新增：把SDK的一帧数据转成cv::Mat
    bool Convert2Mat(MV_FRAME_OUT_INFO_EX* pstImageInfo, unsigned char* pData);
private:
    bool m_bcamopen = false;//相机是否打开，true打开
    bool m_boffLine = false;//是否离线采集标志
    cv::Mat m_imgData;//存放采集后的图片数据
    cv::String m_imageFilePath;//离线采集图片的文件路径
    bool m_bstopSnap = false;//停止连续采集标志
    int m_nImgIndex = 1;     //新增：离线图片序号（改成成员，4路相机各数各的）

    callback_pushImg m_callback_pushImg = nullptr;
    VisionProject* m_pmainWin = nullptr;

    //新增：SDK相关
    void* handle = nullptr;                              //相机句柄
    MV_CC_DEVICE_INFO* pCamInfo = nullptr;               //设备信息
    static MV_CC_DEVICE_INFO_LIST stDeviceList;          //枚举出来的设备列表
};


