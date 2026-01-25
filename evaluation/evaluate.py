import subprocess
import os
import json
import time

def compile_code():
    print("Compiling...")
    subprocess.run(["gcc", "-O2", "-o", "prep_1", "src/prep_1.c", "-lm"], check=True)
    subprocess.run(["gcc", "-O2", "-o", "search_1", "src/search_1.c", "-lm"], check=True)
    print("Compilation finished.")

def parse_output(stderr_output):
    exec_time = 0.0
    stats = {}
    
    for line in stderr_output.splitlines():
        if "Exec Time:" in line:
            try:
                exec_time = float(line.split(":")[1].replace("sec", "").strip())
            except ValueError:
                pass
        if "STATS:" in line:
            # STATS: TotalCand=...
            parts = line.replace("STATS:", "").strip().split()
            for part in parts:
                key, val = part.split("=")
                stats[key] = int(val)
                
    return exec_time, stats

def run_evaluation():
    datasets = [1, 2, 3]
    steps = 10
    
    # Store aggregated results
    # { size: { "times": [], "stats": [ {stats_dict}, ... ] } }
    aggregated_data = {}

    for ds_num in datasets:
        db_file = f"test/db_{ds_num}"
        query_file = f"test/query_{ds_num}"
        print(f"\nProcessing Dataset {ds_num} ({db_file}, {query_file})")

        if not os.path.exists(db_file) or not os.path.exists(query_file):
            print(f"Error: Files not found for dataset {ds_num}")
            continue

        with open(db_file, "rb") as f:
            full_db = f.read()
            
        full_lines = len(full_db) // 16
        print(f"  Total DB lines: {full_lines}")
        
        step_size = full_lines // steps
        curr_subset_sizes = [step_size * i for i in range(1, steps + 1)]

        for size in curr_subset_sizes:
            print(f"  Evaluating DB size: {size}")
            
            temp_db_name = "temp_db.bin"
            temp_idx_name = "temp_index.bin"
            
            # Write subset DB
            with open(temp_db_name, "wb") as f:
                f.write(full_db[:size * 16])
            
            # Prep
            with open(temp_idx_name, "wb") as f:
                subprocess.run(["./prep_1", temp_db_name], stdout=f, check=True)
            
            # Search
            result = subprocess.run(
                ["./search_1", query_file, temp_idx_name],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                universal_newlines=True
            )
            
            exec_time, stats = parse_output(result.stderr)
            print(f"    Time: {exec_time}s")
            
            # Aggregate
            if size not in aggregated_data:
                aggregated_data[size] = {"times": [], "stats": []}
            
            aggregated_data[size]["times"].append(exec_time)
            aggregated_data[size]["stats"].append(stats)
            
            # Cleanup
            if os.path.exists(temp_db_name): os.remove(temp_db_name)
            if os.path.exists(temp_idx_name): os.remove(temp_idx_name)

    # Compute averages
    final_output = []
    for size, data in sorted(aggregated_data.items()):
        avg_time = sum(data["times"]) / len(data["times"])
        
        # Average stats (TotalCand, HistPass, etc.)
        avg_stats = {}
        if data["stats"]:
            keys = data["stats"][0].keys()
            for k in keys:
                total_val = sum(d.get(k, 0) for d in data["stats"])
                avg_stats[k] = total_val / len(data["stats"])
        
        final_output.append({
            "db_size": size,
            "avg_exec_time": avg_time,
            "avg_stats": avg_stats,
            "raw_times": data["times"] # Keep raw for potential variance plot
        })

    with open("evaluation_average.json", "w") as f:
        json.dump(final_output, f, indent=4)
    print("\nEvaluation complete. Saved to evaluation_average.json")

if __name__ == "__main__":
    compile_code()
    run_evaluation()
