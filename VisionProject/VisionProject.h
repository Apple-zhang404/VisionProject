
#pragma once

#include "socket_client.h"   //新增：socket客户端（winsock2.h 要放在最前面，早于windows.h）
#include "Task.h"              //新增：Base64编解码
#include <QtWidgets/QMainWindow>
#include <QAction>          //新增：工具栏动作
#include "ui_VisionProject.h"
#include"DeviceManager.h"
#include"HK_Camera.h"
#include"Balser_Cam.h"
#include<thread>
#include<atomic>   //新增：原子计数
#include<chrono>
#include<map>
#include<queue>
#include<mutex>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include"XMlConfigOperator.h"
#include"ImageProcess.h"   //新增：算法层
class VisionProject : public QMainWindow
{
    Q_OBJECT

public:
    VisionProject(QWidget *parent = nullptr);
    ~VisionProject();
public:
    void display();
private slots:
    void on_actGetFrame();    //单帧采集
    void on_actOpenCam();     //打开相机
    void on_actCloseCam();    //关闭相机
    void on_actSnapGrap();    //连续采集
    void on_actStopCaptrue(); //停止采集
    void on_pbtnOpenDir();   //打开文件路径
    void on_chxBoxOffLine(Qt::CheckState state); //启用离线模式响应
    void on_pbtn_imageTool_clicked();//打开图像参数设计窗口
public:
    //新增：把XML配置对象的指针给ImageTool用，让它读写参数
    XMlConfigOperator* getXMLPtr() {
        return m_doc;
    }
    //新增：把算法层对象给ImageTool用（老师0130也是这么拿的）
    ImageProcess* getImgProcPtr() {
        return &m_imgProcess;
    }
    //新增：图像 <-> Base64 字符串（老师最终版里的两个工具函数）
    static std::string Mat2Base64_JPEG(const cv::Mat& img);   //Mat 压成JPG再转Base64
    static cv::Mat Base2Mat(std::string& base64_data);         //Base64还原成Mat

    //新增：socket的三个回调，注册给SocketClient用
    static void connectServerCallbackFunc(void* p);   //连接成功后：发账号密码
    static void recvCallbackFunc(void* p, char msg[]);//收到服务器数据：解析
    static void keepAliveCallbackFunc(void* p);       //心跳：每10秒发一次
private:
    //开始连续采集
    void startSnap() {
        m_bStartSnapCrap = true;
    }
    //停止连续采集
    void stopSnap() {
        m_bStartSnapCrap = false;
        phk_cam[m_cmb->currentIndex()]->stopCap();//新增：通知当前相机停止连续采集
    }
private:
    static void displayThdFunc(void* para);//显示线程函数
private:
    Ui::VisionProjectClass ui;
private:
    //新增：4个相机各自一个回调，采集到的图直接显示到自己那一路的label上
    static void grapImageCallbackFunc1(cv::Mat img, void* para);//1号相机的采集回调
    static void grapImageCallbackFunc2(cv::Mat img, void* para);//2号相机的采集回调
    static void grapImageCallbackFunc3(cv::Mat img, void* para);//3号相机的采集回调
    static void grapImageCallbackFunc4(cv::Mat img, void* para);//4号相机的采集回调
private:
    HK_Camera* phk_cam[4];//海康相机指针
    Balser_Cam* pBas_cam;//巴斯勒相机指针
private:
    int m_camId;//相机号
    //键：那个相机上显示，值：一个图像的队列，我离线采集采集那些图像用的
    std::map<int, std::queue<cv::Mat>> m_mapImageData;
private:
    QLineEdit* m_lineEdit;//获取文件地址的lineEdit对象,存贮文件地址
    QComboBox* m_cmb;//选择相机的组件
    QCheckBox* m_chk_Off;//是否采用离线模式的组件，勾选符号
private:
    std::thread* tDisp;//显示线程
    std::thread* m_thdRecv;//
    std::thread* m_thdHeart;//
    //新增：工具栏动作指针，连续采集时把按钮禁用掉，防止重复点开多个线程
    QAction* m_actSnapGrap = nullptr;
    QAction* m_actStopCaptrue = nullptr;
    std::atomic<int> m_nSnapThreads{ 0 };//新增：还有几个采集线程在跑
    bool m_bDispFinish;//线程是否退出标注。true退出
    std::mutex m_mtxDisp;//共享变量互斥锁，因为我map是一个共享变量有写有读
    bool m_bStartSnapCrap;//开始连续采集的标志，true开始连续采集，false停止
  

private:
    XMlConfigOperator* m_doc;
    ImageProcess m_imgProcess;   //新增：算法层对象，界面不直接调Halcon
    SocketClient m_socket;       //新增：socket客户端
};
