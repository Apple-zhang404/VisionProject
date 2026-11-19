#include "ImageProcess.h"
#include <iostream>
#include <stdexcept>

//图像目标匹配（先留空，等跟到对应进度再补，老师0130这里也是空的）
void ImageProcess::objectMatching(cv::Mat& image)
{
}

//一维码识别。把cv::Mat转成Halcon的HObject，再调FindBarCode
BarCodeResData& ImageProcess::barCodeIdentify(cv::Mat& image)
{
    //每次识别前先清空上一次的结果
    m_barCodeResult.ho_SymbolRegions = HalconCpp::HObject();
    m_barCodeResult.hv_DecodedDataStrings = HalconCpp::HTuple();

    try
    {
        HalconCpp::HObject symbolRegions = HalconCpp::HObject();
        HalconCpp::HTuple decodingDatas = HalconCpp::HTuple();

        if (image.empty()) {
            throw std::runtime_error("输入图像为空");
        }

        HalconCpp::HObject img;
        matToHobject(image, img);//cv::Mat -> HObject
        if (!img.IsInitialized()) {
            throw std::runtime_error("图像转换失败");
        }
        if (hv_barCodeMod.Length() == 0) {
            throw std::runtime_error("条码模型句柄无效，检查barCodeModel.bar是否加载成功");
        }

        HalconCpp::FindBarCode(img, &symbolRegions, hv_barCodeMod,
            m_barCodeParam.st_strCodeType.toStdString().c_str(), &decodingDatas);

        m_barCodeResult.ho_SymbolRegions = symbolRegions;
        m_barCodeResult.hv_DecodedDataStrings = decodingDatas;
    }
    catch (HalconCpp::HException& except) {
        Herror errorCode = except.ErrorCode();  //获取错误代码
        const char* errorMessage = except.ErrorMessage().Text();
        std::cout << "HALCON错误: " << errorMessage << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "标准异常: " << e.what() << std::endl;
    }
    return m_barCodeResult;
}

//从文件读一维码模型
void ImageProcess::readBarCodeModel(std::string model)
{
    try
    {
        if (model.empty()) {
            std::cout << "条码模型路径为空" << std::endl;
            return;
        }
        HalconCpp::ReadBarCodeModel(model.c_str(), &hv_barCodeMod);
        std::cout << "条码模型加载成功: " << model << std::endl;
    }
    catch (HalconCpp::HException& except) {
        Herror errorCode = except.ErrorCode();
        const char* errorMessage = except.ErrorMessage().Text();
        std::cout << "HALCON错误: " << errorMessage << std::endl;
    }
}

//读形状匹配模型（以后用）
void ImageProcess::readObjectMatchingModel()
{
}

//cv::Mat 转 Halcon 的 HObject（彩色拆三个通道，灰度直接拷贝）
void ImageProcess::matToHobject(cv::Mat& Img, HalconCpp::HObject& hvImg)
{
    int hgt = Img.rows;
    int wid = Img.cols;
    int i;
    if (Img.type() == CV_8UC3)
    {
        std::vector<cv::Mat> imgchannel;
        cv::split(Img, imgchannel);
        cv::Mat imgB = imgchannel[0];
        cv::Mat imgG = imgchannel[1];
        cv::Mat imgR = imgchannel[2];
        uchar* dataR = new uchar[hgt * wid];
        uchar* dataG = new uchar[hgt * wid];
        uchar* dataB = new uchar[hgt * wid];
        for (i = 0; i < hgt; i++)
        {
            memcpy(dataR + wid * i, imgR.data + imgR.step * i, wid);
            memcpy(dataG + wid * i, imgG.data + imgG.step * i, wid);
            memcpy(dataB + wid * i, imgB.data + imgB.step * i, wid);
        }
        HalconCpp::GenImage3(&hvImg, "byte", wid, hgt, (Hlong)dataR, (Hlong)dataG, (Hlong)dataB);
        delete[]dataR;
        delete[]dataG;
        delete[]dataB;
        dataR = NULL;
        dataG = NULL;
        dataB = NULL;
    }
    else if (Img.type() == CV_8UC1)
    {
        uchar* data = new uchar[hgt * wid];
        for (i = 0; i < hgt; i++)
            memcpy(data + wid * i, Img.data + Img.step * i, wid);
        HalconCpp::GenImage1(&hvImg, "byte", wid, hgt, (Hlong)data);
        delete[] data;
        data = NULL;
    }
}

//HObject -> cv::Mat（彩色转成BGR，灰度保持单通道）
void ImageProcess::hobjectToMat(const HalconCpp::HObject& hoImg, cv::Mat& Img)
{
    if (!hoImg.IsInitialized()) {
        Img = cv::Mat();
        return;
    }
    HalconCpp::HTuple hvW, hvH, hvChannels;
    HalconCpp::GetImageSize(hoImg, &hvW, &hvH);
    HalconCpp::CountChannels(hoImg, &hvChannels);
    int wid = hvW[0].I();
    int hgt = hvH[0].I();
    int chn = hvChannels[0].I();

    HalconCpp::HTuple hvType, hvW2, hvH2;
    if (chn == 1) {
        HalconCpp::HTuple hvPtr;
        HalconCpp::GetImagePointer1(hoImg, &hvPtr, &hvType, &hvW2, &hvH2);
        Hlong ptr = hvPtr[0].L();
        cv::Mat tmp(hgt, wid, CV_8UC1, (void*)ptr);
        Img = tmp.clone();   //必须拷贝，Halcon的内存不归OpenCV管
    }
    else if (chn == 3) {
        HalconCpp::HTuple hvPtrR, hvPtrG, hvPtrB;
        HalconCpp::GetImagePointer3(hoImg, &hvPtrR, &hvPtrG, &hvPtrB, &hvType, &hvW2, &hvH2);
        Hlong ptrR = hvPtrR[0].L(), ptrG = hvPtrG[0].L(), ptrB = hvPtrB[0].L();
        std::vector<cv::Mat> channels;
        channels.push_back(cv::Mat(hgt, wid, CV_8UC1, (void*)ptrB));   //OpenCV是BGR
        channels.push_back(cv::Mat(hgt, wid, CV_8UC1, (void*)ptrG));
        channels.push_back(cv::Mat(hgt, wid, CV_8UC1, (void*)ptrR));
        cv::Mat merged;
        cv::merge(channels, merged);
        Img = merged;
    }
    else {
        Img = cv::Mat();
    }
}