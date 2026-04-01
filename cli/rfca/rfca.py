
# TODO: Create consistent dependency environment (multiple libraries for 'serial')
# TODO: Make COM selection dynamic
# TODO: Error handling/communication
# TODO: Cleanup CLI UX
# TODO: Add debug/dev functionality from CLI
# TODO: Make GUI more legible (BIGGER = BETTER)
# TODO: Add Pwr up cmd
# TODO: Add Pwr down cmd
# TODO: Add 'help' command
# TODO: Move sweep data save loc to folder (not root dir)
# TODO: Progress reports from FW increase timeout so program doesn't terminate prematurely
# TODO: CMD Buffer (timeouts cause data desync)
# TODO: Write err(str) and warn(str) and info(str) functions to issue data to user in a more organized manner
# TODO: Subtle spell-casting (file_name and file_name.csv should be treated the same?)


# For FW interaction
import os
import sys
import json
import shlex
import serial
import time

# For GUI
from pathlib import Path
import pandas as pd
import pyqtgraph as pg
from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QTabWidget

# For communication w/ firmware.  NULL 'till main runs
ser = None
PORT = "COM4"
BAUD = 115200 # BAUD is irrelevant for virtual COM (I just chose 115200 because it's somewhat standard) 
connected = False

# List of possible commands
command_set = {"connect", "disconnect", "config", "calibrate", "sweep", "list", "retrieve", "delete", "clear", "cls", "freq", "port"}


##############################
###vv GUI data/functions vv###

'''
 * @brief Return the cal data for a given in/out port pair and a frequency
 * @param None
 * @return Dynamic GUI object
 '''
def initGUI():
    # Init GUI Object
    app = QApplication([])
    GUI = QMainWindow()
    GUI.setWindowTitle("RFCA GUI")
    GUI.resize(1200, 700)

    # Main widget
    central = QWidget()
    GUI.setCentralWidget(central)
    layout = QVBoxLayout(central)

    # Tabs
    GUI.tabs = QTabWidget()
    for i in range(1, 9):
        tab = QWidget()
        GUI.tabs.addTab(tab, f"Port {i}")

    # Central plot
    GUI.plot_widget = pg.PlotWidget()
    GUI.plot_widget.setBackground("w")
    GUI.plot_widget.showGrid(x=True, y=True, alpha=0.2)
    GUI.plot_widget.setLabel("bottom", "Frequency")
    GUI.plot_widget.setLabel("left", "Value")
    GUI.plot_widget.setTitle("Measurement Plot Area")
    GUI.plot_widget.addLegend()

    layout.addWidget(GUI.tabs, stretch=0)
    layout.addWidget(GUI.plot_widget, stretch=1)

    return GUI

# .csv data structure
CSV_COLUMNS = ["out_port", "frequency"] + [f"LA{i}" for i in range(10)] 
csv_path = None # Empty file path for now (global)
GUI = initGUI() # Global GUI object (runs indefinitely while CLI is running)

'''
 * @brief Plot the data for a given SP8T port on the GUI
 * @param port: 1-8 inclusive
 * @param port_data: Info. to be plotted
 * @return None
 '''
def plot_port_data(port: int, port_data):
    GUI.plot_widget.clear()
    GUI.plot_widget.addLegend()

    # Check which ports have no data
    if port not in port_data:
        GUI.plot_widget.setTitle(f"Port {port} (no data)")
        return

    df = port_data[port]
    freq = df["frequency"].to_numpy()

    GUI.plot_widget.setTitle(f"Port {port}")

    # Plot each LogAmp value vs. Frequency
    for i in range(10):
        col = f"LA{i}"
        GUI.plot_widget.plot(
            freq,
            df[col].to_numpy(),
            name=col,
            pen=pg.intColor(i, hues=10),
        )

'''
 * @brief Called when the port is switched on the GUI
 * @param index: 0-7 inclusive
 * @return None
 '''
def on_tab_changed(index: int):
    port = index + 1

    port_data = split_data_by_port(load_sweep_data(csv_path)) # csv_path is global  
    plot_port_data(port, port_data)

# For tab updates
GUI.tabs.currentChanged.connect(on_tab_changed)

'''
 * @brief Load loss information from csv and populate dataframe
 * @param csv_path: File path 
 * @return dataframe with loss information
 '''
def load_sweep_data(csv_path: Path) -> pd.DataFrame:
    df = pd.read_csv(csv_path)

    missing = [col for col in CSV_COLUMNS if col not in df.columns]
    if missing:
        raise ValueError(f"CSV is missing required columns: {missing}")

    df = df[CSV_COLUMNS].copy()
    df["out_port"] = df["out_port"].astype(int)
    df = df.sort_values(["out_port", "frequency"]).reset_index(drop=True)
    return df

'''
 * @brief Create a dictionary organizing loss information by port
 * @param df: Combined loss data
 * @return dictionary with organized loss info
 '''
def split_data_by_port(df: pd.DataFrame) -> dict[int, pd.DataFrame]:
    port_data = {}
    for port in sorted(df["out_port"].unique()):
        port_data[port] = df[df["out_port"] == port].copy()
    return port_data

###^^ GUI data/functions ^^###
##############################


#############################
####vvv CLI functions vvv####

'''
 * @brief Open connection to FW
 * @param PORT: COM port connected to FW
 * @param BAUD: Baud rate of communication, this value is irrelevant for virutal COM
 * @return ser: Serial object for FW communication
 '''
def connectFW(PORT, BAUD):
    global connected
    try:
        # Connect to Teensy
        print("\nConnecting to Firmware...")
        ser = serial.Serial(PORT, BAUD, timeout=1)
        time.sleep(2)
        connected = True
        print("Connected")
        return ser
    except Exception as e:
        print(f"[ERROR] {e}\n")
        return None


'''
 * @brief Close connection to FW and cleanup
 * @param ser: Serial object for FW communication
 * @return: None
 '''
def disconnectFW():
    global ser
    global connected
    
    print("Disconnecting from Firmware...")
    ser.flush() # Flush data in buffer
    ser.close()
    ser = None
    connected = False
    print("Disconnected")
    
'''
 * @brief Clear terminal depending on OS, handle error if unable
 * @return: None
 '''   
def clear_terminal():
    try:
        result = os.system('cls' if os.name == 'nt' else 'clear')

        if result != 0:
            print("(unable to clear)", file=sys.stderr)

    except Exception:
        print("(unable to clear)", file=sys.stderr)


'''
 * @brief Print prompts to user
 * @return: None
 ''' 
def print_rfca_header():
    print("=== RFCA CLI ===")
    print("Enter commands below")
    print("Type 'q' to quit\n")

'''
 * @brief Parses cmd and data from raw user CLI input
 * @param user_input: String w/ user input
 * @return cmd, data: parsed from string input
 '''
def parse_user_input(user_input):

    # Raw inputs are space-delimited
    parts = shlex.split(user_input) 

    # The first part of the input is always the command
    cmd = parts[0]

    # Check validity of commands
    if cmd not in command_set:
        print(f"[ERROR] Command \'{cmd}\' not recognized")
        return None, None


    if cmd == "clear" or cmd == "cls":
        clear_terminal()
        print_rfca_header()
        return None, None

    # If a command is issued while the firmware is disconnected, throw an error
    if cmd != "connect":
        if not connected:
            print("[ERROR] Not connected to firmware")
            return None, None

    # Next, based on the command, process further arguments into 'data' accordingly

    # 'config' needs to open the ./config.json file and store in 'data'
    if cmd == "config":
        try: 
            with open('./config.json', "r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception as e:
            print(f"Failed to read/parse JSON file: {e}")
            return None, None # TODO, check this error handling
        
    # 'calibrate' needs to store port info in 'data'
    elif cmd == "calibrate": 
        out_port = parts[1] # Must be between 1 and 8 inclusive
        in_port = parts[2]  # Must be between 1 and 10 inclusive
        set_cal = '-1'        # -1 by default indicates running typical calibration

        if((int)(out_port) < 1 or (int)(out_port) > 8):
            print(f"\n[WARN] output port out of range: {out_port} is not within [1, 8]")
            return None, None # TODO Error handling
        elif((int)(in_port) < 1 or (int)(in_port) > 10):
            print(f"\n[WARN] input port out of range: {in_port} is not within [1, 10]")
            return None, None # TODO Error handling

        # If a set_cal parameter is passed, set the cal value to that argument
        try:
            set_cal = parts[3] 
        except Exception as e:
            pass

        data = [out_port, in_port, set_cal]

    # 'sweep', 'retrieve', 'delete' each stores sweep name information into 'data'
    elif cmd == "sweep":
        data = parts[1]
    elif cmd == "retrieve":
        data = parts[1]
    elif cmd == "delete":
        data = parts[1]

    # 'list' has no data
    elif cmd == "list":
        data = None

    # Manual connect/disconnect commands
    elif cmd == "connect":
        global ser
        global PORT
        global BAUD
        ser = connectFW(PORT, BAUD)
        return None, None
    elif cmd == "disconnect":
        disconnectFW()
        return None, None
    
    # These are dev/debugging commands
    elif cmd == "freq": # {cmd: 'freq', data: 3500} # Set ADF output to 3500 MHz
        data = parts[1]
    elif cmd == "port":
        data = parts[1] # {cmd: 'port', data: 3} # Open a specific output port

    else:
        # This shouldn't happen because we check cmd validity at the beginning of this function
        return None, None

    # After processing, return both the command and its data
    return cmd, data

'''
 * @brief Send command and relevant data to firmware (after processing user input)
 * @param cmd: Issued command
 * @param data: Relevant data
 * @param ser: Serial object for FW communication
 * @return None
 '''
def sendJson(cmd, data, ser):
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


'''
 * @brief After sending JSON, this function waits for a response from firmware.
 *        It also prints cmd execution status (if relevant) and calls firmware response functions. 
 * @param ser: Serial object for FW communication
 * @param timeout - How long (seconds) to wait for FW response before terminating. 
 *                  15 seconds by default
 * @return None
 '''
def wait_for_response(ser, timeout=15):
    print("[INFO] Waiting firmware for response...")

    # To indicate status
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

                    # Below, we process the status codes from FW 

                    # 'ack' is sent by FW to acknowledge a received command.  FW sends this before beginning its processing
                    if msg.get("type") == "ack":
                        print(f'[INFO] Firmware has received the \'{msg.get("cmd")}\' command.')
                        continue

                    # 'complete' is sent by FW to indicate completion of command processing. 
                    elif msg.get("type") == "complete":
                        print(f'[INFO] Firmware has finished processing the \'{msg.get("cmd")}\' command.')
                        return
                    
                    # 'progress' is sent by FW to inform CLI of its current step in processing
                    elif msg.get("type") == "progress":
                        if(msg.get("cmd") == "sweep"):
                            print(f'[INFO] Measuring frequency: {msg.get("data").get("frequency")} MHz')
                        elif(msg.get("cmd") == "calibrate"):
                            print(f'[INFO] Calibrating for frequency: {msg.get("data").get("frequency")} MHz')

                    # Otherwise, process response
                    elif msg.get("type") == "data":
                        processResponse(msg)

                    else:
                        pass
                        # Error?

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
    print("\n[WARN] Timed out while waiting for response.\n")

'''
 * @brief Process errors or data sent by FW
 * @param response_data: JSON sent by FW
 * @return None
 '''
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

'''
 * @brief Process response data for all possible commands
 * @param cmd: Processed command
 * @param data: Processed data
 * @return None
 '''
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

    global ser
    global PORT
    global BAUD

    # Start on an empty screen
    clear_terminal()
    ser = connectFW(PORT, BAUD)
    print_rfca_header()

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

            # Only send JSON to FW if cmd is present
            if (cmd is not None):
                sendJson(cmd, data, ser)
                wait_for_response(ser, timeout=60) # 60 second time out
            else:
                pass

        except KeyboardInterrupt:
            print("\n[INFO] Interrupted. Type 'q' to quit.")
        except EOFError:
            print("\n[INFO] EOF received. Exiting.")
            break


if __name__ == "__main__":
    main()