# -*- coding: utf-8 -*-
"""把基本信息区从表格换成：浮动边框照片框 + 制表位文字行"""
import io

p = r"C:\Users\SPG\source\repos\VisionProject\docs\make_resume.py"
s = io.open(p, encoding="utf-8").read()

start = s.index("# ==================== 基本信息表（含照片位）====================")
end = s.index("# ==================== 教育背景 ====================")

new_block = '''# ==================== 基本信息区（无表格：浮动画框 + 制表位）====================
# 照片占位框：用“浮动段落 + 段落边框”实现，避免表格边框去不掉的问题
p = p_new(before=2, after=0)
pPr = p._p.get_or_add_pPr()
fr = OxmlElement("w:framePr")
fr.set(qn("w:w"), "1500")      # 约 2.6cm
fr.set(qn("w:h"), "2000")      # 约 3.5cm
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
font(p.add_run("\\n\\n\\n（一寸 / 两寸照片）\\n"), 8, False, (0x80, 0x80, 0x80))

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
    font(p.add_run("\\t"), 9.5)
    font(p.add_run(k2 + "："), 9.5, True, (0x1F, 0x38, 0x64))
    font(p.add_run(v2), 9.5)

# 信息区下面的细分隔线
p = p_new(before=1, after=0)
font(p.add_run(""), 2)
border_para(p, "bottom", "BFBFBF", "6", "1")

'''

s = s[:start] + new_block + s[end:]
io.open(p, "w", encoding="utf-8").write(s)
print("patched: table -> paragraphs")
