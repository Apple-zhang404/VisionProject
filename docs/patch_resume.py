# -*- coding: utf-8 -*-
"""把简历脚本压缩到一页：技能分组精简、项目一条合并、行距收紧"""
import io, re

p = r"C:\Users\SPG\source\repos\VisionProject\docs\make_resume.py"
s = io.open(p, encoding="utf-8").read()

# 1) 行距/字号更紧
s = s.replace("st.font.size = Pt(10)", "st.font.size = Pt(9.5)")
s = s.replace("st.paragraph_format.line_spacing = 1.06", "st.paragraph_format.line_spacing = 1.0")
s = s.replace("sec.top_margin = Cm(1.2)", "sec.top_margin = Cm(1.0)")
s = s.replace("sec.bottom_margin = Cm(1.2)", "sec.bottom_margin = Cm(1.0)")
s = s.replace('def section(title):\n    """深色条 + 白色标题（模仿模板里的黑蓝横条）"""\n    p = p_new(before=7, after=0)',
              'def section(title):\n    """深色条 + 白色标题（模仿模板里的黑蓝横条）"""\n    p = p_new(before=5, after=0)')

# 2) 技能：8 组压成 6 组
old_skills = s[s.index('skills = ['):s.index(']\nfor k, v in skills:') + 1]
new_skills = '''skills = [
    ("编程语言", "C++（熟练：STL 容器/迭代器、模板、智能指针、RAII、移动语义）；C（熟悉）；Python / SQL（了解）"),
    ("并发与网络", "std::thread / mutex / condition_variable / atomic；线程池、生产者-消费者、有界队列与丢帧、线程安全退出（join）；TCP/UDP、粘包半包、长度前缀协议、IO 复用（select / epoll / Reactor）"),
    ("图像与算法", "HALCON（形状模板匹配、一维码识别、ROI 交互）；OpenCV（Mat 操作、颜色空间、编解码）；Halcon 窗口与 Qt 结合"),
    ("界面开发", "Qt6 Widgets、信号槽（含跨线程 QueuedConnection）、QSS 样式、QMetaObject::invokeMethod、自定义控件"),
    ("工程工具", "Visual Studio 2022、CMake、Git（提交规范/分支）、ctest 单元测试、windeployqt 打包、依赖分析、Linux 命令与 Shell"),
    ("设计能力", "面向对象设计、单例 / 工厂 / 观察者模式、模块分层（界面 / 算法 / 采集 / 通信）"),
]'''
s = s.replace(old_skills, new_skills)

# 3) 项目一：6 条压成 5 条（合并“工程化”到“算法层”后面那条）
s = s.replace(
    '''    "算法层：封装 HALCON 形状模板匹配与一维码识别，参数持久化到 XML 并与界面双向同步；识别失败自动旋转 90/180/270 重试，覆盖竖贴条码场景。",
    "工程化：自研线程安全日志模块（单例 + 互斥锁 + 时间戳/级别 + 按天写文件，信号回主线程显示）；用 windeployqt 与依赖分析打包，可在无开发环境的机器运行。",''',
    '''    "算法与工程化：封装 HALCON 形状模板匹配与一维码识别（参数 XML 持久化、失败自动旋转 90/180/270 重试）；自研线程安全日志模块（单例 + 互斥锁 + 按天写文件）；用 windeployqt 打包，可在无开发环境的机器运行。",''')

# 4) 自我评价压成两行
old_eval = s[s.index('font(p.add_run(\n    "软件工程本科在读'):s.index('), 10)') + len('), 10)')]
new_eval = '''font(p.add_run(
    "软件工程本科在读，独立完成两个 C++ 项目（工程型上位机 + 基础型网络服务器）。"
    "习惯用日志与最小复现定位问题，对跨线程、资源生命周期类问题有实际排查经验；能读英文技术文档，愿长期投入 C++ 与工业软件方向。"
), 9.5)'''
s = s.replace(old_eval, new_eval)

io.open(p, "w", encoding="utf-8").write(s)
print("patched")
