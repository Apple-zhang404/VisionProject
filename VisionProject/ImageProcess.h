#pragma once
#include <opencv2/opencv.hpp>
#include "HalconCpp.h"
#include <QString>
#include <string>

//一维码识别算法参数
typedef struct st_BarcodeIdyParam {
    short st_IMinIdenScanLine;      //最小一致扫描线数
    short st_IPersistence;          //模型持久化
    short st_Istopscan;             //提前终止扫描
    short st_Inumscanlines;         //高精度检测扫描线数
    float st_fstartAngle;           //角度方向
    float st_fendAngle;             //方向容差
    float st_felemSizeMin;          //最小模块宽度
    float st_fmeasthresh;           //条码对比度阈值
    QString st_strCheckChar;        //启用校验位
    QString st_strCodeType;         //条码类型
    QString st_strAbort;            //不可解码时停止
    st_BarcodeIdyParam() {
        st_IMinIdenScanLine = 3;
        st_IPersistence = 1;
        st_Istopscan = 1;
        st_Inumscanlines = 10;
        st_fstartAngle = 0;
        st_fendAngle = 15;
        st_felemSizeMin = 2;
        st_fmeasthresh = 0.5;
        st_strCheckChar = "present";
        st_strAbort = "true";
        st_strCodeType = "auto";
    }
}sBCIParam;

//一维码识别结果数据
typedef struct st_BarCodeResultData {
    HalconCpp::HObject ho_SymbolRegions;
    HalconCpp::HTuple hv_DecodedDataStrings;
}BarCodeResData;

//新增：算法层。界面只负责调这里，不再直接写Halcon算子
class ImageProcess
{
private:
    HalconCpp::HTuple hv_objectMatchingMod;   //形状匹配模板（以后用）
    HalconCpp::HTuple hv_barCodeMod;          //一维码模型
    sBCIParam m_barCodeParam;                 //一维码参数
    BarCodeResData m_barCodeResult;           //识别结果（用成员存，避免返回局部变量的引用）
public:
    ImageProcess() {
    }
    ~ImageProcess() {
    }

    void objectMatching(cv::Mat& image);                 //图像目标匹配
    BarCodeResData& barCodeIdentify(cv::Mat& image);     //一维码识别
    void readBarCodeModel(std::string model);            //读条码模型文件
    void readObjectMatchingModel();                      //读形状匹配模型

    void setBarCodeParam(sBCIParam& barCodeParam) {
        m_barCodeParam = barCodeParam;
    }
    void updateBarCodeModel(HalconCpp::HTuple& barCodeModel) {
        hv_barCodeMod = barCodeModel;
    }
    HalconCpp::HTuple& getBarCodeModel() {
        return hv_barCodeMod;
    }
public:
    //新增：Mat 和 HObject 互转（旋转这类OpenCV擅长的事要用到）
    void matToHobject(cv::Mat& Img, HalconCpp::HObject& hvImg);        //Mat -> HObject
    void hobjectToMat(const HalconCpp::HObject& hoImg, cv::Mat& Img);  //HObject -> Mat
private:
};
