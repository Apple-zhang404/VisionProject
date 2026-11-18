#include "VisionProject.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>      //新增：判断文件是否存在
#include <QDir>
#include"ImageTool.h"
#include"json.hpp"                 //新增：JSON库（单头文件）
#include"Logger.h"                 //新增：日志模块
#include"Task.h"                   //新增：Base64编解码
using json = nlohmann::json;       //新增：给个短名字


VisionProject::VisionProject(QWidget *parent)
    : QMainWindow(parent), m_camId(0), m_bDispFinish(false)
{
    m_doc = new XMlConfigOperator("ImageProcessParam.xml");
    if (m_doc->load()) {
        m_doc->ReadFindShapeAllParams();
        m_doc->ReadShapeModelParams();
        m_doc->ReadXMLCodeNodeParams("BarCode");//新增：读一维码那组参数
    }
    //新增：按XML里的配置加载一维码模型、设置条码参数（老师0130的做法）
    {
        std::string barcodeModel = m_doc->GetNodeParam("barCodeModel");
        sBCIParam param;
        param.st_strCodeType = m_doc->GetNodeParam("codeType").c_str();
        m_imgProcess.readBarCodeModel(barcodeModel);
        m_imgProcess.setBarCodeParam(param);
        LOG_INFO(QString("条码模型已加载: %1").arg(QString::fromLocal8Bit(barcodeModel.c_str())));
    }
    //新增：socket初始化（这一步只注册，不连服务器；线程下一步再加）
    m_socket.setMainWinPtr(this);
    m_socket.setSocketPara("127.0.0.1", 8888);
    m_socket.registerConnCallbackFunc(connectServerCallbackFunc);   //连接成功后发账号
    m_socket.registerRecvCallbackFunc(recvCallbackFunc);            //收到数据后解析
    m_socket.registerHeartCallbackFunc(keepAliveCallbackFunc);      //心跳
    m_thdRecv = new std::thread(SocketClient::recvData, &m_socket);
    m_thdHeart = new std::thread(SocketClient::keepAlive, &m_socket);
    //不再 detach：析构时先 stop() 再 join，线程才能干净退出
    //下一步再加这两个线程：
    
    //实例化4个相机并注册到设备管理对象中
    //新增：先枚举一次相机，再给每一路建句柄（没有相机时跳过，离线模式照常能用）
    int nDevNum = HK_Camera::getEnumDevices();
    LOG_INFO(QString("枚举到相机数量: %1").arg(nDevNum));
    for (int i = 0; i < 4; i++) {
        Camera* pcam = DeviceManager::getInstance().creatCamIns(HK_CAM);//实例化一个海康相机
        DeviceManager::getInstance().addDevice(pcam, i + 1);//键用i+1，也就是1~4号相机
        if (nDevNum > 0) {
            ((HK_Camera*)pcam)->createHandle(i % nDevNum);//设备少于4台时循环复用
        }
    }

    ui.setupUi(this);

    //新增：日志接到界面右侧的日志框（信号跨线程排队，子线程写日志也安全）
    connect(&Logger::instance(), &Logger::newLog, this,
        [this](const QString& line) {
            ui.plainTextEdit->appendPlainText(line);
        }, Qt::QueuedConnection);
    LOG_INFO("程序启动");

    //新增：界面布局调整——画面区占大头，日志区窄一些，窗口默认大一点
    resize(1280, 800);
    ui.label->setMinimumSize(300, 220);
    ui.label_2->setMinimumSize(300, 220);
    ui.label_3->setMinimumSize(300, 220);
    ui.label_4->setMinimumSize(300, 220);
    {
        QList<int> sizes;
        sizes << 900 << 300;
        ui.splitter_2->setSizes(sizes);   //左边画面 : 右边日志 = 3 : 1
    }
    //新增：给4个画面加一个浅色边框，看着不那么空
    const char* imgFrameStyle = "border: 1px solid #3a4552; border-radius: 6px; background: #0a0c0e;";
    ui.label->setStyleSheet(imgFrameStyle);
    ui.label_2->setStyleSheet(imgFrameStyle);
    ui.label_3->setStyleSheet(imgFrameStyle);
    ui.label_4->setStyleSheet(imgFrameStyle);
    statusBar()->showMessage("就绪");   //新增：状态栏提示

    //用代码初始化动作
    QAction* actOpenCam = new QAction(QString("打开相机"), this);
    connect(actOpenCam, &QAction::triggered, this, &VisionProject::on_actOpenCam);
    ui.mainToolBar->addAction(actOpenCam);

    QAction* actCloseCam = new QAction(QString("关闭相机"), this);
    connect(actCloseCam, &QAction::triggered, this, &VisionProject::on_actCloseCam);
    ui.mainToolBar->addAction(actCloseCam);

    ui.mainToolBar->addSeparator();   //添加分隔符

    QAction* actGetFrame = new QAction(QString("单帧采集"), this);
    connect(actGetFrame, &QAction::triggered, this, &VisionProject::on_actGetFrame);
    ui.mainToolBar->addAction(actGetFrame);

    m_actSnapGrap = new QAction(QString("连续采集"), this);
    connect(m_actSnapGrap, &QAction::triggered, this, &VisionProject::on_actSnapGrap);
    ui.mainToolBar->addAction(m_actSnapGrap);

    m_actStopCaptrue = new QAction(QString("停止采集"), this);
    connect(m_actStopCaptrue, &QAction::triggered, this, &VisionProject::on_actStopCaptrue);
    ui.mainToolBar->addAction(m_actStopCaptrue);

    ui.mainToolBar->addSeparator();   //添加分隔符

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setPlaceholderText("指定离线采集目录");//给一个默认的值，提示用
    m_lineEdit->setReadOnly(true);//标记为只读，不可以更改
    m_lineEdit->setMaximumWidth(300); //控件宽
    ui.mainToolBar->addWidget(m_lineEdit);//添加到工具蓝

    QPushButton* pbt = new QPushButton("浏览", this);//读文件地址的按钮（新增：原来只显示"..."太窄）
    connect(pbt, &QPushButton::clicked, this, &VisionProject::on_pbtnOpenDir);//把这个按钮的点击信号量与on_pbtnOpenDir这个函数绑定，点击之后就调用这个函数
    ui.mainToolBar->addWidget(pbt);


    ui.mainToolBar->addSeparator();   //添加分隔符

    QLabel* choiceCam_lab = new QLabel("选择相机 ");//一个label提示用的
    ui.mainToolBar->addWidget(choiceCam_lab);

    m_cmb = new QComboBox(this);//选用几号相机，当前所显示的几号相机的下表就代表几号相机，0-3
    QStringList fruits = { "相机1","相机2","相机3","相机4" };
    m_cmb->addItems(fruits);//添加菜单栏
    ui.mainToolBar->addWidget(m_cmb);
    ui.mainToolBar->addSeparator();   //添加分隔符

    m_chk_Off = new QCheckBox("启用离线模式");
    m_chk_Off->setChecked(false);//初始化为假，一开始全是在线模式
    connect(m_chk_Off, &QCheckBox::checkStateChanged, this, &VisionProject::on_chxBoxOffLine);//勾选信号量与:on_chxBoxOffLine绑定
    ui.mainToolBar->addWidget(m_chk_Off);

    QPushButton* pbtn_imageTool = new QPushButton(this);
    pbtn_imageTool->setText("图像参数设置");
    ///pbtn_imageTool->setMaximumWidth();
    ui.mainToolBar->addWidget(pbtn_imageTool);
    connect(pbtn_imageTool, &QPushButton::clicked, this, &VisionProject::on_pbtn_imageTool_clicked);

    

    //把4个相机的指针从设备管理器里取出来
    for (int i = 0; i < 4; i++) {
        phk_cam[i] = dynamic_cast<HK_Camera*>(DeviceManager::getInstance().getDevice(i + 1));

        phk_cam[i]->setOffLineFlags(false);  //初始化为非离线模式
    }

    //新增：4个相机各自注册自己那一路的回调函数，采集到的图直接显示到对应的label上
    phk_cam[0]->setGrapImageCallbackFunc(grapImageCallbackFunc1, this);
    phk_cam[1]->setGrapImageCallbackFunc(grapImageCallbackFunc2, this);
    phk_cam[2]->setGrapImageCallbackFunc(grapImageCallbackFunc3, this);
    phk_cam[3]->setGrapImageCallbackFunc(grapImageCallbackFunc4, this);

    //这个显示线程队列里有图像我就显示（单帧采集的图从这里显示）
    tDisp = new std::thread(displayThdFunc,this);
    //不再 detach：析构时置退出标志再 join
}

VisionProject::~VisionProject()
{
    //新增：退出顺序很重要——先让每个线程"知道该退了"，再 join 等它真的退出
    //1) 显示线程
    m_bDispFinish = true;
    if (tDisp != nullptr && tDisp->joinable()) {
        tDisp->join();
    }
    //2) 采集线程（stopCap 会置相机的停止标志，循环里会退出）
    for (int i = 0; i < 4; i++) {
        phk_cam[i]->stopCap();
    }
    if (m_thdSnap != nullptr && m_thdSnap->joinable()) {
        m_thdSnap->join();
    }
    //3) socket线程（只看标志不够，recv() 阻塞着，必须先 close socket）
    m_socket.stop();
    if (m_thdRecv != nullptr && m_thdRecv->joinable()) {
        m_thdRecv->join();
    }
    if (m_thdHeart != nullptr && m_thdHeart->joinable()) {
        m_thdHeart->join();
    }
    delete m_doc;
    m_doc = nullptr;
}

void VisionProject::on_actOpenCam()
{
    int index = m_cmb->currentIndex();//当前选中的是几号相机
    if (!phk_cam[index]->isOpen()) {//没打开才去打开
        phk_cam[index]->openCam();
        stopSnap();//打开相机时先把这一路之前的连续采集停掉
        QMessageBox::information(this, "信息", "相机打开成功");
    }
    else {
        QMessageBox::information(this, "信息", "相机已经打开");
    }
}

void VisionProject::on_actCloseCam()
{
    int index = m_cmb->currentIndex();
    if (phk_cam[index]->isOpen()) {
        phk_cam[index]->closeCam();
        stopSnap();//关相机前先停止连续采集
        QMessageBox::information(this, "信息", "相机关闭成功");
    }
    else {
        QMessageBox::information(this, "信息", "请检查相机是否打开");
    }
}


//我点击连续采集按钮会把标志位true,之后开一个线程去跑相机的连续采集函数
void VisionProject::on_actSnapGrap()
{
    int index = m_cmb->currentIndex();
    //新增：已经在采集了就不要再开线程（防止连点开出一堆线程）
    if (m_bStartSnapCrap) {
        QMessageBox::information(this, "提示", "连续采集正在进行，请先点[停止采集]");
        return;
    }
    if (phk_cam[index]->isOpen()) {
        if (m_lineEdit->text().isEmpty()) {//看文件路径上是否有路径
            QMessageBox::information(this, "提示", "请查看采集路径是否正确！");
            return;
        }
        //新增：按界面上的勾选设置离线/在线，避免忘了勾离线导致采集失败
        phk_cam[index]->setOffLineFlags(m_chk_Off->isChecked());
        phk_cam[index]->setOffLineGrapImgPath(m_lineEdit->text().toLocal8Bit().toStdString());//把路径设置给相机

        m_bStartSnapCrap = true;
        if (m_actSnapGrap) m_actSnapGrap->setEnabled(false);//新增：开始采集后按钮置灰

        //新增：采集线程存成成员，不再 detach；退出时由析构/停止逻辑 join 回收
        if (m_thdSnap != nullptr && m_thdSnap->joinable()) {//上一路的线程还没回收，先收掉
            m_thdSnap->join();
            delete m_thdSnap;
            m_thdSnap = nullptr;
        }
        m_thdSnap = new std::thread([this, index]() {
            phk_cam[index]->sanpcap();
            //线程结束时回到主线程把按钮恢复
            QMetaObject::invokeMethod(this, [this]() {
                m_bStartSnapCrap = false;
                if (m_actSnapGrap) m_actSnapGrap->setEnabled(true);
                }, Qt::QueuedConnection);
            });
    }
    else {
        QMessageBox::information(this, "信息", "请检查相机是否打开");
    }
}

void VisionProject::on_actStopCaptrue()
{
    int index = m_cmb->currentIndex();
    if (phk_cam[index]->isOpen()) {
        stopSnap();//新增：置标志位并通知相机停止采集
        m_bStartSnapCrap = false;
        if (m_actSnapGrap) m_actSnapGrap->setEnabled(true);//新增：停止后按钮恢复可点
        //新增：等采集线程真正退出再回收，避免它还在跑就析构
        if (m_thdSnap != nullptr && m_thdSnap->joinable()) {
            m_thdSnap->join();
            delete m_thdSnap;
            m_thdSnap = nullptr;
        }
        QMessageBox::information(this, "信息", "停止采集");
    }
    else {
        QMessageBox::information(this, "信息", "请检查相机是否打开");
    }
}

void VisionProject::on_pbtnOpenDir()
{
    //弹出选择文件夹的对话框，把选到的路径填到lineEdit里
    QString folderPath = QFileDialog::getExistingDirectory(this, "选择图像数据文件夹", QDir::homePath());
    if (folderPath.isEmpty()) {
        QMessageBox::information(this, "提示", "请选择正确的文件路径");
        return;
    }
    m_lineEdit->setText(folderPath);
}

void VisionProject::on_chxBoxOffLine(Qt::CheckState state)
{
    //勾选/取消勾选“启用离线模式”时，把4个相机的离线标志一起改掉
    if (state == Qt::Unchecked) {
        for (int i = 0; i < 4; i++) {
            phk_cam[i]->setOffLineFlags(false);
        }
    }
    else if (state == Qt::Checked) {
        for (int i = 0; i < 4; i++) {
            phk_cam[i]->setOffLineFlags(true);
        }
    }
}


void VisionProject::on_actGetFrame() {
    int i = m_cmb->currentIndex();
    if (phk_cam[i]->isOpen()) {
        //新增：按界面上的勾选设置离线/在线（老师把这行注释掉了，导致没勾离线时单帧采集必然失败）
        phk_cam[i]->setOffLineFlags(m_chk_Off->isChecked());
        if (m_lineEdit->text().isEmpty()) {//看文件路径上是否有路径
            QMessageBox::information(this, "提示", "请查看采集路径是否正确！");//如没有
            return;
        }
        phk_cam[i]->setOffLineGrapImgPath(m_lineEdit->text().toLocal8Bit().toStdString());
        if (phk_cam[i]->getFrame()) {//单帧采集成功
            m_mtxDisp.lock();
            m_mapImageData[i].push(phk_cam[i]->getImageData());//采集成功的图片入队
            m_mtxDisp.unlock();
            LOG_INFO(QString("相机%1 单帧采集成功").arg(i + 1));
        }
        else {
            LOG_ERROR(QString("相机%1 单帧采集失败: %2").arg(i + 1)
                .arg(QString::fromLocal8Bit(m_lineEdit->text().toLocal8Bit())));
        }
    }
    else {
        QMessageBox::information(this, "信息", "请选择相机是否打开");
    }

}

void VisionProject::displayThdFunc(void* para)
{
    VisionProject* pmainWin = (VisionProject*)para;

    //同一条码每帧都会识别出来，只有内容变了才写日志，否则刷屏
    static std::string s_lastBarcode;

    while (1) {
        if (pmainWin->m_bDispFinish) {
            return;
        }

        int index = pmainWin->m_cmb->currentIndex();//只处理当前选中的那一路

        cv::Mat img;
        {
            std::lock_guard<std::mutex> lk(pmainWin->m_mtxDisp);
            std::queue<cv::Mat>& q = pmainWin->m_mapImageData[index];
            if (q.empty()) {
                img = cv::Mat();
            }
            else {
                img = q.front();
                q.pop();
            }
        }
        if (img.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            continue;
        }

        //一维码识别
        try {
            BarCodeResData ret = pmainWin->m_imgProcess.barCodeIdentify(img);
            if (ret.hv_DecodedDataStrings.Length() > 0) {
                QString strCodes;
                for (int k = 0; k < ret.hv_DecodedDataStrings.Length(); k++) {
                    HalconCpp::HString hsCode = ret.hv_DecodedDataStrings[k];
                    strCodes += QString::fromLocal8Bit(hsCode.Text()) + " ";
                }
                strCodes = strCodes.trimmed();
                if (s_lastBarcode != strCodes.toStdString()) {
                    s_lastBarcode = strCodes.toStdString();
                    LOG_INFO(QString("相机%1 条码: %2").arg(index + 1).arg(strCodes));
                }
            }
        }
        catch (HalconCpp::HException& e) {
            std::cout << "条码识别异常: " << e.ErrorMessage().Text() << std::endl;
        }

        //显示到对应label（同样是投递到主线程，不在显示线程里直接碰UI）
        postShowImage(pmainWin, img, index);
    }
}
//新增：把图像投递到主线程显示。子线程（采集/显示线程）不能直接 setPixmap，
//      否则就是"跨线程操作UI"，轻则闪烁重则崩溃
void VisionProject::postShowImage(VisionProject* p, cv::Mat img, int cam) {
    if (p == nullptr || img.empty() || cam < 0 || cam >= 4) {
        return;
    }
    QMetaObject::invokeMethod(p, [p, img, cam]() {
        QLabel* labels[4] = { p->ui.label, p->ui.label_2, p->ui.label_3, p->ui.label_4 };
        QImage qimg;
        if (img.channels() > 1) {
            cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
            qimg = QImage(img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);
        }
        else {
            qimg = QImage(img.data, img.cols, img.rows, img.step, QImage::Format_Grayscale8);
        }
        QPixmap pixmap = QPixmap::fromImage(qimg);
        labels[cam]->setPixmap(pixmap.scaled(labels[cam]->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }, Qt::QueuedConnection);
}

//采集回调：本线程只负责"把图交出去"，真正的显示在主线程做
void VisionProject::grapImageCallbackFunc1(cv::Mat img, void* para)
{
    postShowImage((VisionProject*)para, img, 0);
}

void VisionProject::grapImageCallbackFunc2(cv::Mat img, void* para)
{
    postShowImage((VisionProject*)para, img, 1);
}

void VisionProject::grapImageCallbackFunc3(cv::Mat img, void* para)
{
    postShowImage((VisionProject*)para, img, 2);
}

void VisionProject::grapImageCallbackFunc4(cv::Mat img, void* para)
{
    postShowImage((VisionProject*)para, img, 3);
}

void VisionProject::on_pbtn_imageTool_clicked()
{
    ImageTool *imgTool=new ImageTool();
    imgTool->setOfflineFlages(m_chk_Off->isChecked());//是否被勾选离线模式
    imgTool->setMainWinPtr(this);//新增：把主窗口给它，用来读写XML参数
    imgTool->setAttribute(Qt::WA_DeleteOnClose);//新增：关窗口就销毁，析构里才会把参数存回XML
    //目录选了就顺便带一张图过去；没选也没关系，打开后可以在缩略图列表里自己选图
    if (!m_lineEdit->text().isEmpty()) {
        //新增：不能写死 color.png（目录里是 picture_1.png），按顺序找一个真实存在的文件
        QDir dir(m_lineEdit->text());
        QString imgPath;
        QStringList names; names << "picture_1.png" << "color.png";
        for (const QString& n : names) {
            QString p = dir.filePath(n);
            if (QFileInfo::exists(p)) { imgPath = p; break; }
        }
        if (imgPath.isEmpty()) {//还没有就取目录里第一张图片
            QStringList filters; filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
            QFileInfoList list = dir.entryInfoList(filters, QDir::Files, QDir::Name);
            if (!list.isEmpty()) {
                imgPath = list.first().absoluteFilePath();
            }
        }
        if (!imgPath.isEmpty()) {
            imgTool->setImagePath(imgPath.toLocal8Bit().toStdString());
            std::cout << "传给图像工具的首张图: " << imgPath.toStdString() << std::endl;
        }
    }
    imgTool->show();
}
//=================== 新增：socket的三个回调 ===================

//连接成功后：把账号密码发给服务器（JSON格式）
void VisionProject::connectServerCallbackFunc(void* p)
{
    VisionProject* pMainWin = (VisionProject*)p;
    std::cout << "服务建立链接成功" << std::endl;
    try
    {
        json jdata;                         //构造json对象
        jdata["acc"] = "work_ID_1";         //账号
        jdata["pwd"] = "9527";              //密码

        std::string str_jdata = jdata.dump();   //序列化成字符串
        str_jdata += "\r\n";                     //加换行，服务端按行切分
        int ret = pMainWin->m_socket.sendData(str_jdata.c_str());
        if (ret > 0) {
            std::cout << "[connectServerCallbackFunc] 发送登录信息" << std::endl
                      << str_jdata << std::endl;
        }
    }
    catch (json::exception& e)
    {
        std::string strExceInfo = e.what();
        std::cout << "JSON异常: " << strExceInfo << std::endl;
    }
}

//收到服务器数据：解析JSON
void VisionProject::recvCallbackFunc(void* p, char msg[])
{
    VisionProject* pMainWin = (VisionProject*)p;
    std::cout << "收到服务端数据：" << msg << std::endl;

    std::string strInfo = msg;
    if (strInfo.empty()) {
        return;
    }

    try
    {
        json jparse = json::parse(strInfo);   //字符串 -> json对象
        int nPort = jparse.at("serverPort");  //取字段（服务端下发的端口）
        std::cout << "服务端下发端口: " << nPort << std::endl;
    }
    catch (json::exception& e)
    {
        std::string strExceInfo = e.what();
        std::cout << "JSON解析异常: " << strExceInfo << std::endl;
    }
}

//心跳：每10秒发一次"我还活着"
void VisionProject::keepAliveCallbackFunc(void* p)
{
    VisionProject* pMainWin = (VisionProject*)p;
    try
    {
        json jdata;
        jdata["workstationID"] = 1;
        jdata["status"] = "alive";

        std::string str_jdata = jdata.dump();
        str_jdata += "\r\n";
        int ret = pMainWin->m_socket.sendData(str_jdata.c_str());
        if (ret > 0) {
            std::cout << "[keepAliveCallbackFunc] 发送心跳" << std::endl
                      << str_jdata << std::endl;
        }
    }
    catch (json::exception& e)
    {
        std::string strExceInfo = e.what();
        std::cout << "JSON异常: " << strExceInfo << std::endl;
    }
}
//=================== 新增：图像与Base64互转（传图用） ===================

//把一张Mat压成JPG，再编码成Base64字符串（可以直接塞进JSON）
std::string VisionProject::Mat2Base64_JPEG(const cv::Mat& img)
{
    std::vector<uchar> buf;
    cv::imencode(".jpg", img, buf);      //压缩成内存里的JPG
    Base64 objbase;
    return objbase.Encode(buf.data(), (int)buf.size());
}

//把Base64字符串还原成Mat（收到服务器图片时用）
cv::Mat VisionProject::Base2Mat(std::string& base64_data)
{
    Base64 objbase;
    std::string s_mat = objbase.Decode(base64_data.data(), (int)base64_data.size());
    std::vector<char> base64_img(s_mat.begin(), s_mat.end());
    cv::Mat img = cv::imdecode(base64_img, cv::IMREAD_COLOR);
    return img;
}