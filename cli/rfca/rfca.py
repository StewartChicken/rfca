
# TODO: ERROR HANDLING
# TODO: Organize and comment functions in files
# TODO: Write documentation


# For FW interaction
import os
import sys
import json
import shlex
import serial
from serial.tools import list_ports
import time

# For GUI
from pathlib import Path
import pandas as pd
import pyqtgraph as pg
from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QTabWidget

# For communication w/ firmware.  NULL 'till main runs
ser = None
connected = False # Connected to FW?

# List of possible commands
command_set = {
                "config": {
                    "description": "Configure RFCA sweep parameters in firmware using the config.json file in this directory",
                    "usage": "config",
                    "arguments": [],
                    "examples": ["config"]
                },
                "calibrate": {
                    "description": "Calibrate an RFCA output/input pair.",
                    "usage": "calibrate <out_port> <in_port> [set]",
                    "arguments": ["<out_port>    Output port number (1-8)", 
                                  "<in_port>     Input port number (1-10)", 
                                  "[set]         Optional calibration value. If omitted, value is measured from ADC"],
                    "examples": ["calibrate 1 10", 
                                 "calibrate 4 1", 
                                 "calibrate 1 1 0"]
                }, 
                "sweep": {
                    "description": "Begin a frequency sweep and measure loss data according to firmware config",
                    "usage": "sweep <sweep_name>",
                    "arguments": ["<sweep_name>    Name of sweep data as it will be saved in the firmware"],
                    "examples": ["sweep my_cool_sweep"]
                }, 
                "list": {
                    "description": "Print all sweep data files contained in firmware",
                    "usage": "list",
                    "arguments": [],
                    "examples": ["list"]
                }, 
                "retrieve": {
                    "description": "Retrieve a sweep file from firmware and save locally. ",
                    "usage": "retrieve <sweep_name>",
                    "arguments": ["<sweep_name>    Name of the sweep data as it will be searched for in the firmware"],
                    "examples": ["retrieve my_cool_sweep"]
                }, 
                "delete": {
                    "description": "Delete a sweep file from firmware",
                    "usage": "delete <sweep_name>",
                    "arguments": ["<sweep_name>    Name of sweep data file to be deleted"],
                    "examples": ["delete my_cool_sweep" ] 
                }, 
                "connect": {
                    "description": "Attempt connection to firmware",
                    "usage": "connect",
                    "arguments": [],
                    "examples": ["connect"]
                }, 
                "disconnect": {
                    "description": "Disconnect from firmware",
                    "usage": "disconnect",
                    "arguments": [],
                    "examples": ["disconnect"]
                }, 
                "boot": {
                    "description": "Initialize RFCA hardware peripherals",
                    "usage": "boot",
                    "arguments": [],
                    "examples": ["boot"]
                }, 
                "shutdown": {
                    "description": "Shutdown RFCA hardware peripherals",
                    "usage": "shutdown",
                    "arguments": [],
                    "examples": ["shutdown"]
                }, 
                "clear": {
                    "description": "Clear RFCA console",
                    "usage": "clear",
                    "arguments": [],
                    "examples": ["clear"]
                }, 
                "cls": {
                    "description": "Clear RFCA console",
                    "usage": "clear",
                    "arguments": [],
                    "examples": ["clear"]
                }, 
                "help": {
                    "description": "Display available commands or detailed help for a specific command.",
                    "usage": "help [command]",
                    "arguments": ["[command]    Optional. Name of a command to view detailed help. "],
                    "examples": ["help", 
                                 "help config",
                                 "help help"
                                 ]
                }, 
                "freq": {
                    "description": "Set a specific RFCA output frequency",
                    "usage": "freq <frequency>",
                    "arguments": ["<frequency>    Integer value of output frequency in MHz (800-6800)"],
                    "examples": ["freq 3000", # 3000 MHz
                                 "freq 5400"] # 5400 MHz  
                }, 
                "port": {
                    "description": "Open a specific SP8T output port",
                    "usage": "port <output_port>",
                    "arguments": ["<output_port>    Port number to open (1-8)"],
                    "examples": ["port 1",
                                 "port 8"]
                }
            }


##############################
#####vv Info functions vv#####

# Error message to CLI
def err(msg):
    print(f"[ERROR] {msg}")

# Warning message to CLI
def warn(msg):
    print(f"[WARN] {msg}")

# General information to CLI
def info(msg):
    print(f"[INFO] {msg}")
    

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

    app_font = GUI.font()
    app_font.setPointSize(12)
    GUI.setFont(app_font)

    # Central plot
    GUI.plot_widget = pg.PlotWidget()
    GUI.plot_widget.setBackground("w")
    GUI.plot_widget.showGrid(x=True, y=True, alpha=0.2)
    GUI.plot_widget.setLabel("bottom", '<span style="font-size:14pt;">Frequency (MHz)</span>')
    GUI.plot_widget.setLabel("left", '<span style="font-size:14pt;">Signal Loss (dBm)</span>')
    GUI.plot_widget.setTitle("Measurement Plot Area,", size="18pt")

    # Legend config
    legend = GUI.plot_widget.addLegend()
    legend.setLabelTextSize('10pt')
    legend.setPen(pg.mkPen(200, 200, 200))  # border

    # Construct GUI
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
        GUI.plot_widget.setTitle(f"(no data)")
        return

    df = port_data[port]
    freq = df["frequency"].to_numpy()

    GUI.plot_widget.setTitle(f"Signal Loss v. Frequency")

    # Plot each LogAmp value vs. Frequency
    for i in range(10):
        col = f"LA{i}"
        GUI.plot_widget.plot(
            freq,
            df[col].to_numpy(),
            name=col,
            pen=pg.mkPen(pg.intColor(i, hues=10), width=2),
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
 * @brief Enumerate connected USB devices and get COM port of Teensy4.1 
 * @return COM port number of Teensy4.1 or -1 if dne
 '''
def getTeensyPort():
    teensyVID = 5824 
    teensyPID = 1155
    for port in list_ports.comports():
        if port.vid is None or port.pid is None:
            continue

        if port.vid == teensyVID and port.pid == teensyPID:
            return port.device  # COM port
        
    return None # No Teensy4.1 found

'''
 * @brief Open connection to FW
 * @param PORT: COM port connected to FW
 * @param BAUD: Baud rate of communication, this value is irrelevant for virutal COM
 * @return ser: Serial object for FW communication
 '''
def connectFW():
    global connected
    BAUD = 115200 # BAUD is irrelevant for virtual COM (I just chose 115200 because it's somewhat standard) 
    PORT = getTeensyPort()

    if PORT is None:
        err("No Teensy4.1 device found, could not connect to firmware")
        return None

    try:
        # Connect to Teensy
        info("Connecting to Firmware...")
        ser = serial.Serial(PORT, BAUD, timeout=1)
        time.sleep(2)
        connected = True
        info(f"Connected at {PORT}")
        return ser
    except Exception as e:
        err(f"Could not connect to firmware: {e}")
        return None


'''
 * @brief Close connection to FW and cleanup
 * @param ser: Serial object for FW communication
 * @return: None
 '''
def disconnectFW():
    global ser
    global connected
    
    info("Disconnecting from Firmware...")
    ser.flush() # Flush data in buffer
    ser.close()
    ser = None
    connected = False
    info("Disconnected")
    
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
    #args = shlex.split(user_input)
    #parts = validate_input_args(args) # Validate command and arguments
    parts = shlex.split(user_input)

    # Check cmd validity
    cmd = parts[0]
    if cmd not in command_set:
        err(f"Command \'{cmd}\' not recognized. Enter 'help' for a list of available commands. ")
        return None, None

    if cmd == "clear" or cmd == "cls":
        clear_terminal()
        print_rfca_header()
        return None, None

    # Enumerate command list and descriptions
    if cmd == "help":
        help_handler(parts)
        return None, None

    # If a command is issued while the firmware is disconnected, throw an error
    global connected
    if cmd != "connect":
        if not connected:
            err("Not connected to firmware")
            return None, None


    # Next, based on the command, process further arguments into 'data' accordingly
    if cmd == "config":
        command, data = config_handler(parts)
        
    elif cmd == "calibrate": 
        command, data = calibrate_handler(parts)

    elif cmd == "sweep":
        command, data = sweep_handler(parts)
        
    elif cmd == "retrieve":
        command, data = retrieve_handler(parts)
        
    elif cmd == "delete":
        command, data = delete_handler(parts)
        
    elif cmd == "list":
        return cmd, None

    # Manual connect/disconnect commands
    elif cmd == "connect":
        command, data = connect_handler()
    elif cmd == "disconnect":
        command, data = disconnect_handler()
    
    # Power up/down the FW.  No data to send
    elif cmd == "boot":
        return cmd, None
    elif cmd == "shutdown":
        return cmd, None

    # These are dev/debugging commands
    elif cmd == "freq": # {cmd: 'freq', data: 3500} # Set ADF output to 3500 MHz
        command, data = freq_handler(parts)
        
    elif cmd == "port":
        command, data = port_handler(parts)

    else:
        # This shouldn't happen because we check cmd validity at the beginning of this function
        return None, None

    # After processing, return both the command and its data
    return command, data


''' 
 * @brief Handle 'help' CLI commands
 * @param parts: Array of CLI inputs (commands and arguments)
 * @return None
 '''
def help_handler(parts):

    # List description of all commands
    if len(parts) == 1:
        print("=================")
        print("These are the commands needed to conduct a full frequency sweep\n")
        for cmd in ["config", "calibrate", "sweep"]:
            description = command_set[cmd]["description"]
            print(f" {cmd:<15} {description}")
        
        print("\n=================")
        print("These are the data management commands\n")
        for cmd in ["list", "retrieve", "delete"]:
            description = command_set[cmd]["description"]
            print(f" {cmd:<15} {description}")

        print("\n=================")
        print("These are more precise hardware control commands (additional, not necessary for operation)\n")
        for cmd in ["connect", "disconnect", "boot", "shutdown", "freq", "port"]:
            description = command_set[cmd]["description"]
            print(f" {cmd:<15} {description}")

        print("\n=================")
        print("These are CLI aiding commands\n")
        for cmd in ["clear", "cls", "help"]:
            description = command_set[cmd]["description"]
            print(f" {cmd:<15} {description}")

        print("\n")
        return
    # List description of a single command
    else:
        cmd = parts[1]

        if cmd not in command_set:
            err(f"Command \'{cmd}\' not recognized, can't retrieve help information")
            return

        description = command_set[cmd]["description"]   # String
        usage = command_set[cmd]["usage"]               # String
        arguments = command_set[cmd]["arguments"]       # [String]
        examples = command_set[cmd]["examples"]           # [String]

        print(f"=================\n {cmd}")
        print(f"\n Description:\n  {description}")
        print(f"\n Usage:\n  {usage}")

        print("\n Argument(s):")
        if len(arguments) == 0:
            print("  None")
        else:
            for arg in arguments:
                print(f"  {arg}")

        print("\n Example(s):")
        for example in examples:
            print(f"  {example}")

    print("\n")
    return

''' 
 * @brief Handle 'config' CLI commands
 * @param parts: Array of CLI inputs (commands and arguments)
 * @return command, data
 '''
def config_handler(parts):

    # 'config' needs to open the ./config.json file and store in 'data'
    # There are no additional arguments required for this command
    
    try: 
        with open('./config.json', "r", encoding="utf-8") as f:
            data = json.load(f)
    except Exception as e:
        err(f"Failed to read/parse JSON file: {e}")
        return None, None 
    
    # cmd, data
    return parts[0], data

'''
 * @brief Handle 'calibrate' CLI commands
 * @param parts: Array of CLI inputs (commands and arguments)
 * @return command, data
 '''
def calibrate_handler(parts):
        
    # 'calibrate' cmd requires two additional arguments minimum: calibrate out in
    if len(parts) < 3:
        err("The 'calibrate' command requires two positional arguments: (int)out and (int)in")
        return None, None

    out_port = parts[1] # Must be between 1 and 8 inclusive
    in_port = parts[2]  # Must be between 1 and 10 inclusive
    set_cal = '-1'        # -1 by default indicates running typical calibration

    # Argument validation
    if((int)(out_port) < 1 or (int)(out_port) > 8):
        err(f"Argument (out_port) out of range: {out_port} is not within [1, 8]")
        return None, None
    elif((int)(in_port) < 1 or (int)(in_port) > 10):
        err(f"Argument (in_port) of range: {in_port} is not within [1, 10]")
        return None, None 

    # If a set_cal parameter is passed, set the cal value to that argument
    # CLI Args: calibrate out in set
    if len(parts) == 4:
        set_cal = parts[3] 

    # 'calibrate' needs to store port info in 'data'
    data = [out_port, in_port, set_cal]
    return parts[0], data

'''
 * @brief Handle 'sweep' CLI commands
 * @param parts: Array of CLI inputs (commands and arguments)
 * @return command, data
 '''
def sweep_handler(parts):
    # 'sweep' cmd requires one additional argument: sweep sweep_name
    if len(parts) < 2:
        err("The 'sweep' command requires an argument: (string)sweep_name")
        return None, None

    return parts[0], parts[1].removesuffix(".csv")

'''
 * @brief Handle 'retrieve' CLI commands
 * @param parts: Array of CLI inputs (commands and arguments)
 * @return command, data
 '''
def retrieve_handler(parts):
    # 'retrieve' cmd requires one additional argument: retrieve sweep_name
    if len(parts) < 2:
        err("The 'retrieve' command requires an argument: (string)sweep_name")
        return None, None
    
    return parts[0], parts[1].removesuffix(".csv")

'''
 * @brief Handle 'delete' CLI commands
 * @param parts: Array of CLI inputs (commands and arguments)
 * @return command, data
 '''
def delete_handler(parts):
    # 'delete' cmd requires one additional argument: delete sweep_name
    if len(parts) < 2:
        err("The 'delete' command requires an argument: (string)sweep_name")
        return None, None
    
    return parts[0], parts[1].removesuffix(".csv")

'''
 * @brief Handle 'connect' CLI commands
 * @return no data
 '''
def connect_handler():
    global ser
    ser = connectFW()
    return None, None # No cmd or data to send to fw

'''
 * @brief Handle 'disconnect' CLI commands
 * @return no data
 '''
def disconnect_handler():
    disconnectFW()
    return None, None # No cmd or data to send to fw

'''
 * @brief Handle 'freq' CLI commands
 * @param parts: Array of CLI inputs (commands and arguments)
 * @return command, data
 '''
def freq_handler(parts):
    # 'freq' cmd requires one additional argument: freq frequency
    if len(parts) < 2:
        err("The 'freq' command requires an argument: (int)frequency (MHz)")
        return None, None

    # Argument validation
    if((int)(parts[1]) < 800 or (int)(parts[1]) > 6800):
        err(f"Argument (frequency) out of range: {parts[1]} MHz is not within [800, 6800] Mhz")
        return None, None
    
    return parts[0], parts[1]

'''
 * @brief Handle 'port' CLI commands
 * @param parts: Array of CLI inputs (commands and arguments)
 * @return command, data
 '''
def port_handler(parts):
    # 'port' cmd requires one additional argument: port out_port
    if len(parts) < 2:
        err("The 'port' command requires an argument: (int)port [1, 8]")
        return None, None

    # Argument validation
    if((int)(parts[1]) < 1 or (int)(parts[1]) > 8):
        err(f"Argument (port) out of range: {parts[1]} is not within [1, 8]")
        return None, None
    
    return parts[0], parts[1]


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
    info("Waiting firmware for response...")

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
                        info(f'Firmware has received the \'{msg.get("cmd")}\' command.')
                        continue

                    # 'complete' is sent by FW to indicate completion of command processing. 
                    elif msg.get("type") == "complete":
                        info(f'Firmware has finished processing the \'{msg.get("cmd")}\' command.')
                        return
                    
                    # 'progress' is sent by FW to inform CLI of its current step in processing
                    elif msg.get("type") == "progress":

                        if(msg.get("cmd") == "sweep"):
                            frequency = msg.get("data").get("frequency")
                            out_port = msg.get("data").get("port")
                            info(f'From port {out_port}: Measuring frequency: {frequency} MHz')
                        elif(msg.get("cmd") == "calibrate"):
                            out_port = msg.get("data").get("out")
                            in_port = msg.get("data").get("in")
                            frequency = msg.get("data").get("frequency")
                            info(f'Out: {out_port} In: {in_port}: Calibrating for frequency: {frequency} MHz')

                        # Prolong timeout if firmware is sending progress updates
                        start_time = time.time()

                    # Otherwise, process response data
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
                err(f"Failed to read serial response: {e}")
                
        time.sleep(0.1)

    # Timeout warning
    warn("Timed out while waiting for response.")

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
        info('Configured RFCA with the following parameters:')
        info(f'{config_params}')
    elif(cmd == "calibrate"):
        cal_data = data.get("cal_data")
        info('Recorded the following calibration data')
        info(f'{cal_data}')
    elif(cmd == "sweep"):
        config_params = data.get("config_params")
        info("sweep with the following parameters:")
        info(f'{config_params}')
    elif(cmd == "list"):
        files = data.get("files")

        if(len(files) == 0):
            info("Found 0 files")
        else:
            info(f"Found {len(files)} file(s): \n")
            for file in files:
                print(file)

            print('\n')

    # Retrieve a .csv file from the firmware, save it locally and display its data
    elif(cmd == "retrieve"):
        sweep_name = data.get("sweep_name")
        sweep_data = data.get("sweep_data")
        
        # Get directory of this script
        script_dir = Path(__file__).resolve().parent

        # Create sweep_data directory if it doesn't exist
        sweep_dir = script_dir / "sweep_data"
        sweep_dir.mkdir(parents=True, exist_ok=True)

        # Construct file path
        file_path = sweep_dir / f"{sweep_name}.csv"

        try:
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(sweep_data)

            info(f"Saved sweep to: {file_path}")

        except Exception as e:
            err(f"Failed to save CSV: {e}")

        # Use the global csv_path
        global csv_path
        csv_path = file_path

        on_tab_changed(0)  # Set initial tab (0-7)
        GUI.show()

    elif(cmd == "delete"): 
        sweep_name = data.get("sweep_name")
        info(f"Deleted {sweep_name} from the firmware")

    elif(cmd == "boot"):
        info(f"Device booted")
    elif(cmd == "shutdown"):
        info(f"Device shutdown")

    # Dev command
    elif(cmd == "freq"):
        frequency = data.get("freq")
        power = data.get("power")
        info(f"Measured {power} dBm at {frequency} MHz")
    elif(cmd == "port"):
        port = data.get("port")
        info(f"Opened SP8T port {port}")
    else:
        # This shouldn't happen 
        err(f"Unrecognized CMD: {cmd}")

# TODO
def processError():
    print("Error detected")


def main():
    global ser
    
    clear_terminal()
    ser = connectFW()
    print_rfca_header()

    while True:
        try:
            # Prompt user
            user_input = input("rfca>> ").strip()

            # Quit condition
            if user_input.lower() in ["q", "quit", "exit"]:
                disconnectFW()
                info("Exiting RFCA CLI...")
                break

            # Ignore empty input
            if not user_input:
                continue
            
            # Parse terminal input, package and send to firmware, wait till response or time out
            cmd, data = parse_user_input(user_input)

            # Only send JSON to FW if cmd is present
            if (cmd is not None):
                sendJson(cmd, data, ser)
                wait_for_response(ser) 

        except KeyboardInterrupt:
            info("Interrupted. Type 'q' to quit.")
        except EOFError:
            info("EOF received. Exiting.")
            break


if __name__ == "__main__":
    main()