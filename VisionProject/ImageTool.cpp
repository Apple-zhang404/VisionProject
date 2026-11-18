#include "ImageTool.h"
#include <iostream>//调试输出用
#include <cstdlib>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include "VisionProject.h"//为了拿到主窗口的XML配置对象
#include "Logger.h"       //新增：统一日志

ImageTool::ImageTool(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    resize(1200, 780);   //新增：默认窗口开大一点，参数区不至于挤成一列
    initParamDefault();//新增：给形状匹配的参数控件填默认值
    loadDefaultShapeModel();//新增：自动加载exe目录下的模板，免得每次都要手动点"加载模板"
}

ImageTool::~ImageTool()
{
    //关闭窗口时，把界面上的参数写回XML
    updateBarCodeUIToXML();     //新增：条码参数先写进XML文档
    updateUIParamToXMLPara();   //再写模板/匹配参数，并在末尾统一存盘
}
void ImageTool::on_pbtn_getframe_clicked() {
    //新增：这个窗口本来就是拿本地图片做算法验证的，所以不管主界面勾没勾离线，
    //      只要没有路径就弹框让用户选一张，选完直接读出来显示
    //      （原来非离线模式下点这个按钮一点反应都没有）
    {
        if (m_filenpath.empty()) {
            QString file = QFileDialog::getOpenFileName(this, "选择一张图片", ".",
                "图片(*.png *.jpg *.jpeg *.bmp)");
            if (file.isEmpty()) {
                return;
            }
            m_filenpath = file.toLocal8Bit().toStdString();
        }
        try {
            //m_filenpath 现在是本地编码(GBK)，打印时转回来才看得懂中文
            std::cout << "准备读取: [" << QString::fromLocal8Bit(m_filenpath.c_str()).toStdString() << "]" << std::endl;
            ReadImage(&ho_image, m_filenpath.c_str());
            winId_Image = ui.lab_matching->winId();//获取label的窗口id
            if (hv_window.Length() == 0) {//新增：窗口只建一次，避免每次点都新建
                SetWindowAttr("background_color", "black");//设置下一个窗口的属性，必须在OpenWindow之前
                OpenWindow(0, 0, ui.lab_matching->width(),
                    ui.lab_matching->height(), winId_Image, "visible", "", &hv_window);
            }
            else {
                SetWindowExtents(hv_window, 0, 0, ui.lab_matching->width(), ui.lab_matching->height());
            }
            SetLineWidth(hv_window, 3);//设置划线的宽度为三
            ClearWindow(hv_window);
            dispImageFit();//新增：自适应显示
        }
        catch (HalconCpp::HException& e) {
            //Halcon出错是抛异常，不接住的话程序会直接崩掉
            appendLog(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())), true);
        }
    }
}

//新增：点"选择图片"按钮，一次可以选多张图
void ImageTool::on_pbtn_openImage_clicked() {
    QStringList files = QFileDialog::getOpenFileNames(this, "选择图片", ".",
        "图片(*.png *.jpg *.jpeg *.bmp)");
    if (files.isEmpty()) {
        return;
    }
    m_imagePaths = files;
    updateListImage();//把选中的图都做成缩略图
}

//新增：把 m_imagePaths 里的图片都做成缩略图，横着排在 listWidget 里
void ImageTool::updateListImage() {
    ui.listWidget->clear();
    ui.listWidget->setViewMode(QListView::IconMode);//图标模式，横向排列
    ui.listWidget->setResizeMode(QListWidget::Adjust);//窗口大小变了自动重排
    ui.listWidget->setMovement(QListWidget::Static);//不让拖动
    ui.listWidget->setIconSize(QSize(96, 96));//缩略图大小
    ui.listWidget->setSpacing(5);

    for (const QString& path : m_imagePaths) {
        //一条就是一个缩略图：图标用原图，文字用文件名
        QListWidgetItem* item = new QListWidgetItem(QIcon(path), QFileInfo(path).fileName());
        item->setSizeHint(QSize(100, 120));
        item->setTextAlignment(Qt::AlignHCenter);
        ui.listWidget->addItem(item);
    }
}

//新增：点中某张缩略图，就把它显示出来
void ImageTool::on_listWidget_itemClicked(QListWidgetItem* item) {
    if (!item) {
        return;
    }
    int index = ui.listWidget->row(item);
    if (index < 0 || index >= m_imagePaths.size()) {
        return;
    }
    displayImage(m_imagePaths.at(index));
}

//新增：读一张图，显示到label上（halcon窗口只创建一次，之后只改大小）
void ImageTool::displayImage(const QString& file) {
    try {
        std::cout << "选中图片: [" << file.toStdString() << "]" << std::endl;
        //Qt6 的 toStdString 是 UTF-8，Halcon 要本地编码(GBK)，中文路径必须用 toLocal8Bit
        QByteArray localPath = file.toLocal8Bit();
        m_filenpath = localPath.toStdString();   //新增：记住当前选中的图，采集单帧按钮就用它
        ReadImage(&ho_image, localPath.constData());

        winId_Image = ui.lab_matching->winId();//获取label的窗口id
        if (hv_window.Length() == 0) {//第一次才创建窗口
            SetWindowAttr("background_color", "black");
            OpenWindow(0, 0, ui.lab_matching->width(), ui.lab_matching->height(),
                winId_Image, "visible", "", &hv_window);
        }
        else {//已经有窗口了，只改大小，别重复创建
            SetWindowExtents(hv_window, 0, 0, ui.lab_matching->width(), ui.lab_matching->height());
        }
        SetLineWidth(hv_window, 3);
        ClearWindow(hv_window);
        dispImageFit();//新增：按窗口比例显示整幅图（原来是1:1，大图会被裁掉）
    }
    catch (HalconCpp::HException& e) {
        appendLog(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())), true);
    }
}

//新增：把整幅图按比例缩放到 Halcon 窗口里显示（保持长宽比，四周补黑边）
void ImageTool::dispImageFit() {
    if (!ho_image.IsInitialized() || hv_window.Length() == 0) {
        return;
    }
    int winW = ui.lab_matching->width();
    int winH = ui.lab_matching->height();
    if (winW <= 0 || winH <= 0) {
        return;
    }
    HTuple hvW, hvH;
    GetImageSize(ho_image, &hvW, &hvH);
    double imgW = hvW[0].D();
    double imgH = hvH[0].D();
    if (imgW <= 0 || imgH <= 0) {
        return;
    }
    //算出缩放比，取小的那个，保证整幅图都进得去
    double scale = std::min(winW / imgW, winH / imgH);
    double marginX = (winW - imgW * scale) / 2.0;   //左右黑边
    double marginY = (winH - imgH * scale) / 2.0;   //上下黑边
    //把黑边折算回图像坐标，SetPart 的参数是"显示图像的哪一块"
    SetPart(hv_window,
        -marginY / scale, -marginX / scale,
        imgH - 1 + marginY / scale, imgW - 1 + marginX / scale);
    DispObj(ho_image, hv_window);
}

//新增：窗口大小变化时重新适配显示
void ImageTool::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (hv_window.Length() == 0 || ui.lab_matching->width() <= 0) {
        return;
    }
    try {
        SetWindowExtents(hv_window, 0, 0, ui.lab_matching->width(), ui.lab_matching->height());
        if (ho_image.IsInitialized()) {
            ClearWindow(hv_window);
            dispImageFit();
        }
    }
    catch (HalconCpp::HException&) {
    }
}

//=================== 下面都是新增：形状匹配 ===================

//新增：选择模板ROI —— 在图像上拉一个矩形，把这块区域抠出来当模板
void ImageTool::on_pbtn_ROIChoice_clicked() {
    try {
        if (ho_image.IsInitialized() == false) {
            std::cout << "还没有图像，先采集一张" << std::endl;
            return;
        }
        DispObj(ho_image, hv_window);//先把原图显示出来
        SetDraw(hv_window, "margin");//画边框的方式
        SetLineWidth(hv_window, 3);//线宽
        SetColor(hv_window, "blue");//颜色

        HObject ho_gray;
        HTuple hv_row1, hv_column1, hv_row2, hv_column2;
        HObject ho_Rectangle;
        Rgb1ToGray(ho_image, &ho_gray);//转灰度
        DrawRectangle1(hv_window, &hv_row1, &hv_column1, &hv_row2, &hv_column2);//鼠标拉框
        GenRectangle1(&ho_Rectangle, hv_row1, hv_column1, hv_row2, hv_column2);//生成矩形区域
        DispObj(ho_Rectangle, hv_window);
        ReduceDomain(ho_gray, ho_Rectangle, &ho_cutGray);//把矩形里的图像抠出来
        std::cout << "ROI 选择完成" << std::endl;
    }
    catch (HalconCpp::HException& e) {
        appendLog(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())), true);
    }
}

//新增：创建模板 —— 用ROI抠出来的图生成形状模板，并存成 shapeModel1.mod
void ImageTool::on_pbtn_createmodel_clicked() {
    try {
        if (ho_cutGray.IsInitialized() == false) {
            std::cout << "还没有选ROI，先点[选择模板ROI]" << std::endl;
            return;
        }
        //从界面读参数
        HTuple hv_numlevels;
        if (ui.cmb_numlevels->currentText() == "auto") {
            hv_numlevels = "auto";
        }
        else {
            hv_numlevels = ui.cmb_numlevels->currentText().toInt();
        }
        HTuple hv_angleStart = ui.dspin_anglestart->value();
        HTuple hv_angleExtent = ui.dspin_extent->value();
        HTuple hv_angleStep = ui.dspin_anglestep->value();
        HTuple hv_optimization = ui.cmb_optimization->currentText().toStdString().c_str();
        HTuple hv_metric = ui.cmb_metric->currentText().toStdString().c_str();
        HTuple hv_contrast = ui.cmb_constrast->currentText().toStdString().c_str();
        HTuple hv_minContrast = ui.lineEdit_mincontrast->text().toDouble();

        CreateShapeModel(ho_cutGray, hv_numlevels, hv_angleStart, hv_angleExtent, hv_angleStep,
            hv_optimization, hv_metric, hv_contrast, hv_minContrast, &hv_shapeModel);

        //新增：让用户自己选保存位置和文件名，这样可以存多个模板，不用互相覆盖
        QString savePath = QFileDialog::getSaveFileName(this, "保存模板",
            QDir::current().filePath("shapeModel1.mod"), "模板文件(*.mod)");
        if (savePath.isEmpty()) {
            appendLog("已创建模板，但没保存（取消了保存对话框）");
            return;
        }
        QByteArray localSave = savePath.toLocal8Bit();
        WriteShapeModel(hv_shapeModel, localSave.constData());
        m_shapeModelPath = savePath;   //记下路径，下次打开自动加载
        appendLog("创建模板成功，已保存: " + savePath);
    }
    catch (HalconCpp::HException& e) {
        appendLog(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())), true);
        appendLog("创建模板失败");
    }
}

//新增：预览模板 —— 看看模板分了几层、哪些边缘被用上了
void ImageTool::on_pbtn_inspectshape_clicked() {
    try {
        if (ho_cutGray.IsInitialized() == false) {
            std::cout << "还没有选ROI" << std::endl;
            return;
        }
        HObject ho_shapeModelImages;
        HObject ho_shapeModelRegions;
        InspectShapeModel(ho_cutGray, &ho_shapeModelImages, &ho_shapeModelRegions, 2, 50);
        DispObj(ho_shapeModelRegions, hv_window);//把模板边缘画出来
    }
    catch (HalconCpp::HException& e) {
        appendLog(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())), true);
    }
}

//新增：加载模板 —— 从 .mod 文件读一个已经做好的模板
void ImageTool::on_pbtn_loadmodel_clicked() {
    try {
        hv_shapeModel.Clear();//读之前先清空
        QString modelfile = QFileDialog::getOpenFileName(this, "选择模板文件", "/", "模板文件(*.mod)");
        if (modelfile.isEmpty()) {
            QMessageBox::warning(this, "警告", "文件路径为空");
            return;
        }
        //Qt6 的 toStdString 是 UTF-8，Halcon 要本地编码(GBK)
        QByteArray localModel = modelfile.toLocal8Bit();
        ReadShapeModel(localModel.constData(), &hv_shapeModel);
        if (hv_shapeModel.Length() == 0) {
            QMessageBox::warning(this, "警告", "模板数据为空");
            return;
        }
        m_shapeModelPath = modelfile;
        appendLog("加载模板成功: " + modelfile);
    }
    catch (HalconCpp::HException& e) {
        appendLog(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())), true);
    }
}

//新增：目标匹配 —— 在当前图里找模板，画出位置并把分数打出来
void ImageTool::on_pbtn_matching_clicked() {
    try {
        if (hv_shapeModel.Length() == 0) {
            std::cout << "还没有模板，先创建或加载一个" << std::endl;
            return;
        }
        HTuple hv_minScore = ui.dspin_minscore->value();
        HTuple hv_numMatches = (int)ui.spin_nummatches->value();
        HTuple hv_maxOverlap = ui.dspin_maxoverlap->value();
        HTuple hv_subPixel = ui.cmb_subpixel->currentText().toStdString().c_str();
        HTuple hv_numLevels;
        if (ui.cmb_numlevels_2->currentText() == "auto") {
            //FindShapeModel 的 NumLevels 只认整数，0 表示用模板的全部层；传"auto"会报 #1208
            hv_numLevels = 0;
        }
        else {
            hv_numLevels = ui.cmb_numlevels_2->currentText().toInt();
        }
        HTuple hv_greediness = ui.dspin_greediness->value();
        HTuple hv_angleStart = ui.dspin_anglestart->value();
        HTuple hv_angleExtent = ui.dspin_extent->value();

        HTuple hv_row, hv_column, hv_angle, hv_score;
        HObject ho_gray;
        Rgb1ToGray(ho_image, &ho_gray);

        FindShapeModel(ho_gray, hv_shapeModel, hv_angleStart, hv_angleExtent, hv_minScore,
            hv_numMatches, hv_maxOverlap, hv_subPixel, hv_numLevels, hv_greediness,
            &hv_row, &hv_column, &hv_angle, &hv_score);

        if (hv_score.Length() == 0) {
            appendLog("没有匹配到模板");
            return;
        }
        //把每处匹配用矩形框出来
        SetDraw(hv_window, "margin");
        SetLineWidth(hv_window, 3);
        SetColor(hv_window, "green");
        DispObj(ho_image, hv_window);
        for (int i = 0; i < hv_score.Length(); i++) {
            HObject ho_objRectangle;
            GenRectangle1(&ho_objRectangle, hv_row[i] - 20, hv_column[i] - 20,
                hv_row[i] + 20, hv_column[i] + 20);
            DispObj(ho_objRectangle, hv_window);
            QString str = QString("匹配 %1: score=%2  row=%3  col=%4  angle=%5")
                .arg(i + 1)
                .arg(hv_score[i].D())
                .arg(hv_row[i].D())
                .arg(hv_column[i].D())
                .arg(hv_angle[i].D());
            appendLog(str);
        }
        std::cout << "匹配到 " << hv_score.Length() << " 个目标" << std::endl;
    }
    catch (HalconCpp::HException& e) {
        appendLog(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())), true);
    }
}

//新增：给形状匹配的参数控件填默认值（HALCON 常用默认，角度单位是弧度）
void ImageTool::initParamDefault() {
    //---------------- 模板参数 ShapeModelPara ----------------
    //NumLevels：auto 或者 1~8 的层数
    ui.cmb_numlevels->clear();
    ui.cmb_numlevels->addItems({ "auto","1","2","3","4","5","6","7","8" });
    ui.cmb_numlevels->setCurrentText("auto");

    //AngleStart：模板起始角度，弧度。0.39≈22.5°，这里给 -0.39
    ui.dspin_anglestart->setDecimals(4);
    ui.dspin_anglestart->setRange(-6.2832, 6.2832);
    ui.dspin_anglestart->setSingleStep(0.0175);//1° ≈ 0.0175 弧度
    ui.dspin_anglestart->setValue(-0.39);

    //AngleExtent：角度范围，0.79≈45°
    ui.dspin_extent->setDecimals(4);
    ui.dspin_extent->setRange(0, 6.2832);
    ui.dspin_extent->setSingleStep(0.0175);
    ui.dspin_extent->setValue(0.79);

    //AngleStep：角度步长，必须大于0，否则就是错误码1305
    ui.dspin_anglestep->setDecimals(4);
    ui.dspin_anglestep->setRange(0.001, 1);
    ui.dspin_anglestep->setSingleStep(0.005);
    ui.dspin_anglestep->setValue(0.0175);//1°一步

    //Optimization
    ui.cmb_optimization->clear();
    ui.cmb_optimization->addItems({ "auto","none","point_reduction_low","point_reduction_medium","point_reduction_high" });
    ui.cmb_optimization->setCurrentText("auto");

    //Metric：目标有没有极性变化（黑白反转）就用哪个
    ui.cmb_metric->clear();
    ui.cmb_metric->addItems({ "use_polarity","ignore_global_polarity","ignore_local_polarity","ignore_color_polarity" });
    ui.cmb_metric->setCurrentText("use_polarity");

    //Contrast
    ui.cmb_constrast->clear();
    ui.cmb_constrast->addItems({ "auto","auto_contrast","auto_contrast_hyst","auto_contrast_very_hyst" });
    ui.cmb_constrast->setCurrentText("auto");

    //MinContrast：绝对对比度阈值
    ui.lineEdit_mincontrast->setText("10");

    //---------------- 匹配参数 FindShapePara ----------------
    //MinScore：最低分数
    ui.dspin_minscore->setDecimals(3);
    ui.dspin_minscore->setRange(0, 1);
    ui.dspin_minscore->setSingleStep(0.05);
    ui.dspin_minscore->setValue(0.5);

    //NumMatches：最多找几个
    ui.spin_nummatches->setDecimals(0);
    ui.spin_nummatches->setRange(1, 100);
    ui.spin_nummatches->setValue(1);

    //MaxOverlap：多个目标允许的重叠比例
    ui.dspin_maxoverlap->setDecimals(3);
    ui.dspin_maxoverlap->setRange(0, 1);
    ui.dspin_maxoverlap->setSingleStep(0.05);
    ui.dspin_maxoverlap->setValue(0.5);

    //SubPixel：亚像素精度
    ui.cmb_subpixel->clear();
    ui.cmb_subpixel->addItems({ "least_squares","interpolation","none" });
    ui.cmb_subpixel->setCurrentText("least_squares");

    //NumLevels(匹配用)
    ui.cmb_numlevels_2->clear();
    ui.cmb_numlevels_2->addItems({ "auto","1","2","3","4","5","6","7","8" });
    ui.cmb_numlevels_2->setCurrentText("auto");

    //Greediness：0最稳但慢，1最快但可能漏，常用0.9
    ui.dspin_greediness->setDecimals(3);
    ui.dspin_greediness->setRange(0, 1);
    ui.dspin_greediness->setSingleStep(0.05);
    ui.dspin_greediness->setValue(0.9);
}

//新增：从XML读参数，填到界面控件上（打开窗口时调用）
void ImageTool::updateXMLParaToUIParam() {
    if (m_mainwin == nullptr || m_mainwin->getXMLPtr() == nullptr) {
        return;
    }
    XMlConfigOperator* doc = m_mainwin->getXMLPtr();
    std::string v;

    //---------------- 模板参数 ShapeModelPara ----------------
    v = doc->GetNodeParam("NumLevels");
    if (!v.empty()) { ui.cmb_numlevels->setCurrentText(QString::fromStdString(v)); }

    v = doc->GetNodeParam("AngleStart");
    if (!v.empty()) { ui.dspin_anglestart->setValue(atof(v.c_str())); }

    v = doc->GetNodeParam("AngleExtent");
    if (!v.empty()) { ui.dspin_extent->setValue(atof(v.c_str())); }

    v = doc->GetNodeParam("AngleStep");
    if (!v.empty() && atof(v.c_str()) > 0) {//XML里是0，0会报#1305，所以为0时保留默认值
        ui.dspin_anglestep->setValue(atof(v.c_str()));
    }

    v = doc->GetNodeParam("Optimization");
    if (!v.empty()) { ui.cmb_optimization->setCurrentText(QString::fromStdString(v)); }

    v = doc->GetNodeParam("Metric");
    if (!v.empty()) { ui.cmb_metric->setCurrentText(QString::fromStdString(v)); }

    v = doc->GetNodeParam("Contrast");
    if (!v.empty()) { ui.cmb_constrast->setCurrentText(QString::fromStdString(v)); }

    v = doc->GetNodeParam("MinContrast");
    if (!v.empty()) { ui.lineEdit_mincontrast->setText(QString::fromStdString(v)); }

    //---------------- 匹配参数 FindShapePara ----------------
    v = doc->GetFindParam("MinScore");
    if (!v.empty()) { ui.dspin_minscore->setValue(atof(v.c_str())); }

    v = doc->GetFindParam("NumMatches");
    if (!v.empty()) { ui.spin_nummatches->setValue(atof(v.c_str())); }

    v = doc->GetFindParam("MaxOverlap");
    if (!v.empty()) { ui.dspin_maxoverlap->setValue(atof(v.c_str())); }

    v = doc->GetFindParam("SubPixel");
    if (!v.empty()) { ui.cmb_subpixel->setCurrentText(QString::fromStdString(v)); }

    v = doc->GetFindParam("NumLevels_2");
    if (!v.empty()) { ui.cmb_numlevels_2->setCurrentText(QString::fromStdString(v)); }

    v = doc->GetFindParam("Greediness");
    if (!v.empty()) { ui.dspin_greediness->setValue(atof(v.c_str())); }
}

//新增：把界面上的参数写回XML并保存（关闭窗口时调用）
void ImageTool::updateUIParamToXMLPara() {
    if (m_mainwin == nullptr || m_mainwin->getXMLPtr() == nullptr) {
        return;
    }
    XMlConfigOperator* doc = m_mainwin->getXMLPtr();

    //---------------- 模板参数 ----------------
    doc->UpdateShapeMatchingXMLNodeValue("ShapeModelPara", "NumLevels", ui.cmb_numlevels->currentText().toStdString());
    doc->UpdateShapeMatchingXMLNodeValue("ShapeModelPara", "AngleStart", std::to_string(ui.dspin_anglestart->value()));
    doc->UpdateShapeMatchingXMLNodeValue("ShapeModelPara", "AngleExtent", std::to_string(ui.dspin_extent->value()));
    doc->UpdateShapeMatchingXMLNodeValue("ShapeModelPara", "AngleStep", std::to_string(ui.dspin_anglestep->value()));
    doc->UpdateShapeMatchingXMLNodeValue("ShapeModelPara", "Optimization", ui.cmb_optimization->currentText().toStdString());
    doc->UpdateShapeMatchingXMLNodeValue("ShapeModelPara", "Metric", ui.cmb_metric->currentText().toStdString());
    doc->UpdateShapeMatchingXMLNodeValue("ShapeModelPara", "Contrast", ui.cmb_constrast->currentText().toStdString());
    doc->UpdateShapeMatchingXMLNodeValue("ShapeModelPara", "MinContrast", ui.lineEdit_mincontrast->text().toStdString());

    //---------------- 匹配参数 ----------------
    doc->UpdateShapeMatchingXMLNodeValue("FindShapePara", "MinScore", std::to_string(ui.dspin_minscore->value()));
    doc->UpdateShapeMatchingXMLNodeValue("FindShapePara", "NumMatches", std::to_string((int)ui.spin_nummatches->value()));
    doc->UpdateShapeMatchingXMLNodeValue("FindShapePara", "MaxOverlap", std::to_string(ui.dspin_maxoverlap->value()));
    doc->UpdateShapeMatchingXMLNodeValue("FindShapePara", "SubPixel", ui.cmb_subpixel->currentText().toStdString());
    doc->UpdateShapeMatchingXMLNodeValue("FindShapePara", "NumLevels_2", ui.cmb_numlevels_2->currentText().toStdString());
    doc->UpdateShapeMatchingXMLNodeValue("FindShapePara", "Greediness", std::to_string(ui.dspin_greediness->value()));

    if (!doc->SaveXml()) {
        std::cout << "XML保存失败" << std::endl;
    }
}
//=================== 新增：条码识别页 ===================

//新增：创建条码模型，按界面的勾选项设参数，然后写到 barCodeModel.bar
void ImageTool::on_pbtn_createbarcode_clicked() {
    try {
        CreateBarCodeModel(HTuple(), HTuple(), &hv_barCodeModel);

        if (ui.chx_defaultpara->isChecked()) {//不可解码时停止
            SetBarCodeParam(hv_barCodeModel, "abort", ui.cmb_defaultpara->currentText().toStdString().c_str());
        }
        if (ui.chx_angle->isChecked()) {//角度方向/容差
            SetBarCodeParam(hv_barCodeModel, "orientation", atof(ui.edit_startangle->text().toStdString().c_str()));
            SetBarCodeParam(hv_barCodeModel, "orientation_tol", atof(ui.edit_endangle->text().toStdString().c_str()));
        }
        if (ui.chx_minscanlies->isChecked()) {//最小一致扫描线数
            SetBarCodeParam(hv_barCodeModel, "min_identical_scanlines", atoi(ui.edit_minscanlines->text().toStdString().c_str()));
        }
        if (ui.chx_minsize->isChecked()) {//最小模块宽度
            SetBarCodeParam(hv_barCodeModel, "element_size_min", atof(ui.edit_minsize->text().toStdString().c_str()));
        }
        if (ui.chx_checked->isChecked()) {//启用校验位
            SetBarCodeParam(hv_barCodeModel, "check_char", "present");
        }
        if (ui.chx_persistance->isChecked()) {//模型持久化
            SetBarCodeParam(hv_barCodeModel, "persistence", 1);
        }
        if (ui.chx_stopafterscan->isChecked()) {//提前终止扫描
            SetBarCodeParam(hv_barCodeModel, "stop_after_result_num", 1);
        }
        if (ui.chx_numscanlines->isChecked()) {//扫描线数
            SetBarCodeParam(hv_barCodeModel, "num_scanlines", atoi(ui.edit_numscanlines->text().toStdString().c_str()));
        }
        if (ui.chx_measthresh->isChecked()) {//对比度阈值
            SetBarCodeParam(hv_barCodeModel, "meas_thresh", atof(ui.edit_measthresh->text().toStdString().c_str()));
        }
        //module_gap_min 有的版本不支持，老师也是注释掉的，这里不设

        if (m_mainwin != nullptr) {//把新模型交给算法层
            m_mainwin->getImgProcPtr()->updateBarCodeModel(hv_barCodeModel);
        }
        WriteBarCodeModel(hv_barCodeModel, "barCodeModel.bar");
        std::cout << "条码模型创建成功，已保存 barCodeModel.bar" << std::endl;
    }
    catch (HalconCpp::HException& e) {
        appendLog(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())), true);
    }
}

//新增：在当前图像上找条码，框出来并打印内容
void ImageTool::on_pbtn_findbarcode_clicked() {
    try {
        if (!ho_image.IsInitialized()) {
            std::cout << "还没有图像，先采集一张" << std::endl;
            return;
        }
        if (hv_barCodeModel.Length() == 0) {
            std::cout << "还没有条码模型，先点[创建条码模型]或[加载模型]" << std::endl;
            return;
        }

        HTuple hv_codeType = ui.cmb_codetype->currentText().toStdString().c_str();
        HObject ho_search = ho_image;           //先拿原图试（HObject是引用计数句柄，赋值为浅拷贝）
        HObject ho_symbolRegions;
        HTuple hv_decodedDataStrings;

        FindBarCode(ho_search, &ho_symbolRegions, hv_barCodeModel, hv_codeType, &hv_decodedDataStrings);

        //新增：原图读不出来时，用OpenCV把图旋转 90/180/270 各试一次（竖着贴的条码也能读）
        //注意：不能用Halcon的RotateImage，它保持原尺寸会把图裁掉，转完更读不出来
        int usedRotate = 0;
        if (hv_decodedDataStrings.Length() == 0 && m_mainwin != nullptr) {
            cv::Mat matSrc, matRot;
            m_mainwin->getImgProcPtr()->hobjectToMat(ho_image, matSrc);
            if (!matSrc.empty()) {
                for (int k = 1; k <= 3 && hv_decodedDataStrings.Length() == 0; k++) {
                    if (k == 1)      cv::rotate(matSrc, matRot, cv::ROTATE_90_CLOCKWISE);
                    else if (k == 2) cv::rotate(matSrc, matRot, cv::ROTATE_180);
                    else             cv::rotate(matSrc, matRot, cv::ROTATE_90_COUNTERCLOCKWISE);

                    HObject hoRot;
                    m_mainwin->getImgProcPtr()->matToHobject(matRot, hoRot);
                    FindBarCode(hoRot, &ho_symbolRegions, hv_barCodeModel, hv_codeType, &hv_decodedDataStrings);
                    if (hv_decodedDataStrings.Length() > 0) {
                        ho_search = hoRot;
                        usedRotate = k * 90;
                    }
                }
            }
        }

        if (hv_decodedDataStrings.Length() == 0) {
            std::cout << "没识别到条码" << std::endl;
            ui.plnTextEdit_barcode->setPlainText("没识别到条码（原图 + 旋转90/180/270 都试过了）");
            return;
        }

        //显示到"条码识别"页的label上（旋转过的就显示旋转后的图，框才对得上）
        winId_bar = ui.label->winId();
        if (hv_window_bar.Length() == 0) {
            SetWindowAttr("background_color", "black");
            OpenWindow(0, 0, ui.label->width(), ui.label->height(), winId_bar, "visible", "", &hv_window_bar);
        }
        else {
            SetWindowExtents(hv_window_bar, 0, 0, ui.label->width(), ui.label->height());
        }
        DispObj(ho_search, hv_window_bar);
        SetDraw(hv_window_bar, "margin");
        SetLineWidth(hv_window_bar, 3);
        SetColor(hv_window_bar, "blue");
        DispObj(ho_symbolRegions, hv_window_bar);

        //新增：结果写到条码页的文本框里
        QString strResult;
        strResult += QString("识别到 %1 个条码").arg((int)hv_decodedDataStrings.Length());
        if (usedRotate != 0) {
            strResult += QString("（原图失败，旋转 %1° 后成功）").arg(usedRotate);
        }
        strResult += "\n";
        for (int i = 0; i < hv_decodedDataStrings.Length(); i++) {
            HalconCpp::HString hsCode = hv_decodedDataStrings[i];
            std::string strCode = hsCode.Text();
            strResult += QString("[%1] %2\n").arg(i + 1).arg(QString::fromLocal8Bit(strCode.c_str()));
            std::cout << "条码: " << strCode << std::endl;
        }
        strResult += QString("码制: %1").arg(ui.cmb_codetype->currentText());
        ui.plnTextEdit_barcode->setPlainText(strResult);
    }
    catch (HalconCpp::HException& e) {
        appendLog(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())), true);
        ui.plnTextEdit_barcode->setPlainText(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())));
    }
}
//新增：从文件加载条码模型
void ImageTool::on_pbtn_LoadBarCodeMod_clicked() {
    try {
        hv_barCodeModel.Clear();
        QString modefile = QFileDialog::getOpenFileName(this, "选择条码模型", "/", "模型文件(*.bar *.mod)");
        if (modefile.isEmpty()) {
            QMessageBox::warning(this, "警告", "文件路径为空");
            return;
        }
        QByteArray localModel = modefile.toLocal8Bit();//Halcon要本地编码
        ReadBarCodeModel(localModel.constData(), &hv_barCodeModel);
        if (hv_barCodeModel.Length() == 0) {
            QMessageBox::warning(this, "警告", "模型数据为空");
            return;
        }
        if (m_mainwin != nullptr) {
            m_mainwin->getImgProcPtr()->updateBarCodeModel(hv_barCodeModel);
        }
        std::cout << "条码模型加载成功: " << modefile.toStdString() << std::endl;
    }
    catch (HalconCpp::HException& e) {
        appendLog(QString("HALCON错误: %1").arg(QString::fromLocal8Bit(e.ErrorMessage().Text())), true);
    }
}

//新增：XML里的 BarCode 参数 → 界面控件
void ImageTool::updateBarCodeXMLToUI() {
    if (m_mainwin == nullptr || m_mainwin->getXMLPtr() == nullptr) {
        return;
    }
    XMlConfigOperator* doc = m_mainwin->getXMLPtr();
    std::string v;

    v = doc->GetNodeParam("abort");
    if (!v.empty()) {
        ui.cmb_defaultpara->setCurrentText(QString::fromStdString(v));
        ui.chx_defaultpara->setChecked(v == "true");
    }
    v = doc->GetNodeParam("orientation");
    if (!v.empty()) {
        ui.edit_startangle->setText(QString::fromStdString(v));
        ui.chx_angle->setChecked(true);
    }
    v = doc->GetNodeParam("orientation_tol");
    if (!v.empty()) { ui.edit_endangle->setText(QString::fromStdString(v)); }

    v = doc->GetNodeParam("min_identical_scanlines");
    if (!v.empty()) {
        ui.edit_minscanlines->setText(QString::fromStdString(v));
        ui.chx_minscanlies->setChecked(true);
    }
    v = doc->GetNodeParam("element_size_min");
    if (!v.empty()) {
        ui.edit_minsize->setText(QString::fromStdString(v));
        ui.chx_minsize->setChecked(true);
    }
    v = doc->GetNodeParam("check_char");
    if (!v.empty()) { ui.chx_checked->setChecked(v == "true" || v == "present"); }

    v = doc->GetNodeParam("persistence");
    if (!v.empty()) { ui.chx_persistance->setChecked(v == "1"); }

    v = doc->GetNodeParam("stop_after_result_num");
    if (!v.empty()) { ui.chx_stopafterscan->setChecked(v == "1"); }

    v = doc->GetNodeParam("num_scanlines");
    if (!v.empty()) {
        ui.edit_numscanlines->setText(QString::fromStdString(v));
        ui.chx_numscanlines->setChecked(true);
    }
    v = doc->GetNodeParam("meas_thresh");
    if (!v.empty()) {
        ui.edit_measthresh->setText(QString::fromStdString(v));
        ui.chx_measthresh->setChecked(true);
    }
    v = doc->GetNodeParam("codeType");
    if (!v.empty()) { ui.cmb_codetype->setCurrentText(QString::fromStdString(v)); }
}

//新增：界面的条码参数 → XML（只改内存里的文档，存盘交给调用者）
void ImageTool::updateBarCodeUIToXML() {
    if (m_mainwin == nullptr || m_mainwin->getXMLPtr() == nullptr) {
        return;
    }
    XMlConfigOperator* doc = m_mainwin->getXMLPtr();

    doc->UpdateXMLCodeNodeValue("BarCode", "abort", ui.cmb_defaultpara->currentText().toStdString());
    doc->UpdateXMLCodeNodeValue("BarCode", "orientation", ui.edit_startangle->text().toStdString());
    doc->UpdateXMLCodeNodeValue("BarCode", "orientation_tol", ui.edit_endangle->text().toStdString());
    doc->UpdateXMLCodeNodeValue("BarCode", "min_identical_scanlines", ui.edit_minscanlines->text().toStdString());
    doc->UpdateXMLCodeNodeValue("BarCode", "element_size_min", ui.edit_minsize->text().toStdString());
    doc->UpdateXMLCodeNodeValue("BarCode", "check_char", (ui.chx_checked->isChecked() ? "true" : "false"));
    doc->UpdateXMLCodeNodeValue("BarCode", "persistence", (ui.chx_persistance->isChecked() ? "1" : "0"));
    doc->UpdateXMLCodeNodeValue("BarCode", "stop_after_result_num", (ui.chx_stopafterscan->isChecked() ? "1" : "0"));
    doc->UpdateXMLCodeNodeValue("BarCode", "num_scanlines", ui.edit_numscanlines->text().toStdString());
    doc->UpdateXMLCodeNodeValue("BarCode", "meas_thresh", ui.edit_measthresh->text().toStdString());
    doc->UpdateXMLCodeNodeValue("BarCode", "codeType", ui.cmb_codetype->currentText().toStdString());
}
//新增：启动时自动加载模板。优先用上次记住的路径，没有就用exe目录下的 shapeModel1.mod
void ImageTool::loadDefaultShapeModel() {
    QString path = m_shapeModelPath;
    if (path.isEmpty()) {
        QString def = QDir::current().filePath("shapeModel1.mod");
        if (QFileInfo::exists(def)) {
            path = def;
        }
    }
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return;
    }
    try {
        QByteArray localPath = path.toLocal8Bit();
        ReadShapeModel(localPath.constData(), &hv_shapeModel);
        m_shapeModelPath = path;
        std::cout << "已自动加载模板: " << path.toStdString() << std::endl;
        appendLog("已自动加载模板: " + path);
    }
    catch (HalconCpp::HException& e) {
        std::cout << "自动加载模板失败: " << e.ErrorMessage().Text() << std::endl;
    }
}
//新增：工具窗口自己的日志入口
//  1) 追加到本窗口的结果框（用户能看见）
//  2) 同时写进全局 Logger（文件 + 主界面日志框）
void ImageTool::appendLog(const QString& msg, bool isError)
{
    ui.plnTextEdit_result->appendPlainText(msg);
    if (isError) {
        LOG_ERROR(msg);
    }
    else {
        LOG_INFO(msg);
    }
}