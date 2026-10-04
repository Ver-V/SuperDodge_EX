"""풀 사용 / 미사용 프레임 로그 비교.

각 인자는 한 그룹이며 glob 패턴을 쓸 수 있음. 그룹 안의 여러 실행 결과를 합쳐서 집계함.

사용법 (SuperDodge 폴더에서):
    python ../tools/frame_stats.py "frame_pool_*.csv" "frame_nopool_*.csv"
    python ../tools/frame_stats.py --boss "frame_pool_*.csv" "frame_nopool_*.csv"

--boss: 보스 탄막 구간(bossActive == 1) 프레임만 집계
"""

import csv
import glob
import sys


def percentile(sorted_values, p):
    if not sorted_values:
        return 0.0
    index = min(len(sorted_values) - 1, int(round(p / 100.0 * (len(sorted_values) - 1))))
    return sorted_values[index]


def load_rows(path, boss_only):
    rows = []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            if not row.get("cpuMs") or not row.get("bossActive"):
                continue  # 강제 종료로 잘린 마지막 줄
            if boss_only and row["bossActive"] != "1":
                continue
            rows.append(row)
    # 첫 프레임은 초기화 시간이 섞이므로 제외
    return rows[1:]


def summarize(pattern, boss_only):
    paths = sorted(glob.glob(pattern))
    if not paths:
        return None

    rows = []
    run_cpu_avgs = []
    for path in paths:
        run_rows = load_rows(path, boss_only)
        if not run_rows:
            continue
        rows.extend(run_rows)
        run_cpu_avgs.append(sum(float(r["cpuMs"]) for r in run_rows) / len(run_rows))

    if not rows:
        return None

    frame_ms = sorted(float(r["frameMs"]) for r in rows)
    cpu_ms = sorted(float(r["cpuMs"]) for r in rows)
    created = sum(int(r["created"]) for r in rows)
    destroyed = sum(int(r["destroyed"]) for r in rows)
    seconds = sum(frame_ms) / 1000.0

    return {
        "runs": len(run_cpu_avgs),
        "frames": len(rows),
        "seconds": seconds,
        "frame_avg": sum(frame_ms) / len(frame_ms),
        "frame_p99": percentile(frame_ms, 99),
        "cpu_avg": sum(cpu_ms) / len(cpu_ms),
        "cpu_p99": percentile(cpu_ms, 99),
        "cpu_run_min": min(run_cpu_avgs),
        "cpu_run_max": max(run_cpu_avgs),
        "created_per_sec": created / seconds if seconds > 0 else 0.0,
        "destroyed_per_sec": destroyed / seconds if seconds > 0 else 0.0,
        "max_active": max(int(r["activeObjects"]) for r in rows),
        "max_total": max(int(r["totalObjects"]) for r in rows),
    }


def main():
    args = sys.argv[1:]
    boss_only = "--boss" in args
    patterns = [a for a in args if a != "--boss"]
    if not patterns:
        print(__doc__)
        return

    labels = [
        ("runs", "실행 횟수", "{:.0f}"),
        ("frames", "프레임 수", "{:.0f}"),
        ("seconds", "측정 시간(s)", "{:.1f}"),
        ("frame_avg", "프레임 평균(ms)", "{:.3f}"),
        ("frame_p99", "프레임 상위1%(ms)", "{:.3f}"),
        ("cpu_avg", "CPU 평균(ms)", "{:.3f}"),
        ("cpu_p99", "CPU 상위1%(ms)", "{:.3f}"),
        ("cpu_run_min", "실행별 CPU평균 최소", "{:.3f}"),
        ("cpu_run_max", "실행별 CPU평균 최대", "{:.3f}"),
        ("created_per_sec", "생성/초", "{:.1f}"),
        ("destroyed_per_sec", "삭제/초", "{:.1f}"),
        ("max_active", "최대 활성 오브젝트", "{:.0f}"),
        ("max_total", "최대 전체 오브젝트", "{:.0f}"),
    ]

    results = [(p, summarize(p, boss_only)) for p in patterns]
    print("구간:", "보스 탄막만" if boss_only else "전체")
    print("{:<22}".format("") + "".join("{:>22}".format(p.replace("\\", "/").split("/")[-1]) for p, _ in results))
    for key, name, fmt in labels:
        line = "{:<22}".format(name)
        for _, s in results:
            line += "{:>22}".format(fmt.format(s[key]) if s else "-")
        print(line)


if __name__ == "__main__":
    main()
