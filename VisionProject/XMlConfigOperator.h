#pragma once
#include"tinyxml2.h"
#include<string>
#include<map>
#include<iostream>
//不能写 using namespace tinyxml2; 否则 tinyxml2::XMLDocument 会和 Windows 的 msxml.h 里的 XMLDocument 冲突
class XMlConfigOperator
{
	tinyxml2::XMLDocument doc;
	std::string m_filename;
	std::map<std::string, std::string> m_params;      //ShapeModelPara 里的参数
	std::map<std::string, std::string> m_findParams;  //FindShapePara 里的参数
	std::map<std::string, std::string> m_codeParams;  //CodeIdentification 里的参数（条码）
public:
	XMlConfigOperator(const std::string& file) :m_filename(file) {
	}

	bool load() {
		tinyxml2::XMLError error = doc.LoadFile(m_filename.c_str());//下载文件路径
		if (error != tinyxml2::XML_SUCCESS) {
			std::cout << "XML读取失败，错误码：" << error << std::endl;
			return false;
		}
		return true;
	}

	std::map<std::string, std::string> ReadShapeModelParams() {
		tinyxml2::XMLElement* root = doc.RootElement();
		if (!root) {
			return m_params;
		}

		tinyxml2::XMLElement* shapeModelPara = root->FirstChildElement("ObjectMatching")->FirstChildElement("ShapeMatching")
			->FirstChildElement("ShapeModelPara");

		if (!shapeModelPara) {
			return m_params;
		}

		tinyxml2::XMLElement* child = shapeModelPara->FirstChildElement();
		while (child) {
			const char* attrValue = child->Attribute("data");
			std::cout << attrValue << std::endl;
			if (attrValue) {
				m_params[child->Name()] = attrValue;
			}
			child = child->NextSiblingElement();
		}

		return m_params;
	}

	std::map<std::string, std::string> ReadFindShapeAllParams() {
		tinyxml2::XMLElement* root = doc.RootElement();
		if (!root) {
			return m_findParams;
		}

		tinyxml2::XMLElement* shapeModelPara = root->FirstChildElement("ObjectMatching")->FirstChildElement("ShapeMatching")
			->FirstChildElement("FindShapePara");
		if (!shapeModelPara) {
			return m_findParams;
		}

		tinyxml2::XMLElement* child = shapeModelPara->FirstChildElement();
		while (child) {
			const char* attrValue = child->Attribute("data");
			std::cout << attrValue << std::endl;
			if (attrValue) {
				m_findParams[child->Name()] = attrValue;   //注意：这里单独存一份，不然和模板参数里的 AngleStart 重名会互相覆盖
			}
			child = child->NextSiblingElement();
		}

		return m_findParams;
	}

	//读取指定的参数数据（先找模板参数，再找匹配参数）
	std::string& GetNodeParam(const std::string& paramName) {
		static std::string empty_param = "";   //必须是static，否则返回的是局部变量的引用
		std::map<std::string, std::string>::iterator it = m_params.find(paramName);
		if (it != m_params.end()) {
			return it->second;
		}
		it = m_findParams.find(paramName);
		if (it != m_findParams.end()) {
			return it->second;
		}
		it = m_codeParams.find(paramName);
		if (it != m_codeParams.end()) {
			return it->second;
		}
		return empty_param;
	}

	//读取匹配参数（FindShapePara 里的）
	std::string& GetFindParam(const std::string& paramName) {
		static std::string empty_param = "";
		std::map<std::string, std::string>::iterator it = m_findParams.find(paramName);
		if (it != m_findParams.end()) {
			return it->second;
		}
		return empty_param;
	}

	//修改 ShapeModelPara / FindShapePara 下面某个节点的 data 属性
	void UpdateShapeMatchingXMLNodeValue(const std::string& parentNodeName, const std::string& modifyNodeName, const std::string& value) {
		tinyxml2::XMLElement* root = doc.RootElement();   //取到根节点
		if (!root) {
			return;
		}
		tinyxml2::XMLElement* objectMatching = root->FirstChildElement("ObjectMatching");
		if (!objectMatching) {
			return;
		}
		tinyxml2::XMLElement* shapeMatching = objectMatching->FirstChildElement("ShapeMatching");
		if (!shapeMatching) {
			return;
		}
		tinyxml2::XMLElement* parentNode = shapeMatching->FirstChildElement(parentNodeName.c_str());
		if (!parentNode) {
			return;
		}
		tinyxml2::XMLElement* child = parentNode->FirstChildElement(modifyNodeName.c_str());
		if (child) {
			child->SetAttribute("data", value.c_str());
		}
	}

	//读取 CodeIdentification 下面某个节点的所有子节点（比如 BarCode）
	std::map<std::string, std::string> ReadXMLCodeNodeParams(const std::string& nodeName) {
		tinyxml2::XMLElement* root = doc.RootElement();
		if (!root) {
			return m_codeParams;
		}
		tinyxml2::XMLElement* codeIdentification = root->FirstChildElement("CodeIdentification");
		if (!codeIdentification) {
			return m_codeParams;
		}
		tinyxml2::XMLElement* codePara = codeIdentification->FirstChildElement(nodeName.c_str());
		if (!codePara) {
			return m_codeParams;
		}
		tinyxml2::XMLElement* child = codePara->FirstChildElement();
		while (child) {
			const char* attrValue = child->Attribute("data");
			if (attrValue) {
				m_codeParams[child->Name()] = attrValue;
			}
			child = child->NextSiblingElement();
		}
		return m_codeParams;
	}

	//修改 CodeIdentification 下面某个节点的 data 属性
	void UpdateXMLCodeNodeValue(const std::string& parentNodeName, const std::string& modifyNodeName, const std::string& value) {
		tinyxml2::XMLElement* root = doc.RootElement();
		if (!root) {
			return;
		}
		tinyxml2::XMLElement* codeIdentification = root->FirstChildElement("CodeIdentification");
		if (!codeIdentification) {
			return;
		}
		tinyxml2::XMLElement* parentNode = codeIdentification->FirstChildElement(parentNodeName.c_str());
		if (!parentNode) {
			return;
		}
		tinyxml2::XMLElement* child = parentNode->FirstChildElement(modifyNodeName.c_str());
		if (child) {
			child->SetAttribute("data", value.c_str());
		}
	}
	//保存XML文件
	bool SaveXml() {
		tinyxml2::XMLError error = doc.SaveFile(m_filename.c_str());
		if (error != tinyxml2::XML_SUCCESS) {
			std::cout << "XML保存失败，错误码：" << error << std::endl;
			return false;
		}
		return true;
	}
};
