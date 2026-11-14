import csv
import math
from collections import defaultdict
from fractions import Fraction
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from estimates import TARGET, upper_bound, verdict

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "research" / "data"
OUT = ROOT / "research" / "results"

FILE_BITS = 1e6
BIT_RATE = 9600.0
SERIES_COLORS = ["#2a78d6", "#eb6834", "#1baf7a"]
TEXT_SECONDARY = "#52514e"
GRID = "#e4e3df"
CHANNEL_TITLES = {"BSC": "ДСК", "Gilbert": "канал Гилберта"}
LEVEL_NAMES = {"BSC": "p", "Gilbert": "P_ХП"}


def read_rows(name):
    with open(DATA / name, encoding="utf-8") as source:
        return list(csv.DictReader(source, delimiter=";"))


def write_rows(name, rows):
    with open(OUT / name, "w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=list(rows[0].keys()), delimiter=";")
        writer.writeheader()
        writer.writerows(rows)


def key(row):
    return row["channel"], row["level"], row["n"], row["g"]


def check_bits(row):
    return int(row["n"]) - int(row["k"])


def final_rows():
    rows = {key(row): row for row in read_rows("simulations.csv")}
    for row in read_rows("refined.csv"):
        rows[key(row)] = row
    return rows


def probability_text(events, trials):
    if events == 0:
        return f"— (<{3 / trials:.1e})"
    return f"{events / trials:.2e}"


def transfer_time(row):
    n, k = int(row["n"]), int(row["k"])
    correct = int(row["Nv"])
    attempts = int(row["N"]) / correct if correct else math.inf
    return FILE_BITS / k * n / BIT_RATE * attempts, attempts


def levels_of(rows, channel):
    return sorted({row["level"] for row in rows if row["channel"] == channel}, key=float, reverse=True)


def result_tables(base, final):
    tables = {}
    for channel in ("BSC", "Gilbert"):
        levels = levels_of(base.values(), channel)
        table = []
        codes = sorted({(int(r["n"]), check_bits(r), r["k"], r["t"], r["g"]) for r in base.values()})
        for n, h, k, t, g in codes:
            line = {"n": n, "k": k, "h": h, "t": t, "g": g}
            for level in levels:
                row = final[(channel, level, str(n), g)]
                events, trials = int(row["Nlozh"]), int(row["N"])
                line[f"Plozh {LEVEL_NAMES[channel]}={level}"] = probability_text(events, trials)
                line[f"ok {LEVEL_NAMES[channel]}={level}"] = {"yes": "да", "no": "нет", "borderline": "?"}[verdict(events, trials)]
                if trials != 3000000:
                    line[f"Plozh {LEVEL_NAMES[channel]}={level}"] += f" [N={trials:.0e}]"
            table.append(line)
        tables[channel] = table
        write_rows(f"plozh_{channel.lower()}.csv", table)
    return tables


def best_codes(final):
    per_length = []
    for (channel, level, n, g), row in sorted(final.items(), key=lambda item: (item[0][0], -float(item[0][1]), int(item[0][2]))):
        events, trials = int(row["Nlozh"]), int(row["N"])
        if verdict(events, trials) != "yes":
            continue
        time, attempts = transfer_time(row)
        per_length.append({"channel": channel, "level": level, "n": int(n), "k": int(row["k"]), "h": check_bits(row),
                           "g": g, "Nlozh": events, "N": trials, "upper": f"{upper_bound(events, trials):.2e}",
                           "I": round(check_bits(row) / int(n), 3), "L": round(attempts, 4), "Tf": round(time, 1)})
    minimal = {}
    for row in per_length:
        group = (row["channel"], row["level"], row["n"])
        if group not in minimal or row["h"] < minimal[group]["h"]:
            minimal[group] = row
    chosen = list(minimal.values())
    write_rows("best_per_length.csv", chosen)
    overall = []
    for channel, level in sorted({(r["channel"], r["level"]) for r in chosen}, key=lambda x: (x[0], -float(x[1]))):
        candidates = [r for r in chosen if (r["channel"], r["level"]) == (channel, level)]
        by_redundancy = min(candidates, key=lambda r: (r["I"], r["Tf"]))
        by_time = min(candidates, key=lambda r: r["Tf"])
        overall.append({"channel": channel, "level": level,
                        "best_I": f"({by_redundancy['n']},{by_redundancy['k']}) I={by_redundancy['I']} Tf={by_redundancy['Tf']}",
                        "best_Tf": f"({by_time['n']},{by_time['k']}) I={by_time['I']} Tf={by_time['Tf']}"})
    write_rows("best_overall.csv", overall)
    return chosen, overall


def undetected_probability(spectrum, n, h, p):
    p = Fraction(p)
    counts = [int(c) for c in spectrum["counts"].split()]
    if spectrum["kind"] == "code":
        total = sum(count * p ** w * (1 - p) ** (n - w) for w, count in enumerate(counts) if w > 0)
    else:
        total = sum(count * (1 - 2 * p) ** j for j, count in enumerate(counts)) / 2 ** h - (1 - p) ** n
    return float(total)


def theory_tables(base):
    spectra = {(row["n"], row["g"]): row for row in read_rows("weight_spectra.csv")}
    correct_rows, undetected_rows = [], []
    seen_lengths = set()
    for row in base.values():
        if row["channel"] != "BSC":
            continue
        n, h, p, trials = int(row["n"]), check_bits(row), float(row["p"]), int(row["N"])
        if (n, p) not in seen_lengths:
            seen_lengths.add((n, p))
            expected = (1 - p) ** n
            measured = int(row["Nv"]) / trials
            sigma = math.sqrt(expected * (1 - expected) / trials)
            correct_rows.append({"n": n, "p": p, "Pv": f"{measured:.5f}", "Pv_theory": f"{expected:.5f}",
                                 "z": round((measured - expected) / sigma, 2)})
        theory = undetected_probability(spectra[(row["n"], row["g"])], n, h, p)
        mean = theory * trials
        events = int(row["Nlozh"])
        if mean < 1e-3 and events == 0:
            agreement = "да"
        elif mean < 30:
            agreement = "да" if poisson_tail_ok(events, mean) else "нет"
        else:
            agreement = "да" if abs(events - mean) <= 3 * math.sqrt(mean) else "нет"
        undetected_rows.append({"n": n, "k": row["k"], "h": h, "p": p, "Nlozh": events,
                                "expected": f"{mean:.3g}", "Plozh": probability_text(events, trials),
                                "Plozh_theory": f"{theory:.3e}", "agree": agreement})
    correct_rows.sort(key=lambda r: (r["n"], -r["p"]))
    undetected_rows.sort(key=lambda r: (r["n"], r["h"], -r["p"]))
    write_rows("theory_pv.csv", correct_rows)
    write_rows("theory_plozh.csv", undetected_rows)
    replicates = read_rows("replicates.csv")
    counts = [int(row["Nlozh"]) for row in replicates]
    first = replicates[0]
    theory = undetected_probability(spectra[(first["n"], first["g"])], int(first["n"]), check_bits(first), float(first["p"]))
    write_rows("replicates_summary.csv", [{"n": first["n"], "g": first["g"], "p": first["p"],
                                           "seeds": " ".join(row["seed"] for row in replicates),
                                           "Nlozh": " ".join(map(str, counts)),
                                           "mean": round(sum(counts) / len(counts), 2),
                                           "expected": round(theory * int(first["N"]), 2)}])
    return correct_rows, undetected_rows


def poisson_tail_ok(events, mean):
    below = sum(math.exp(-mean + k * math.log(mean) - math.lgamma(k + 1)) for k in range(events + 1)) if mean > 0 else 1.0
    above = 1.0 - below + (math.exp(-mean + events * math.log(mean) - math.lgamma(events + 1)) if mean > 0 else 0.0)
    return below > 0.005 and above > 0.005


def weight_table():
    rows = []
    for row in read_rows("error_weights.csv"):
        if row["channel"] != "bsc":
            continue
        n, p, j, count = int(row["n"]), float(row["level"]), int(row["j"]), int(row["count"])
        probability = math.comb(n, j) * p ** j * (1 - p) ** (n - j)
        expected = 3000000 * probability
        if expected < 5 and count < 5:
            continue
        sigma = math.sqrt(expected * (1 - probability))
        rows.append({"n": n, "p": p, "j": j, "observed": count, "expected": f"{expected:.1f}",
                     "z": round((count - expected) / sigma, 2)})
    rows.sort(key=lambda r: (r["n"], -r["p"], r["j"]))
    write_rows("theory_weights.csv", rows)
    return rows


def style_axes(axes):
    axes.set_yscale("log")
    axes.grid(True, which="major", color=GRID, linewidth=0.8)
    axes.set_axisbelow(True)
    for side in ("top", "right"):
        axes.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        axes.spines[side].set_color(TEXT_SECONDARY)
    axes.tick_params(colors=TEXT_SECONDARY, labelsize=9)


def plot_series(axes, points, color, label, linestyle="-", offset=0.0):
    points.sort()
    observed = [(h, value) for h, value, seen in points if seen]
    unseen = [(h + offset, value) for h, value, seen in points if not seen]
    if observed:
        axes.plot(*zip(*observed), color=color, linewidth=2, linestyle=linestyle, marker="o", markersize=6, label=label)
    if unseen:
        axes.plot(*zip(*unseen), color=color, linestyle="none", marker="v", markersize=8, markerfacecolor="white",
                  markeredgewidth=1.5, label=None if observed else label)


def decimal_comma(text):
    return text.replace(".", ",")


def max_check_bits(final, n):
    return max(check_bits(row) for row in final.values() if row["n"] == str(n))


def collect_points(final, channel, level, n):
    points = []
    for (row_channel, row_level, row_n, _), row in final.items():
        if (row_channel, row_level, row_n) != (channel, level, str(n)):
            continue
        events, trials = int(row["Nlozh"]), int(row["N"])
        value = events / trials if events else 3 / trials
        points.append((check_bits(row), value, events > 0))
    return points


def finish_figure(figure, axes_list, title):
    for axes in axes_list:
        axes.axhline(TARGET, color=TEXT_SECONDARY, linestyle="--", linewidth=1)
        axes.set_xlabel("h — число проверочных бит", color=TEXT_SECONDARY, fontsize=9)
    axes_list[0].set_ylabel("Pлож", color=TEXT_SECONDARY, fontsize=9)
    axes_list[0].text(axes_list[0].get_xlim()[0], TARGET * 1.3, "требование 10⁻⁶", color=TEXT_SECONDARY, fontsize=8)
    handles, labels = axes_list[0].get_legend_handles_labels()
    unseen = plt.Line2D([], [], color=TEXT_SECONDARY, linestyle="none", marker="v", markersize=8,
                        markerfacecolor="white", label="не наблюдалось (оценка сверху 3/N)")
    figure.legend(handles + [unseen], labels + [unseen.get_label()], loc="lower center", ncol=2, frameon=False, fontsize=9)
    figure.suptitle(title, fontsize=11)
    figure.tight_layout(rect=(0, 0.13, 1, 0.97))


def plot_channel(final, channel):
    lengths = [15, 31, 63]
    figure, axes_list = plt.subplots(1, len(lengths), figsize=(8, 4.4), sharey=True)
    for axes, n in zip(axes_list, lengths):
        span = max_check_bits(final, n)
        for index, (color, level) in enumerate(zip(SERIES_COLORS, levels_of(final.values(), channel))):
            plot_series(axes, collect_points(final, channel, level, n), color,
                        f"{LEVEL_NAMES[channel]} = {decimal_comma(level)}", offset=(index - 1) * 0.025 * span)
        axes.set_title(f"n = {n}", fontsize=10)
        style_axes(axes)
    finish_figure(figure, list(axes_list), f"Вероятность ложного приёма, {CHANNEL_TITLES[channel]}")
    figure.savefig(OUT / f"plozh_{channel.lower()}.png", dpi=200)
    plt.close(figure)


def plot_comparison(final):
    lengths = [15, 31, 63]
    figure, axes_list = plt.subplots(1, len(lengths), figsize=(8, 4.4), sharey=True)
    for axes, n in zip(axes_list, lengths):
        span = max_check_bits(final, n)
        plot_series(axes, collect_points(final, "BSC", "0.005", n), SERIES_COLORS[0], "ДСК, p = 0,005",
                    offset=-0.015 * span)
        plot_series(axes, collect_points(final, "Gilbert", "0.001", n), SERIES_COLORS[1],
                    "Гилберт, P_ХП = 0,001 (p = 0,00495)", linestyle="--", offset=0.015 * span)
        axes.set_title(f"n = {n}", fontsize=10)
        style_axes(axes)
    finish_figure(figure, list(axes_list), "ДСК и канал Гилберта при одинаковой средней вероятности ошибки")
    figure.savefig(OUT / "plozh_comparison.png", dpi=200)
    plt.close(figure)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    base = {key(row): row for row in read_rows("simulations.csv")}
    final = final_rows()
    result_tables(base, final)
    best_codes(final)
    theory_tables(base)
    weight_table()
    plot_channel(final, "BSC")
    plot_channel(final, "Gilbert")
    plot_comparison(final)
    print(f"Таблицы и графики сохранены в {OUT}")


if __name__ == "__main__":
    main()
