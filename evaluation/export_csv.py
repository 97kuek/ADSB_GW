import json
import csv

def export_csv():
    try:
        with open("evaluation_average.json", "r") as f:
            data = json.load(f)
    except FileNotFoundError:
        print("evaluation_average.json not found.")
        return

    csv_file = "evaluation_results.csv"
    
    with open(csv_file, "w", newline='') as f:
        writer = csv.writer(f)
        
        # Header
        header = [
            "DB_Size", 
            "Avg_Exec_Time(sec)", 
            "Total_Candidates", 
            "Hist_Passed", 
            "Myers_Checked", 
            "Matched_Hamming", 
            "Matched_Myers",
            "Total_Matched"
        ]
        writer.writerow(header)
        
        for entry in data:
            stats = entry["avg_stats"]
            row = [
                entry["db_size"],
                f"{entry['avg_exec_time']:.6f}",
                f"{stats.get('TotalCand', 0):.0f}",
                f"{stats.get('HistPass', 0):.0f}",
                f"{stats.get('CheckMyers', 0):.0f}",
                f"{stats.get('MatchHam', 0):.0f}",
                f"{stats.get('MatchMyers', 0):.0f}",
                f"{stats.get('MatchHam', 0) + stats.get('MatchMyers', 0):.0f}"
            ]
            writer.writerow(row)
            
    print(f"Exported data to {csv_file}")

if __name__ == "__main__":
    export_csv()
