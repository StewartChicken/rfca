
# TODO: Create consistent dependency environment (multiple libraries for 'serial')

# For FW interaction
import os
import json
import shlex
import serial
import time
import sys

# For GUI
from pathlib import Path

import pandas as pd
import pyqtgraph as pg

from PySide6.QtWidgets import (
    QApplication, 
    QMainWindow, 
    QWidget, 
    QVBoxLayout, 
    QTabWidget,
    QHBoxLayout,
    QCheckBox,
    QLabel,
    QScrollArea,
    QFrame,
)

# TODO: Make COM selection dynamic
PORT = "COM4"
BAUD = 115200 # BAUD is irrelevant for virtual COM (I just chose 115200 because it's somewhat standard) 

# Connect to Teensy
print("Connecting to Firmware...")
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)


##############################
##### GUI data/functions #####

# For dynamic legend
class ColorSwatch(QFrame):
    def __init__(self, color):
        super().__init__()
        self.setFixedSize(14, 14)

        # Convert pyqtgraph color → Qt color
        qcolor = pg.mkColor(color)

        self.setStyleSheet(f"""
            background-color: {qcolor.name()};
            border: 1px solid black;
        """)

def initGUI():
    # Init GUI
    app = QApplication.instance() or QApplication([])
    GUI = QMainWindow()
    GUI.setWindowTitle("RFCA GUI")
    GUI.resize(1400, 800)

    # Main widget
    central = QWidget()
    GUI.setCentralWidget(central)
    main_layout = QVBoxLayout(central)

    # Tabs
    GUI.tabs = QTabWidget()
    for i in range(1, 9):
        tab = QWidget()
        GUI.tabs.addTab(tab, f"Port {i}")

    # Plot + legend row
    content_row = QHBoxLayout()

    # Central plot
    GUI.plot_widget = pg.PlotWidget()
    GUI.plot_widget.setBackground("w")
    GUI.plot_widget.showGrid(x=True, y=True, alpha=0.2)
    GUI.plot_widget.setLabel("bottom", "Frequency")
    GUI.plot_widget.setLabel("left", "Value")
    GUI.plot_widget.setTitle("Measurement Plot Area")

    # Custom legend panel
    GUI.legend_container = QWidget()
    GUI.legend_layout = QVBoxLayout(GUI.legend_container)
    GUI.legend_layout.setContentsMargins(8, 8, 8, 8)
    GUI.legend_layout.setSpacing(6)

    legend_title = QLabel("Traces")
    GUI.legend_layout.addWidget(legend_title)

    GUI.legend_scroll = QScrollArea()
    GUI.legend_scroll.setWidgetResizable(True)

    GUI.legend_content = QWidget()
    GUI.legend_content_layout = QVBoxLayout(GUI.legend_content)
    GUI.legend_content_layout.setContentsMargins(4, 4, 4, 4)
    GUI.legend_content_layout.setSpacing(4)

    GUI.legend_scroll.setWidget(GUI.legend_content)
    GUI.legend_layout.addWidget(GUI.legend_scroll)

    # Store currently plotted items + controls
    GUI.trace_items = {}
    GUI.trace_checkboxes = {}

    content_row.addWidget(GUI.plot_widget, stretch=1)
    content_row.addWidget(GUI.legend_container, stretch=0)

    main_layout.addWidget(GUI.tabs, stretch=0)
    main_layout.addLayout(content_row, stretch=1)

    return GUI

def clear_layout(layout):
    while layout.count():
        item = layout.takeAt(0)
        widget = item.widget()
        child_layout = item.layout()

        if widget is not None:
            widget.deleteLater()
        elif child_layout is not None:
            clear_layout(child_layout)


def rebuild_legend():
    clear_layout(GUI.legend_content_layout)
    GUI.trace_checkboxes.clear()

    if not GUI.trace_items:
        label = QLabel("No traces")
        GUI.legend_content_layout.addWidget(label)
        GUI.legend_content_layout.addStretch()
        return

    for name, (plot_item, color) in GUI.trace_items.items():

        row = QWidget()
        row_layout = QHBoxLayout(row)
        row_layout.setContentsMargins(2, 2, 2, 2)
        row_layout.setSpacing(6)

        # Color box
        swatch = ColorSwatch(color)

        # Checkbox
        checkbox = QCheckBox(name)
        checkbox.setChecked(True)

        def make_toggle(item):
            def toggle(state):
                item.setVisible(bool(state))
            return toggle

        checkbox.toggled.connect(make_toggle(plot_item))
        GUI.trace_checkboxes[name] = checkbox

        row_layout.addWidget(swatch)
        row_layout.addWidget(checkbox)
        row_layout.addStretch()

        GUI.legend_content_layout.addWidget(row)

    GUI.legend_content_layout.addStretch()

CSV_COLUMNS = ["out_port", "frequency"] + [f"LA{i}" for i in range(10)] # data .csv structure
csv_path = None # Empty file path for now (global)
GUI = initGUI()

# Port tab switch functions
def plot_port_data(port: int, port_data):
    GUI.plot_widget.clear()
    GUI.trace_items.clear()

    if port not in port_data:
        GUI.plot_widget.setTitle(f"Port {port} (no data)")
        rebuild_legend()
        return

    df = port_data[port]
    freq = df["frequency"].to_numpy()

    GUI.plot_widget.setTitle(f"Port {port}")

    for i in range(10):
        col = f"LA{i}"
        color = pg.intColor(i, hues=10)

        item = GUI.plot_widget.plot(
            freq,
            df[col].to_numpy(),
            name=col,
            pen=color,
        )

        # Store BOTH item + color
        GUI.trace_items[col] = (item, color)

    rebuild_legend()

def on_tab_changed(index: int):
    port = index + 1

    port_data = split_data_by_port(load_sweep_data(csv_path)) # csv_path is global  
    plot_port_data(port, port_data)

# For tab updates
GUI.tabs.currentChanged.connect(on_tab_changed)

def load_sweep_data(csv_path: Path) -> pd.DataFrame:
    df = pd.read_csv(csv_path)

    missing = [col for col in CSV_COLUMNS if col not in df.columns]
    if missing:
        raise ValueError(f"CSV is missing required columns: {missing}")

    df = df[CSV_COLUMNS].copy()
    df["out_port"] = df["out_port"].astype(int)
    df = df.sort_values(["out_port", "frequency"]).reset_index(drop=True)
    return df


def split_data_by_port(df: pd.DataFrame) -> dict[int, pd.DataFrame]:
    port_data = {}
    for port in sorted(df["out_port"].unique()):
        port_data[port] = df[df["out_port"] == port].copy()
    return port_data

###^^ GUI functions ^^###
#########################



# CLI input
def parse_user_input(user_input):
    parts = shlex.split(user_input) # Space-delimited

    cmd = parts[0]

    if cmd == "config":
        try: 
            with open('./config.json', "r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception as e:
            print(f"Failed to read/parse JSON file: {e}")
            return None, None # TODO, check this error handling
    elif cmd == "calibrate": 
        out_port = parts[1] # Must be between 1 and 8 inclusive
        in_port = parts[2]  # Must be between 1 and 10 inclusive
        data = [out_port, in_port]
    elif cmd == "sweep":
        data = parts[1]
    elif cmd == "list":
        data = None
    elif cmd == "retrieve":
        data = parts[1]
    elif cmd == "delete":
        data = parts[1]
    else:
        data = None


    return cmd, data

# Send JSON command data to firmware
def sendJson(cmd, data):
    # Create JSON structure
    envelope = {
        "cmd": cmd,
        "data": data
    }

    # Generate string literal with '\n' appened to indicate termination
    body = (json.dumps(envelope, separators=(",", ":")) + "\n")

    # Write to USB
    ser.write(body.encode("utf-8"))
    ser.flush()


# 15 second timeout by default
def wait_for_response(timeout=15):
    print("[INFO] Waiting firmware for response...")

    spinner = ["|", "/", "-", "\\"]
    spinner_idx = 0
    
    start_time = time.time()

    while (time.time() - start_time) < timeout:
        # Read and process anything currently available
        if ser.in_waiting > 0:
            try:
                line = ser.readline().decode("utf-8", errors="ignore").strip()

                if not line:
                    continue
                
                # For debugging
                #print(f"[FW] {line}")

                # Process as JSON
                try:
                    msg = json.loads(line)

                    # Process 'ack' and 'complete' codes from firmware
                    if msg.get("type") == "ack":
                        print(f'[INFO] Firmware has received the \'{msg.get("cmd")}\' command.')
                        continue
                    elif msg.get("type") == "complete":
                        print(f'[INFO] Firmware has finished processing the \'{msg.get("cmd")}\' command.')
                        return
                    elif msg.get("type") == "progress":
                        print(f'[INFO] Wrote frequency: {msg.get("data").get("frequency")}')

                    # Otherwise, process response
                    processResponse(msg)

                # String was not JSON format
                except json.JSONDecodeError:
                    # Non-JSON firmware output is still shown, just ignored structurally
                    pass

            except Exception as e:
                print(f"[ERROR] Failed to read serial response: {e}")

        # Spinner while waiting
        #print(f"\r[INFO]{spinner[spinner_idx % 4]}", end="", flush=True)
        #spinner_idx += 1
        time.sleep(0.1)

    # Timeout warning
    print("\n[WARN] Response wait timed out.\n")

# response_data is JSON
def processResponse(response_data):

    # Handle errors returned by the firmware
    if (response_data.get("status") != "OK"):
        processError() # TODO - this function is a blank placeholder
        return
    
    # If no errors, process the response data based on CMD type
    if (response_data.get("type") == "data"):
        cmd = response_data.get("cmd")
        data = response_data.get("data")
        processData(cmd, data)
        return


# Process response data from all possible commands
def processData(cmd, data):
    if(cmd == "config"):
        config_params = data.get("config_params")
        print('[INFO] Configured RFCA with the following parameters:')
        print(f'[INFO] {config_params}')
    elif(cmd == "calibrate"):
        cal_data = data.get("cal_data")
        print('[INFO] Recorded the following calibration data')
        print(f'[INFO] {cal_data}')
    elif(cmd == "sweep"):
        config_params = data.get("config_params")
        print("[INFO] Completed sweep with the following parameters:")
        print(f'[INFO] {config_params}')
    elif(cmd == "list"):
        files = data.get("files")

        if(len(files) == 0):
            print("[INFO] Found 0 files")
        else:
            print(f"[INFO] Found {len(files)} file(s): \n")
            for file in files:
                print(file)

            print('\n')

    # Retrieve a .csv file from the firmware, save it locally and display its data
    elif(cmd == "retrieve"):
        sweep_name = data.get("sweep_name")
        sweep_data = data.get("sweep_data")
        
        # Get directory of this script
        script_dir = os.path.dirname(os.path.abspath(__file__))

        # Construct file path
        file_path = os.path.join(script_dir, f"{sweep_name}.csv")

        try:
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(sweep_data)

            print(f"[INFO] Saved sweep to: {file_path}")

        except Exception as e:
            print(f"[ERROR] Failed to save CSV: {e}")

        # Use the global csv_path, not a local instance
        global csv_path
        csv_path = Path(f"{sweep_name}.csv")
        on_tab_changed(0) # Set initial tab (0-7)
        GUI.show()

    elif(cmd == "delete"): 
        sweep_name = data.get("sweep_name")
        print(f"[INFO] Deleted {sweep_name} from the firmware")
    else:
        # This shouldn't happen TODO: Throw error?
        print("[ERROR] Unrecognized CMD")
        print(cmd)

# TODO
def processError():
    print("Error detected")



def main():

    # CLI User Prompts
    print("=== RFCA CLI ===")
    print("Enter commands below")
    print("Type 'q' to quit\n")

    while True:
        try:
            # Prompt user
            user_input = input("rfca>> ").strip()

            # Quit condition
            if user_input.lower() in ["q", "quit", "exit"]:
                print("Exiting RFCA CLI...")
                break

            # Ignore empty input
            if not user_input:
                continue
            
            # Parse terminal input, package and send to firmware, wait till response or time out
            cmd, data = parse_user_input(user_input)
            sendJson(cmd, data)
            wait_for_response(timeout=60) # 60 second time out
            
        except KeyboardInterrupt:
            print("\n[INFO] Interrupted. Type 'q' to quit.")
        except EOFError:
            print("\n[INFO] EOF received. Exiting.")
            break


if __name__ == "__main__":
    main()