VCC_CMD=${VCC_CMD:-}
VCC_PATH=${VCC_PATH:-}

format_bytes_mb() {
    awk -v bytes="$1" 'BEGIN { printf "%.0f", bytes / 1024 / 1024 }'
}

format_bytes_gb() {
    awk -v bytes="$1" 'BEGIN { printf "%.1f", bytes / 1024 / 1024 / 1024 }'
}

read_temperature_value() {
    if [[ -r /sys/class/thermal/thermal_zone0/temp ]]; then
        awk '{ printf "%.1f C", $1 / 1000 }' /sys/class/thermal/thermal_zone0/temp
    else
        printf 'n/a'
    fi
}

read_vcc_value() {
    local value path label input

    if [[ -n "$VCC_CMD" ]]; then
        run_with_timeout "$VCC_CMD" 2>/dev/null || printf 'n/a'
        return
    fi
    if [[ -n "$VCC_PATH" && -r "$VCC_PATH" ]]; then
        value=$(cat "$VCC_PATH")
        awk -v v="$value" 'BEGIN { if(v > 1000) printf "%.2f V", v / 1000000; else printf "%.2f V", v }'
        return
    fi
    if command -v vcgencmd >/dev/null 2>&1; then
        value=$(vcgencmd measure_volts 2>/dev/null | sed -n 's/^volt=//p')
        if [[ -n "$value" ]]; then
            printf '%s' "$value"
            return
        fi
    fi

    for path in /sys/class/power_supply/*/voltage_now; do
        [[ -r "$path" ]] || continue
        value=$(cat "$path")
        awk -v v="$value" 'BEGIN { printf "%.2f V", v / 1000000 }'
        return
    done

    for label in /sys/class/hwmon/hwmon*/in*_label; do
        [[ -r "$label" ]] || continue
        if grep -Eiq 'vcc|vin|5v|power|supply' "$label"; then
            input=${label%_label}_input
            if [[ -r "$input" ]]; then
                value=$(cat "$input")
                awk -v v="$value" 'BEGIN { printf "%.2f V", v / 1000 }'
                return
            fi
        fi
    done

    printf 'n/a'
}

read_cpu_percent() {
    local cpu user nice system idle iowait irq softirq steal guest guest_nice
    local idle1 total1 idle2 total2 diff_idle diff_total

    read -r cpu user nice system idle iowait irq softirq steal guest guest_nice < /proc/stat
    idle1=$((idle + iowait))
    total1=$((user + nice + system + idle + iowait + irq + softirq + steal))
    sleep 1
    read -r cpu user nice system idle iowait irq softirq steal guest guest_nice < /proc/stat
    idle2=$((idle + iowait))
    total2=$((user + nice + system + idle + iowait + irq + softirq + steal))
    diff_idle=$((idle2 - idle1))
    diff_total=$((total2 - total1))
    if (( diff_total <= 0 )); then
        printf 'n/a'
    else
        awk -v idle="$diff_idle" -v total="$diff_total" 'BEGIN { printf "%.0f%%", (100 * (total - idle)) / total }'
    fi
}

cmd_diag() {
    require_no_args "$@" || return 1

    local mem_total_kb mem_avail_kb mem_used_mb mem_total_mb
    local disk_used_b disk_total_b disk_used_gb disk_total_gb
    local cpu temp vcc

    mem_total_kb=$(awk '/^MemTotal:/ { print $2 }' /proc/meminfo)
    mem_avail_kb=$(awk '/^MemAvailable:/ { print $2 }' /proc/meminfo)
    mem_used_mb=$(((mem_total_kb - mem_avail_kb) / 1024))
    mem_total_mb=$((mem_total_kb / 1024))

    read -r disk_total_b disk_used_b < <(df -B1 --output=size,used / | awk 'NR == 2 { print $1, $2 }')
    disk_used_gb=$(format_bytes_gb "$disk_used_b")
    disk_total_gb=$(format_bytes_gb "$disk_total_b")

    cpu=$(read_cpu_percent)
    temp=$(read_temperature_value)
    vcc=$(read_vcc_value)

    reply "mem: ${mem_used_mb}/${mem_total_mb} MB
disk: ${disk_used_gb}/${disk_total_gb} GB
cpu: ${cpu}
temp: ${temp}
vcc: ${vcc}"
    log_msg "done command=diag"
}

register_command diag cmd_diag 1 diag
