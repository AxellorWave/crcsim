import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

DOCS = Path(__file__).resolve().parent
REPORT = DOCS / "report.docx"
PDF = DOCS / "report.pdf"
BUILDER = DOCS / "build_report.py"


def build(pages_file=None):
    arguments = [sys.executable, str(BUILDER)]
    if pages_file is not None:
        arguments.append(str(pages_file))
    subprocess.run(arguments, check=True)


def convert(work):
    profile = work / "profile"
    subprocess.run(["soffice", f"-env:UserInstallation=file://{profile}", "--headless", "--convert-to", "pdf",
                    "--outdir", str(DOCS), str(REPORT)], check=True, capture_output=True)


def page_texts(work):
    pattern = work / "page%03d.txt"
    subprocess.run(["gs", "-q", "-dNOPAUSE", "-dBATCH", "-sDEVICE=txtwrite", f"-sOutputFile={pattern}", str(PDF)],
                   check=True)
    return [path.read_text(encoding="utf-8", errors="replace") for path in sorted(work.glob("page*.txt"))]


def normalized(text):
    return re.sub(r"\s+", " ", text).strip()


def heading_pages(texts, headings):
    pages = {}
    lines = [[normalized(line) for line in text.splitlines()] for text in texts]
    contents_page = next(number for number, page in enumerate(lines, start=1) if "СОДЕРЖАНИЕ" in page)
    for heading in headings:
        target = normalized(heading)[:24]
        for number in range(contents_page + 1, len(lines) + 1):
            if any(line.startswith(target) for line in lines[number - 1]):
                pages[heading] = number
                break
    return pages


def main():
    sys.path.insert(0, str(DOCS))
    from build_report import write_body
    from report_tools import Report

    outline = Report()
    write_body(outline)
    headings = [text for _, text in outline.headings]
    with tempfile.TemporaryDirectory(prefix="crcsim-report-") as directory:
        work = Path(directory)
        build()
        convert(work)
        pages = heading_pages(page_texts(work), headings)
        pages_file = work / "pages.json"
        pages_file.write_text(json.dumps(pages, ensure_ascii=False), encoding="utf-8")
        build(pages_file)
        convert(work)
    missing = [heading for heading in headings if heading not in pages]
    if missing:
        print("Не найдены страницы для:", "; ".join(missing))
    print(f"Готово: {REPORT.name}, {PDF.name}")


if __name__ == "__main__":
    main()
