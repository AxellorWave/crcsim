import re

from docx import Document
from docx.enum.section import WD_SECTION
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_BREAK, WD_TAB_ALIGNMENT, WD_TAB_LEADER
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Pt, RGBColor

FONT = "Times New Roman"
CODE_FONT = "Courier New"
SUPERSCRIPT = str.maketrans("-0123456789", "⁻⁰¹²³⁴⁵⁶⁷⁸⁹")


def decimal(value, digits=3):
    return f"{value:.{digits}g}".replace(".", ",")


def scientific(value, digits=2):
    if value == 0:
        return "0"
    mantissa, exponent = f"{value:.{digits}e}".split("e")
    mantissa = mantissa.replace(".", ",")
    power = str(int(exponent)).translate(SUPERSCRIPT)
    return f"{mantissa}·10{power}"


def probability(events, trials):
    if events == 0:
        return f"— (< {scientific(3 / trials, 1)})"
    return scientific(events / trials)


def set_cell_shading(cell, color):
    properties = cell._tc.get_or_add_tcPr()
    shading = OxmlElement("w:shd")
    shading.set(qn("w:val"), "clear")
    shading.set(qn("w:color"), "auto")
    shading.set(qn("w:fill"), color)
    properties.append(shading)


def set_repeat_header(row):
    properties = row._tr.get_or_add_trPr()
    header = OxmlElement("w:tblHeader")
    header.set(qn("w:val"), "true")
    properties.append(header)


def add_field(paragraph, instruction):
    run = paragraph.add_run()
    begin = OxmlElement("w:fldChar")
    begin.set(qn("w:fldCharType"), "begin")
    text = OxmlElement("w:instrText")
    text.set(qn("xml:space"), "preserve")
    text.text = instruction
    separate = OxmlElement("w:fldChar")
    separate.set(qn("w:fldCharType"), "separate")
    end = OxmlElement("w:fldChar")
    end.set(qn("w:fldCharType"), "end")
    run._r.append(begin)
    run._r.append(text)
    run._r.append(separate)
    run._r.append(end)


class Report:
    def __init__(self):
        self.document = Document()
        self.figures = 0
        self.tables = 0
        self.listings = 0
        self.headings = []
        self._setup_page()
        self._setup_styles()

    def _setup_page(self):
        section = self.document.sections[0]
        section.page_height = Cm(29.7)
        section.page_width = Cm(21.0)
        section.left_margin = Cm(3.0)
        section.right_margin = Cm(2.0)
        section.top_margin = Cm(2.0)
        section.bottom_margin = Cm(2.0)
        section.different_first_page_header_footer = True
        footer = section.footer.paragraphs[0]
        footer.alignment = WD_ALIGN_PARAGRAPH.CENTER
        add_field(footer, "PAGE")

    def _font(self, style, size, bold=False, name=FONT):
        style.font.name = name
        style.font.size = Pt(size)
        style.font.bold = bold
        style.font.color.rgb = RGBColor(0, 0, 0)
        fonts = style.element.get_or_add_rPr().get_or_add_rFonts()
        for attribute in ("w:ascii", "w:hAnsi", "w:cs", "w:eastAsia"):
            fonts.set(qn(attribute), name)

    def _setup_styles(self):
        styles = self.document.styles
        normal = styles["Normal"]
        self._font(normal, 14)
        normal.paragraph_format.line_spacing = 1.5
        normal.paragraph_format.first_line_indent = Cm(1.25)
        normal.paragraph_format.space_after = Pt(0)
        normal.paragraph_format.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
        for level, size in ((1, 14), (2, 14), (3, 14)):
            heading = styles[f"Heading {level}"]
            self._font(heading, size, bold=True)
            heading.paragraph_format.first_line_indent = Cm(1.25)
            heading.paragraph_format.space_before = Pt(12)
            heading.paragraph_format.space_after = Pt(6)
            heading.paragraph_format.keep_with_next = True
            heading.paragraph_format.alignment = WD_ALIGN_PARAGRAPH.LEFT
            heading.paragraph_format.line_spacing = 1.5
        for name in ("TOC 1", "TOC 2", "TOC 3"):
            toc = styles[name] if name in styles else styles.add_style(name, 1)
            self._font(toc, 14)
            toc.paragraph_format.first_line_indent = Cm(0)
            toc.paragraph_format.left_indent = Cm(0.75 * (int(name[-1]) - 1))
            toc.paragraph_format.line_spacing = 1.15
        for name in ("List Bullet", "List Number"):
            self._font(styles[name], 14)
            styles[name].paragraph_format.line_spacing = 1.5
            styles[name].paragraph_format.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY

    def _runs(self, paragraph, text):
        for part in re.split(r"(\*\*[^*]+\*\*|`[^`]+`)", text):
            if not part:
                continue
            if part.startswith("**"):
                paragraph.add_run(part[2:-2]).bold = True
            elif part.startswith("`"):
                run = paragraph.add_run(part[1:-1])
                run.font.name = CODE_FONT
                run.font.size = Pt(12)
            else:
                paragraph.add_run(part)

    def paragraph(self, text, alignment=None, indent=True):
        paragraph = self.document.add_paragraph()
        self._runs(paragraph, text)
        if alignment is not None:
            paragraph.alignment = alignment
        if not indent:
            paragraph.paragraph_format.first_line_indent = Cm(0)
        return paragraph

    def centered(self, text, bold=False, size=14, space_after=0):
        paragraph = self.document.add_paragraph()
        paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER
        paragraph.paragraph_format.first_line_indent = Cm(0)
        paragraph.paragraph_format.space_after = Pt(space_after)
        run = paragraph.add_run(text)
        run.bold = bold
        run.font.size = Pt(size)
        return paragraph

    def formula(self, text):
        paragraph = self.centered(text)
        paragraph.paragraph_format.space_before = Pt(6)
        paragraph.paragraph_format.space_after = Pt(6)
        return paragraph

    def bullets(self, items):
        for item in items:
            paragraph = self.document.add_paragraph(style="List Bullet")
            self._runs(paragraph, item)

    def numbered(self, items):
        for index, item in enumerate(items, start=1):
            paragraph = self.document.add_paragraph()
            self._runs(paragraph, f"{index}) {item}")

    def heading(self, text, level=1):
        self.headings.append((level, text))
        return self.document.add_heading(text, level=level)

    def page_break(self):
        self.document.add_paragraph().add_run().add_break(WD_BREAK.PAGE)

    def figure(self, path, caption, width_cm=16.0):
        self.figures += 1
        paragraph = self.document.add_paragraph()
        paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER
        paragraph.paragraph_format.first_line_indent = Cm(0)
        paragraph.paragraph_format.keep_with_next = True
        paragraph.paragraph_format.space_before = Pt(6)
        paragraph.add_run().add_picture(str(path), width=Cm(width_cm))
        self.centered(f"Рисунок {self.figures} — {caption}", space_after=6)
        return self.figures

    def table(self, header, rows, caption, widths=None, font_size=11, bold_cells=None, shade_cells=None):
        self.tables += 1
        title = self.paragraph(f"Таблица {self.tables} — {caption}", indent=False)
        title.alignment = WD_ALIGN_PARAGRAPH.LEFT
        title.paragraph_format.keep_with_next = True
        title.paragraph_format.space_before = Pt(6)
        table = self.document.add_table(rows=1, cols=len(header))
        table.style = "Table Grid"
        table.autofit = False
        table.alignment = WD_TABLE_ALIGNMENT.CENTER
        for index, text in enumerate(header):
            self._cell(table.rows[0].cells[index], text, font_size, bold=True, center=True)
        set_repeat_header(table.rows[0])
        for row_index, row in enumerate(rows):
            cells = table.add_row().cells
            for column, text in enumerate(row):
                bold = bold_cells is not None and (row_index, column) in bold_cells
                self._cell(cells[column], str(text), font_size, bold=bold, center=len(str(text)) <= 16)
                if shade_cells is not None and (row_index, column) in shade_cells:
                    set_cell_shading(cells[column], "E2EFDA")
        if widths:
            for column, width in zip(table.columns, widths):
                column.width = Cm(width)
            for row in table.rows:
                for index, width in enumerate(widths):
                    row.cells[index].width = Cm(width)
        spacer = self.document.add_paragraph()
        spacer.paragraph_format.space_after = Pt(0)
        spacer.paragraph_format.line_spacing = 1.0
        return self.tables

    def _cell(self, cell, text, font_size, bold=False, center=False):
        paragraph = cell.paragraphs[0]
        paragraph.paragraph_format.first_line_indent = Cm(0)
        paragraph.paragraph_format.line_spacing = 1.0
        paragraph.alignment = WD_ALIGN_PARAGRAPH.CENTER if center else WD_ALIGN_PARAGRAPH.LEFT
        self._runs(paragraph, text)
        for run in paragraph.runs:
            run.font.size = Pt(font_size)
            if bold:
                run.bold = True

    def code(self, text, caption=None, size=10, indent_cm=0.5):
        if caption:
            self.listings += 1
            title = self.paragraph(f"Листинг {self.listings} — {caption}", indent=False)
            title.paragraph_format.keep_with_next = True
            title.paragraph_format.space_before = Pt(6)
        lines = text.rstrip("\n").split("\n")
        for index, line in enumerate(lines):
            paragraph = self.document.add_paragraph()
            paragraph.alignment = WD_ALIGN_PARAGRAPH.LEFT
            paragraph.paragraph_format.first_line_indent = Cm(0)
            paragraph.paragraph_format.left_indent = Cm(indent_cm)
            paragraph.paragraph_format.line_spacing = 1.0
            paragraph.paragraph_format.keep_together = True
            if index < len(lines) - 1 and len(lines) < 40:
                paragraph.paragraph_format.keep_with_next = True
            if index == len(lines) - 1:
                paragraph.paragraph_format.space_after = Pt(8)
            run = paragraph.add_run(line if line else " ")
            run.font.name = CODE_FONT
            run.font.size = Pt(size)
            run._r.get_or_add_rPr().get_or_add_rFonts().set(qn("w:hAnsi"), CODE_FONT)

    def table_of_contents(self, entries, pages):
        self.centered("СОДЕРЖАНИЕ", bold=True, space_after=12)
        for level, text in entries:
            paragraph = self.document.add_paragraph()
            paragraph.alignment = WD_ALIGN_PARAGRAPH.LEFT
            paragraph.paragraph_format.first_line_indent = Cm(0)
            paragraph.paragraph_format.left_indent = Cm(0.75 * (level - 1))
            paragraph.paragraph_format.line_spacing = 1.15
            paragraph.paragraph_format.space_after = Pt(2)
            paragraph.paragraph_format.tab_stops.add_tab_stop(Cm(16.0), WD_TAB_ALIGNMENT.RIGHT, WD_TAB_LEADER.DOTS)
            paragraph.add_run(f"{text}\t{pages.get(text, '')}")

    def save(self, path):
        self.document.save(str(path))
