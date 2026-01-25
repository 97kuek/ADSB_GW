import json

def create_svg(width, height, content):
    return f'<svg width="{width}" height="{height}" xmlns="http://www.w3.org/2000/svg" style="background-color:white; font-family: \'Hiragino Kaku Gothic ProN\', \'Yu Gothic\', \'Meiryo\', sans-serif;">{content}</svg>'

def create_avg_time_graph():
    try:
        with open("evaluation_average.json", "r") as f:
            data = json.load(f)
    except FileNotFoundError:
        return

    db_sizes = [d["db_size"] for d in data]
    avg_times = [d["avg_exec_time"] for d in data]

    w, h = 800, 500
    pad = 80
    
    max_x = max(db_sizes)
    max_y = max(avg_times) * 1.15

    def sx(x): return pad + (x / max_x) * (w - 2 * pad)
    def sy(y): return h - pad - (y / max_y) * (h - 2 * pad)

    svg = ""
    # Title
    svg += f'<text x="{w/2}" y="{pad/2}" font-size="22" text-anchor="middle" font-weight="bold" fill="#333">データサイズごとの平均実行時間 (3セット平均)</text>'
    
    # Grid lines
    # Y-axis grid
    ticks_y = 5
    for i in range(ticks_y + 1):
        val = max_y * i / ticks_y
        y_pos = sy(val)
        svg += f'<line x1="{pad}" y1="{y_pos}" x2="{w-pad}" y2="{y_pos}" stroke="#e0e0e0" stroke-width="1"/>'
        svg += f'<text x="{pad-10}" y="{y_pos+5}" font-size="12" text-anchor="end" fill="#666">{val:.1f}s</text>'
    
    # X-axis grid
    ticks_x = 5
    for i in range(ticks_x + 1):
        val = max_x * i / ticks_x
        x_pos = sx(val)
        svg += f'<line x1="{x_pos}" y1="{pad}" x2="{x_pos}" y2="{h-pad}" stroke="#e0e0e0" stroke-width="1" stroke-dasharray="4"/>'
        label = f"{int(val/10000)}万" if val > 0 else "0"
        svg += f'<text x="{x_pos}" y="{h-pad+25}" font-size="12" text-anchor="middle" fill="#666">{label}</text>'

    # Axes
    svg += f'<line x1="{pad}" y1="{h-pad}" x2="{w-pad}" y2="{h-pad}" stroke="#333" stroke-width="2"/>'
    svg += f'<line x1="{pad}" y1="{h-pad}" x2="{pad}" y2="{pad}" stroke="#333" stroke-width="2"/>'
    
    # Axis Titles
    svg += f'<text x="{w/2}" y="{h-20}" font-size="14" text-anchor="middle" fill="#333">データベース行数</text>'
    svg += f'<text x="25" y="{h/2}" font-size="14" text-anchor="middle" transform="rotate(-90 25,{h/2})" fill="#333">実行時間 (秒)</text>'

    # Plot Line
    points = " ".join([f"{sx(x)},{sy(y)}" for x, y in zip(db_sizes, avg_times)])
    svg += f'<polyline points="{points}" fill="none" stroke="#007AFF" stroke-width="3"/>'

    # Points
    for x, y in zip(db_sizes, avg_times):
        svg += f'<circle cx="{sx(x)}" cy="{sy(y)}" r="5" fill="#fff" stroke="#007AFF" stroke-width="2"/>'
        svg += f'<text x="{sx(x)}" y="{sy(y)-15}" font-size="11" text-anchor="middle" fill="#333" font-weight="bold">{y:.2f}s</text>'

    with open("avg_time_graph.svg", "w") as f:
        f.write(create_svg(w, h, svg))
    print("Saved avg_time_graph.svg")

def create_avg_filter_graph():
    try:
        with open("evaluation_average.json", "r") as f:
            data = json.load(f)
    except FileNotFoundError:
        return

    # Use the largest dataset size (last entry) for stats
    last_entry = data[-1]
    stats = last_entry["avg_stats"]
    
    # TotalCand, HistPass, MatchMyers (match result)
    # CheckMyers is arguably "Candidates after Hamming" if Hamming didn't strictly filter but just checked.
    # In my code: Hist -> Hamming -> Myers.
    # Hamming is very cheap, so maybe group it?
    # Actually, let's show key stages:
    # 1. Total Candidates (Bucket retrieval)
    # 2. Passed Histogram (Expensive check candidates)
    # 3. Passed Hamming (Passed cheap bitwise check) -> Input to Myers
    # 4. Final Match
    
    stages = [
        {"label": "検索候補(全バケット)", "val": stats.get("TotalCand", 0), "color": "#B0C4DE"},
        {"label": "ヒストグラム通過", "val": stats.get("HistPass", 0), "color": "#87CEFA"},
        {"label": "Myers詳細計算", "val": stats.get("CheckMyers", 0), "color": "#4682B4"},
        {"label": "最終一致", "val": stats.get("MatchHam", 0) + stats.get("MatchMyers", 0), "color": "#32CD32"}
    ]

    
    w, h = 800, 500
    pad = 80
    max_val = stages[0]["val"] * 1.1

    def sy(y): return h - pad - (y / max_val) * (h - 2 * pad)
    
    svg = ""
    svg += f'<text x="{w/2}" y="{pad/2}" font-size="22" text-anchor="middle" font-weight="bold" fill="#333">フィルタリング効率 (3セット平均)</text>'
    
    # Axes
    svg += f'<line x1="{pad}" y1="{h-pad}" x2="{w-pad}" y2="{h-pad}" stroke="#333" stroke-width="2"/>'
    
    bar_w = 100
    spacing = (w - 2 * pad - len(stages) * bar_w) / (len(stages) + 1)
    
    for i, item in enumerate(stages):
        x = pad + spacing + i * (bar_w + spacing)
        y = sy(item["val"])
        bh = h - pad - y
        
        svg += f'<rect x="{x}" y="{y}" width="{bar_w}" height="{bh}" fill="{item["color"]}" rx="4" ry="4"/>'
        
        # Value Label
        val_str = f"{item['val']/1000000:.1f}M"
        if item['val'] > 1000000000:
            val_str = f"{item['val']/1000000000:.2f}B"
        elif item['val'] < 1000000:
            val_str = f"{item['val']/1000:.1f}K"
            
        svg += f'<text x="{x+bar_w/2}" y="{y-10}" font-size="14" text-anchor="middle" font-weight="bold" fill="#333">{val_str}</text>'
        
        # Label (wrap if needed)
        svg += f'<text x="{x+bar_w/2}" y="{h-pad+25}" font-size="13" text-anchor="middle" fill="#333">{item["label"]}</text>'
        
        # Percentage annotation
        if i > 0:
            prev = stages[i-1]["val"]
            curr = item["val"]
            if i == 3: # Special case for match, compare to Myers? Or just skip
                pass
            else:
                pct = (1 - curr/prev) * 100
                svg += f'<text x="{x - spacing/2}" y="{sy(prev) + (sy(curr)-sy(prev))/2}" font-size="12" text-anchor="middle" fill="#d32f2f">▼{pct:.1f}%</text>'

    with open("avg_filter_graph.svg", "w") as f:
        f.write(create_svg(w, h, svg))
    print("Saved avg_filter_graph.svg")

if __name__ == "__main__":
    create_avg_time_graph()
    create_avg_filter_graph()
