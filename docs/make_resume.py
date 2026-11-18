# -*- coding: utf-8 -*-
"""张谦帅 简历 —— 表头 + 信息表 + 照片位 + 深色模块条（一页版）"""
from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_TAB_ALIGNMENT, WD_BREAK
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

OUT = r"C:\Users\SPG\Documents\简历\张谦帅_简历_v2.docx"

NAVY = "1F3864"
GRAY = (0x59, 0x59, 0x59)
CONTENT_W = 21.0 - 1.6 * 2

doc = Document()
sec = doc.sections[0]
sec.page_width, sec.page_height = Cm(21.0), Cm(29.7)
sec.left_margin = sec.right_margin = Cm(1.6)
sec.top_margin = Cm(1.0)
sec.bottom_margin = Cm(0.9)

st = doc.styles["Normal"]
st.font.name = "Calibri"
st.font.size = Pt(9.5)
st.element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")
st.paragraph_format.space_after = Pt(0)
st.paragraph_format.line_spacing = Pt(12.6)


def font(run, size=9.5, bold=False, color=None, spacing=None):
    run.font.size = Pt(size)
    run.font.bold = bold
    run.font.name = "Calibri"
    run._element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")
    if color:
        run.font.color.rgb = RGBColor(*color)
    if spacing is not None:
        rPr = run._element.get_or_add_rPr()
        sp = OxmlElement("w:spacing")
        sp.set(qn("w:val"), str(spacing))
        rPr.append(sp)


def p_new(before=0, after=0, align=None):
    p = doc.add_paragraph()
    p.paragraph_format.space_before = Pt(before)
    p.paragraph_format.space_after = Pt(after)
    if align is not None:
        p.alignment = align
    return p


def shade_para(p, fill):
    pPr = p._p.get_or_add_pPr()
    shd = OxmlElement("w:shd")
    shd.set(qn("w:val"), "clear")
    shd.set(qn("w:color"), "auto")
    shd.set(qn("w:fill"), fill)
    pPr.append(shd)


def border_para(p, edge="bottom", color=NAVY, sz="8", space="1"):
    pPr = p._p.get_or_add_pPr()
    pBdr = OxmlElement("w:pBdr")
    e = OxmlElement("w:" + edge)
    e.set(qn("w:val"), "single")
    e.set(qn("w:sz"), sz)
    e.set(qn("w:space"), space)
    e.set(qn("w:color"), color)
    pBdr.append(e)
    pPr.append(pBdr)


def section(title):
    p = p_new(before=5, after=0)
    shade_para(p, NAVY)
    p.paragraph_format.left_indent = Cm(0.15)
    font(p.add_run("  " + title), 10.5, True, (0xFF, 0xFF, 0xFF), spacing=30)


def bullet(text):
    p = doc.add_paragraph()
    p.paragraph_format.left_indent = Cm(0.6)
    p.paragraph_format.first_line_indent = Cm(-0.3)
    p.paragraph_format.space_before = Pt(0.3)
    p.paragraph_format.space_after = Pt(0.3)
    font(p.add_run("● "), 7.5, False, (0x2E, 0x5C, 0x9A))
    font(p.add_run(text), 9.5)


def kv_line(key, value, size=9.5):
    p = p_new(after=0.3)
    font(p.add_run(key + "："), size, True, (0x1F, 0x38, 0x64))
    font(p.add_run(value), size)


def project_header(name, date):
    p = p_new(before=4, after=0.5)
    p.paragraph_format.tab_stops.add_tab_stop(Cm(CONTENT_W), WD_TAB_ALIGNMENT.RIGHT)
    font(p.add_run(name), 10, True, (0x1F, 0x38, 0x64))
    font(p.add_run("\t" + date), 8.5, False, GRAY)


def tech_line(text):
    p = p_new(after=0.5)
    font(p.add_run("技术栈："), 8.5, True, GRAY)
    font(p.add_run(text), 8.5, False, GRAY)


# ==================== 顶部标题条 ====================
p = p_new(before=0, after=0)
font(p.add_run("个 人 简 历"), 19, True, (0x1F, 0x38, 0x64), spacing=40)
p.paragraph_format.line_spacing = Pt(27)   # 标题字号大，不能用固定行距，否则被裁
font(p.add_run("   |  "), 13, False, (0xBF, 0xBF, 0xBF))
font(p.add_run("Personal Resume"), 10.5, False, GRAY, spacing=10)
border_para(p, "bottom", NAVY, "18", "2")

# ==================== 基本信息区（无表格：浮动画框 + 制表位）====================
# 照片占位框：用“浮动段落 + 段落边框”实现，避免表格边框去不掉的问题
p = p_new(before=2, after=0)
pPr = p._p.get_or_add_pPr()
fr = OxmlElement("w:framePr")
fr.set(qn("w:w"), "1500")      # 约 2.6cm
fr.set(qn("w:h"), "1250")      # 约 2.2cm，正好和 4 行信息齐平
fr.set(qn("w:wrap"), "around")
fr.set(qn("w:vAnchor"), "text")
fr.set(qn("w:hAnchor"), "margin")
fr.set(qn("w:xAlign"), "right")
fr.set(qn("w:y"), "0")
pPr.append(fr)
bdr = OxmlElement("w:pBdr")
for edge in ("top", "left", "bottom", "right"):
    e = OxmlElement("w:" + edge)
    e.set(qn("w:val"), "single")
    e.set(qn("w:sz"), "6")
    e.set(qn("w:space"), "6")
    e.set(qn("w:color"), "A6A6A6")
    bdr.append(e)
pPr.append(bdr)
p.alignment = WD_ALIGN_PARAGRAPH.CENTER
font(p.add_run("\n（一寸 / 两寸照片）\n"), 8, False, (0x80, 0x80, 0x80))

# 四行基本信息：用制表位对齐左右两列
info_rows = [
    ("姓　　名", "张谦帅", "出生年月", "【2006.xx】"),
    ("电　　话", "13039665806", "政治面貌", "【共青团员】"),
    ("邮　　箱", "155331234@qq.com", "学　　历", "本科"),
    ("毕业院校", "黑龙江大学", "专　　业", "软件工程"),
]
for k1, v1, k2, v2 in info_rows:
    p = p_new(before=1, after=1)
    p.paragraph_format.tab_stops.add_tab_stop(Cm(8.6), WD_TAB_ALIGNMENT.LEFT)
    font(p.add_run(k1 + "："), 9.5, True, (0x1F, 0x38, 0x64))
    font(p.add_run(v1), 9.5)
    font(p.add_run("\t"), 9.5)
    font(p.add_run(k2 + "："), 9.5, True, (0x1F, 0x38, 0x64))
    font(p.add_run(v2), 9.5)

# 信息区下面的细分隔线
p = p_new(before=1, after=0)
font(p.add_run(""), 2)
border_para(p, "bottom", "BFBFBF", "6", "1")

# ==================== 教育背景 ====================
section("教 育 背 景")
p = p_new(before=0.5, after=0.5)
p.paragraph_format.tab_stops.add_tab_stop(Cm(CONTENT_W), WD_TAB_ALIGNMENT.RIGHT)
font(p.add_run("黑龙江大学"), 10, True)
font(p.add_run("    软件工程    本科"), 10)
font(p.add_run("\t2024.09 – 2028.06"), 8.5, False, GRAY)
kv_line("主修课程", "C++ 程序设计、数据结构、操作系统、计算机网络、数据库原理、软件工程")
kv_line("专业基础", "数据结构与算法、计算机组成原理、操作系统、计算机网络、数据库（四大件）")

# ==================== 专业技能 ====================
section("专 业 技 能")
skills = [
    ("编程语言", "C++（熟练：STL 容器/迭代器、模板、智能指针、RAII、移动语义）；C（熟悉）；Python / SQL（了解）"),
    ("并发与网络", "std::thread / mutex / condition_variable / atomic；线程池、生产者-消费者、有界队列与丢帧、线程安全退出（join）；TCP/UDP、粘包半包、长度前缀协议、IO 复用（select / epoll / Reactor）"),
    ("图像与算法", "HALCON（形状模板匹配、一维码识别、ROI 交互）；OpenCV（Mat 操作、颜色空间、编解码）；Halcon 窗口与 Qt 结合"),
    ("界面开发", "Qt6 Widgets、信号槽（含跨线程 QueuedConnection）、QSS 样式、QMetaObject::invokeMethod、自定义控件"),
    ("工程工具", "Visual Studio 2022、CMake、Git（提交规范/分支）、ctest 单元测试、windeployqt 打包、依赖分析、Linux 命令与 Shell"),
    ("设计能力", "面向对象设计、单例 / 工厂 / 观察者模式、模块分层（界面 / 算法 / 采集 / 通信）"),
]
for k, v in skills:
    p = p_new(after=0.3)
    font(p.add_run(k + "："), 9.5, True, (0x1F, 0x38, 0x64))
    font(p.add_run(v), 9.5)

# ==================== 项目经历 ====================
section("项 目 经 历")

project_header("工业视觉检测上位机（Qt6 + HALCON + OpenCV）", "2026.09 – 至今")
tech_line("C++17、Qt6、HALCON 17.12、OpenCV 4.8、海康 MVS SDK、Winsock2、JSON、XML、Git")
p = p_new(after=1)
font(p.add_run("代码仓库："), 9, True, GRAY)
font(p.add_run("github.com/Apple-zhang404/VisionProject"), 9, False, (0x2E, 0x5C, 0x9A))
font(p.add_run("（私有仓库，可通过邀请查看）"), 9, False, GRAY)
for b in [
    "面向四路工业相机的桌面视觉软件，独立完成采集、算法、界面、网络上报到发布打包的完整链路。",
    "采集层：抽象 Camera 基类 + 工厂/单例（DeviceManager）管理四路设备，屏蔽海康 MVS SDK 与离线图片序列两种数据源，切换只改一个模式开关。",
    "显示层：有界队列（容量 1）+ 条件变量解耦采集与显示，新帧覆盖旧帧实现主动丢帧，显示延迟恒定在一帧内、内存恒定，避免 30fps 采集把主线程打满。",
    "线程安全：UI 更新统一经 QMetaObject::invokeMethod（QueuedConnection）投递回主线程，消除子线程操作控件导致的偶发崩溃；四个工作线程用原子标志 + join 回收，关闭窗口耗时由 5.2s 降至 0.4s。",
    "算法与工程化：封装 HALCON 形状模板匹配与一维码识别（参数 XML 持久化、识别失败自动旋转 90/180/270 重试）；自研线程安全日志模块（单例 + 互斥锁 + 按天写文件）；用 windeployqt 打包，可在无开发环境的机器运行。",
]:
    bullet(b)

project_header("Linux C++ TCP 服务器（Reactor 模型）", "2026.10 – 2026.12")
tech_line("C++17、epoll、CMake、Linux、ctest")
for b in [
    "基于 epoll 实现单线程 Reactor 服务器，按 EventLoop / Epoller / Channel / Acceptor / TcpConnection 分层，非阻塞 IO + 事件驱动。",
    "设计长度前缀协议（magic + type + length + payload，大端序，64 KiB 上限）解决 TCP 粘包/半包：数据不足返回 NeedMore 且不消费缓冲区，magic 或长度异常直接断开。",
    "用 CMake 组织工程，为 Buffer 与 Codec 编写单元测试，覆盖半包、粘包、长度越界等边界场景。",
]:
    bullet(b)

# ==================== 自我评价 ====================
section("自 我 评 价")
p = p_new(before=0.5, after=0)
font(p.add_run(
    "软件工程本科在读，独立完成两个 C++ 项目（工程型上位机 + 基础型网络服务器）：习惯用日志与最小复现定位问题，"
    "对跨线程、资源生命周期类问题有实际排查经验；能读英文技术文档，愿长期投入 C++ 与工业软件方向。"
), 9.5)
p = p_new(before=1, after=0)
font(p.add_run(
    "个人特质：沟通表达清晰，课程小组中负责需求拆分与接口对齐；抗压能力强，能扛住长周期调试（如多线程崩溃复现）；"
    "热爱篮球，长期参加院系比赛，团队协作意识与执行力较强。"
), 9.5)
_brk = p.add_run()
_brk.add_break(WD_BREAK.PAGE)


# ==================== 第二页：项目展示 ====================

p = p_new(before=0, after=2)
shade_para(p, NAVY)
p.paragraph_format.left_indent = Cm(0.15)
font(p.add_run("  项 目 展 示 （ 离 线 模 拟 采 集 ）"), 12, True, (0xFF, 0xFF, 0xFF), spacing=20)

p = p_new(before=2, after=6)
font(p.add_run("说明：以下截图均为离线图片序列模拟四路相机采集，未连接真实工业相机；界面为 Qt6 + QSS 深色主题，算法为 HALCON 模板匹配与一维码识别。"), 9, False, GRAY)


def figure(path, caption, width_cm):
    doc.add_picture(path, width=Cm(width_cm))
    _p = doc.paragraphs[-1]
    _p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    _p.paragraph_format.line_spacing = 1.0   # 图片段落不能用固定行距，否则图片被裁成一条
    cap = p_new(before=1, after=4)
    cap.alignment = WD_ALIGN_PARAGRAPH.CENTER
    font(cap.add_run(caption), 8.5, False, GRAY)


figure(r"C:/Users/SPG/source/repos/VisionProject/docs/images/main-window.png",
       "主界面：四路相机采集与显示（右侧为运行日志，带时间戳与级别）", 14.0)

figure(r"C:/Users/SPG/source/repos/VisionProject/docs/images/imagetool.png",
       "参数与算法窗口：形状模板匹配参数、ROI 交互、条码识别页", 9.5)

figure(r"C:/Users/SPG/source/repos/VisionProject/docs/images/barcode-page.png",
       "条码识别页：左侧图像显示，右侧参数配置与识别结果", 9.5)

doc.save(OUT)
print("saved:", OUT)
