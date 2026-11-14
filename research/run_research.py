import argparse
import csv
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from estimates import verdict

ROOT = Path(__file__).resolve().parent.parent
EXE = ".exe" if os.name == "nt" else ""
SIMULATOR = ROOT / "build" / f"crcsim{EXE}"
ENUMERATOR = ROOT / "build" / f"weight_enumerator{EXE}"
DATA = ROOT / "research" / "data"

BSC_LEVELS = [0.05, 0.01, 0.005]
GILBERT_LEVELS = [0.003, 0.001, 0.0005]
GILBERT_FIXED = {"p_bg": 0.1, "p_bad": 0.5, "p_good": 0.0}


def run(arguments):
    completed = subprocess.run(arguments, capture_output=True, text=True, encoding="utf-8", check=True)
    return completed.stdout


def known_codes():
    lines = run([str(SIMULATOR), "--list-codes"]).strip().splitlines()
    return list(csv.DictReader(lines, delimiter=";"))


def channel_points():
    points = [{"channel": "bsc", "level": p, "args": ["--channel", "bsc", "--p", str(p)]} for p in BSC_LEVELS]
    for p_gb in GILBERT_LEVELS:
        args = ["--channel", "gilbert", "--p-gb", str(p_gb), "--p-bg", str(GILBERT_FIXED["p_bg"]),
                "--p-bad", str(GILBERT_FIXED["p_bad"]), "--p-good", str(GILBERT_FIXED["p_good"])]
        points.append({"channel": "gilbert", "level": p_gb, "args": args})
    return points


def simulate(code, point, blocks, seed):
    arguments = [str(SIMULATOR), "--n", code["n"], "--g", code["g"], *point["args"],
                 "--blocks", str(blocks), "--seed", str(seed), "--csv-stdout"]
    header, row = run(arguments).strip().splitlines()
    record = dict(zip(header.split(";"), row.split(";")))
    record["level"] = str(point["level"])
    record["t"] = code["t"]
    return record


def weight_histogram(n, g, point, blocks, seed):
    arguments = [str(SIMULATOR), "--n", n, "--g", g, *point["args"], "--blocks", str(blocks),
                 "--seed", str(seed), "--quiet"]
    histogram = {}
    for match in re.finditer(r"j =\s*(\d+):\s*(\d+)", run(arguments)):
        histogram[int(match.group(1))] = int(match.group(2))
    return histogram


def write_rows(path, rows, fields):
    with open(path, "w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=fields, delimiter=";")
        writer.writeheader()
        writer.writerows(rows)


def collect_simulations(codes, blocks, seed, workers):
    tasks = [(code, point) for point in channel_points() for code in codes]
    with ThreadPoolExecutor(max_workers=workers) as pool:
        rows = list(pool.map(lambda task: simulate(task[0], task[1], blocks, seed), tasks))
    fields = ["level", "t", "n", "k", "g", "channel", "p", "N", "Nv", "Nobn", "Nlozh", "Pv", "Pobn", "Plozh", "I"]
    write_rows(DATA / "simulations.csv", rows, fields)
    return rows


def collect_weight_spectra(codes, workers):
    def spectrum(code):
        kind, *counts = run([str(ENUMERATOR), code["n"], code["g"]]).strip().split(";")
        return {"n": code["n"], "g": code["g"], "kind": kind, "counts": " ".join(counts)}

    with ThreadPoolExecutor(max_workers=workers) as pool:
        rows = list(pool.map(spectrum, codes))
    write_rows(DATA / "weight_spectra.csv", rows, ["n", "g", "kind", "counts"])


def collect_histograms(codes, blocks, seed, workers):
    first_code_per_length = {}
    for code in codes:
        first_code_per_length.setdefault(code["n"], code)
    tasks = [(code, point) for point in channel_points() for code in first_code_per_length.values()]

    def histogram(task):
        code, point = task
        counts = weight_histogram(code["n"], code["g"], point, blocks, seed)
        return [{"channel": point["channel"], "level": point["level"], "n": code["n"], "j": j, "count": count}
                for j, count in sorted(counts.items())]

    with ThreadPoolExecutor(max_workers=workers) as pool:
        rows = [row for rows in pool.map(histogram, tasks) for row in rows]
    write_rows(DATA / "error_weights.csv", rows, ["channel", "level", "n", "j", "count"])


def refine_borderline(codes, rows, blocks, seed, workers):
    codes_by_key = {(code["n"], code["g"]): code for code in codes}
    points = {(point["channel"].upper() if point["channel"] == "bsc" else "Gilbert", str(point["level"])): point
              for point in channel_points()}
    tasks = [(codes_by_key[(row["n"], row["g"])], points[(row["channel"], row["level"])])
             for row in rows if verdict(int(row["Nlozh"]), int(row["N"])) == "borderline"]
    with ThreadPoolExecutor(max_workers=workers) as pool:
        refined = list(pool.map(lambda task: simulate(task[0], task[1], blocks, seed), tasks))
    fields = ["level", "t", "n", "k", "g", "channel", "p", "N", "Nv", "Nobn", "Nlozh", "Pv", "Pobn", "Plozh", "I"]
    write_rows(DATA / "refined.csv", refined, fields)
    return refined


def replicate_point(n, g, p, blocks, seeds, workers):
    point = {"channel": "bsc", "level": p, "args": ["--channel", "bsc", "--p", str(p)]}
    code = {"n": str(n), "g": g, "t": ""}
    with ThreadPoolExecutor(max_workers=workers) as pool:
        rows = list(pool.map(lambda seed: {"seed": seed, **simulate(code, point, blocks, seed)}, seeds))
    fields = ["seed", "level", "t", "n", "k", "g", "channel", "p", "N", "Nv", "Nobn", "Nlozh", "Pv", "Pobn", "Plozh", "I"]
    write_rows(DATA / "replicates.csv", rows, fields)


def main():
    parser = argparse.ArgumentParser(description="Пакетный прогон исследования циклических кодов")
    parser.add_argument("--blocks", type=int, default=3000000)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--refine-blocks", type=int, default=30000000)
    parser.add_argument("--refine-seed", type=int, default=43)
    parser.add_argument("--workers", type=int, default=os.cpu_count() or 1)
    options = parser.parse_args()
    if not SIMULATOR.exists() or not ENUMERATOR.exists():
        sys.exit("Сначала соберите программы: make research-tools")
    DATA.mkdir(parents=True, exist_ok=True)
    codes = known_codes()
    collect_weight_spectra(codes, options.workers)
    collect_histograms(codes, options.blocks, options.seed, options.workers)
    rows = collect_simulations(codes, options.blocks, options.seed, options.workers)
    refined = refine_borderline(codes, rows, options.refine_blocks, options.refine_seed, options.workers)
    replicate_point(7, "1011", 0.01, options.blocks, range(1, 9), options.workers)
    print(f"Готово: {len(rows)} точек, уточнено {len(refined)}, результаты в {DATA}")


if __name__ == "__main__":
    main()
