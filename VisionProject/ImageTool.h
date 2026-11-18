#pragma once
#include<QWidget>
#include"ui_ImageTool.h"
#include"DeviceManager.h"
#include"HalconCpp.h"
#include<QStringList>
using namespace HalconCpp;

class VisionProject;   //前置声明：这里只用到指针，不用包含整个头文件

class ImageTool :public QWidget
{
	Q_OBJECT
public:
	ImageTool(QWidget* parent = nullptr);
	~ImageTool();
public:
	//获取离线路径，离线路径从主窗口中获取
	void setImagePath(std::string filepath) {
		m_filenpath = filepath;
	}
	void setOfflineFlages(bool boffline) {
		m_bOffline = boffline;
	}
	//新增：接收主窗口指针，顺便把XML里的参数读到界面上
	void setMainWinPtr(VisionProject* p) {
		m_mainwin = p;
		updateXMLParaToUIParam();
		updateBarCodeXMLToUI();//新增：条码页的参数也从XML读
	}
private slots:
	void on_pbtn_getframe_clicked();//采集单帧图像	//新增：缩略图列表相关
	void on_pbtn_openImage_clicked();                      //选多张图片
	void on_listWidget_itemClicked(QListWidgetItem* item);   //点中某张缩略图
	//新增：形状匹配相关（按钮名和老师的一致）
	void on_pbtn_ROIChoice_clicked();      //选择模板ROI
	void on_pbtn_createmodel_clicked();    //创建模板
	void on_pbtn_inspectshape_clicked();   //预览模板
	void on_pbtn_loadmodel_clicked();      //加载模板
	void on_pbtn_matching_clicked();       //目标匹配
	//新增：条码识别页
	void on_pbtn_createbarcode_clicked();  //创建条码模型
	void on_pbtn_findbarcode_clicked();    //条码识别
	void on_pbtn_LoadBarCodeMod_clicked(); //加载条码模型
private:
	void updateListImage();                 //刷新缩略图
	void displayImage(const QString& file);  //读一张图并显示到label上
	void dispImageFit();                     //新增：按窗口比例把整幅图缩放到label里（保持长宽比，多余留黑边）
	void loadDefaultShapeModel();            //新增：启动时自动加载exe目录下的模板
protected:
	void resizeEvent(QResizeEvent* e) override;   //新增：窗口变大变小时重新适配
	void initParamDefault();                 //新增：给参数控件填默认值
	void updateXMLParaToUIParam();           //新增：读XML，把参数填到界面
	void updateUIParamToXMLPara();           //新增：把界面上的参数写回XML并保存
	void updateBarCodeXMLToUI();             //新增：读条码参数到界面
	void updateBarCodeUIToXML();             //新增：条码参数写回XML
private:
	Ui::ImageTool ui;
private:
	HObject ho_image;//存图像的
	HTuple hv_window;//halcon的窗口句柄，用haclon的组件绑定qt的label,在label上调用haclon的组件显示
	HTuple hv_width, hv_height;//窗口大小
	Hlong winId_Image;//图像窗口id
private:
	QStringList m_imagePaths;//新增：缩略图列表里所有图片的路径
private:
	HObject ho_cutGray;//新增：ROI抠出来的模板图像
	HTuple hv_shapeModel;//新增：形状模板数据
	QString m_shapeModelPath;//新增：当前形状模板文件路径（用于下次自动加载）
	HTuple hv_barCodeModel;//新增：一维码模型
	HTuple hv_window_bar;//新增：条码页那个label上的Halcon窗口句柄
	Hlong winId_bar;//新增：条码页label的窗口id
private:
	bool m_bOffline;//离线采集标志
	std::string m_filenpath;//文件路径
	VisionProject* m_mainwin = nullptr;//新增：主窗口指针，用来拿XML配置对象
	
};

