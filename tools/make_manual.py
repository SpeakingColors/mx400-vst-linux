#!/usr/bin/env python3
"""Builds the PDF manual: docs/manual.txt (sections), docs/tour/*.png (screenshots of the real editor),
docs/MIDI-IMPLEMENTATION.md (CC tables). Usage: python3 tools/make_manual.py <out.pdf>   Needs: pip install reportlab"""
import re, sys, os
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, Image, PageBreak, KeepTogether

out = sys.argv[1]
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
H1 = ParagraphStyle("h1", fontName="Helvetica-Bold", fontSize=16, spaceAfter=6, spaceBefore=10)
H2 = ParagraphStyle("h2", fontName="Helvetica-Bold", fontSize=11, spaceAfter=4, spaceBefore=8)
P = ParagraphStyle("p", fontName="Helvetica", fontSize=9.5, leading=13, spaceAfter=4)
C = ParagraphStyle("c", fontName="Helvetica", fontSize=7.5, leading=9)

def esc(t):
    t = t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    t = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", t)
    return re.sub(r"`(.+?)`", r"<font face='Courier'>\1</font>", t)

story = [Paragraph("MX400 Controller", ParagraphStyle("t", fontName="Helvetica-Bold", fontSize=26, leading=32, spaceAfter=4)),
         Paragraph("Studio 87. Free and open source (AGPL-3.0-or-later). Collects nothing.", P), Spacer(1, 6)]

sec = None
for line in open(os.path.join(root, "docs/manual.txt"), encoding="utf-8").read().splitlines():
    if line.startswith("# "):
        story.append(Paragraph(esc(line[2:]), H1)); continue
    if line.strip():
        story.append(Paragraph(esc(line.lstrip("- ")) if not line.startswith("- ") else "&bull; " + esc(line[2:]), P))

tour = [("mx400-panel.png", "Panel", "Routing panel and effect panels. Each knob sends one CC when you move it. The effect picker chooses which CC map and labels are shown; it does not change the unit. Advanced strip: the extra parameters (CC 18 to 25 and 45 to 52 in Stereo). The graph is an illustration, not calibrated."),
        ("mx400-library.png", "Library", "99 program boxes with your notes and favourites, program change and bank select, and the SysEx capture list. Import check validates a .syx file; it does not send it."),
        ("mx400-surround.png", "Surround", "Surround Hall controls (CC 13 to 31). Channel trims and the ducker are not built yet."),
        ("mx400-settings.png", "Settings", "MIDI ports, route (direct or host out), channels A and B, send rate, recall on load, manual mode, MIDI thru, hints. Actions: send all touched, forget touched, send programs again.")]
story.append(PageBreak()); story.append(Paragraph("Interface tour", H1))
story.append(Paragraph("Screenshots are rendered from the real plugin editor. Hover any control in the plugin for its info bubble (hints can be turned off in Settings).", P))
for f, t, d in tour:
    p = os.path.join(root, "docs/tour", f)
    if not os.path.exists(p): continue
    w = 170 * mm
    story.append(KeepTogether([Paragraph(t, H2), Image(p, width=w, height=w * 940 / 1500), Spacer(1, 3), Paragraph(esc(d), P)]))

story.append(PageBreak()); story.append(Paragraph("MIDI reference (official manual only)", H1))
story.append(Paragraph("Every entry below is read from the printed MX400 manual and is not yet verified on hardware.", P))
md = open(os.path.join(root, "docs/MIDI-IMPLEMENTATION.md"), encoding="utf-8").read().splitlines()
i = 0
while i < len(md):
    l = md[i]
    if l.startswith("## ") or l.startswith("### "):
        story.append(Paragraph(esc(l.lstrip("# ")), H2)); i += 1; continue
    if l.startswith("|"):
        rows = []
        while i < len(md) and md[i].startswith("|"):
            if not re.match(r"^\|[\s\-|]+\|$", md[i]):
                rows.append([Paragraph(esc(c.strip()), C) for c in md[i].strip().strip("|").split("|")])
            i += 1
        n = max(len(r) for r in rows)
        rows = [r + [Paragraph("", C)] * (n - len(r)) for r in rows]
        t = Table(rows, colWidths=[(A4[0] - 30 * mm) / n] * n, repeatRows=1)
        t.setStyle(TableStyle([("GRID", (0, 0), (-1, -1), 0.3, colors.grey), ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#dde3f5")), ("VALIGN", (0, 0), (-1, -1), "TOP")]))
        story.append(t); story.append(Spacer(1, 5)); continue
    if l.strip() and not l.startswith("# "):
        story.append(Paragraph(esc(l), P))
    i += 1

SimpleDocTemplate(out, pagesize=A4, leftMargin=15 * mm, rightMargin=15 * mm, topMargin=15 * mm, bottomMargin=15 * mm,
                  title="MX400 Controller manual", author="Studio 87").build(story)
print("wrote", out)
