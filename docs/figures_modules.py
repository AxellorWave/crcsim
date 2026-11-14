from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

OUTPUT = Path(__file__).resolve().parent / "figures" / "modules.png"
INK = "#0b0b0b"
MUTED = "#52514e"
CORE_FILL = "#e8f1fb"
CORE_EDGE = "#2a78d6"
APP_FILL = "#fdeee7"
APP_EDGE = "#eb6834"
TOOL_FILL = "#e6f6ef"
TOOL_EDGE = "#1baf7a"

BOXES = {
    "research": (0.4, 8.6, "research/\nскрипты исследования", TOOL_FILL, TOOL_EDGE),
    "console": (5.6, 8.6, "console/\nконсольная версия", APP_FILL, APP_EDGE),
    "gui": (10.8, 8.6, "gui/\nграфический интерфейс", APP_FILL, APP_EDGE),
    "params": (0.4, 6.0, "params\nпараметры и проверка", CORE_FILL, CORE_EDGE),
    "simulator": (5.6, 6.0, "simulator\nцикл моделирования", CORE_FILL, CORE_EDGE),
    "results": (10.8, 6.0, "results\nоценки и CSV", CORE_FILL, CORE_EDGE),
    "encoder": (0.4, 3.4, "encoder, decoder\nкодер и синдром", CORE_FILL, CORE_EDGE),
    "channel": (5.6, 3.4, "channel\nДСК, Гилберт", CORE_FILL, CORE_EDGE),
    "random": (10.8, 3.4, "random\nгенератор mt19937_64", CORE_FILL, CORE_EDGE),
    "poly": (0.4, 1.0, "poly, code, codes\nмногочлены и коды", CORE_FILL, CORE_EDGE),
}
WIDTH, HEIGHT = 3.2, 1.3

ARROWS = [
    ("research", "console", "аргументы, CSV", "above"),
    ("console", "simulator", "SimulationParams", "right"),
    ("gui", "simulator", "SimulationParams,\nRunControl", "right"),
    ("gui", "results", "SimulationResult", "right"),
    ("params", "simulator", "CyclicCode,\nChannel", "above"),
    ("simulator", "results", "SimulationResult", "above"),
    ("simulator", "encoder", "D, Y", "right"),
    ("simulator", "channel", "вектор E", "right"),
    ("channel", "random", "u ∈ [0; 1)", "above"),
    ("encoder", "poly", "остаток mod 2", "right"),
]


def anchor(name, toward):
    x, y, *_ = BOXES[name]
    tx, ty, *_ = BOXES[toward]
    cx, cy = x + WIDTH / 2, y + HEIGHT / 2
    if abs(ty - y) < 0.1:
        return (x + WIDTH if tx > x else x, cy)
    if abs(tx - x) < 0.1:
        return (cx, y if ty < y else y + HEIGHT)
    return (cx + (0.6 if tx > x else -0.6), y if ty < y else y + HEIGHT)


def main():
    figure, axes = plt.subplots(figsize=(10, 7.4))
    axes.set_xlim(0, 14.4)
    axes.set_ylim(0.4, 10.6)
    axes.axis("off")
    axes.add_patch(FancyBboxPatch((0.15, 0.7), 14.1, 6.95, boxstyle="round,pad=0.05,rounding_size=0.2",
                                  facecolor="none", edgecolor=MUTED, linestyle="--", linewidth=1))
    axes.text(14.1, 0.85, "core/ — вычислительное ядро (только стандартная библиотека C++)", ha="right",
              va="bottom", fontsize=9, color=MUTED)
    for x, y, text, fill, edge in BOXES.values():
        axes.add_patch(FancyBboxPatch((x, y), WIDTH, HEIGHT, boxstyle="round,pad=0.02,rounding_size=0.15",
                                      facecolor=fill, edgecolor=edge, linewidth=1.5))
        axes.text(x + WIDTH / 2, y + HEIGHT / 2, text, ha="center", va="center", fontsize=10, color=INK)
    for source, target, label, place in ARROWS:
        start, end = anchor(source, target), anchor(target, source)
        axes.add_patch(FancyArrowPatch(start, end, arrowstyle="-|>", mutation_scale=14, color=MUTED, linewidth=1.3))
        mx, my = (start[0] + end[0]) / 2, (start[1] + end[1]) / 2
        if place == "above":
            axes.text(mx, my + 0.12, label, fontsize=8.5, color=INK, ha="center", va="bottom")
        else:
            axes.text(mx + 0.15, my, label, fontsize=8.5, color=INK, ha="left", va="center")
    figure.tight_layout()
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(OUTPUT, dpi=200)


if __name__ == "__main__":
    main()
