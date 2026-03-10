
# TODO: dynamically assign COM port

# Used to build the Command Line Interface (CLI)
from argparse import ArgumentParser, Namespace

# Used for USB communication with the Teensy 4.1
import serial
import time
import json
import sys
PORT = "COM4"
BAUD = 115200

# Connect to Teensy
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)


###############################
###### Create Arg Parser ######
###############################

parser = ArgumentParser(description="Main parser")
subparsers = parser.add_subparsers(dest="command", required=True)

# Create the 'config' subcommand
config_parser = subparsers.add_parser("config", help="Configure the system")
config_parser.add_argument(
    "-f", "--file", 
    required=True, 
    help="Specify the configuration filename"
)

# Create the 'sweep' subcommand
sweep_parser = subparsers.add_parser("sweep", help="Run a sweep")
sweep_parser.add_argument(
    "-n", "--name", 
    required=True, 
    help="Specify the name of the sweep"
)

# Create the 'calibrate' subcommand
calibrate_parser = subparsers.add_parser("calibrate", help="Calibrate thru loss")

# Create the 'cancel' subcommand
#cancel_parser = subparsers.add_parser("cancel", help="Cancels a running sweep")

# Creates the 'list' subcomand
list_parser = subparsers.add_parser("list", help="Lists previous sweeps")

# Create the 'retrieve' subcomand
retrieve_parser = subparsers.add_parser("retrieve", help="Retrieve specified sweep data")
retrieve_parser.add_argument(
    "-n", "--name",
    required=True,
    help="Specify the name of the sweep"
)

# Create the 'delete' subcomand
delete_parser = subparsers.add_parser("delete", help="Delete specified sweep data")
delete_parser.add_argument(
    "-n", "--name",
    required=True,
    help="Specify the name of the sweep to be deleted"
)

args: Namespace = parser.parse_args()

##############################
###### Helper Functions ######
##############################

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

    # Wait for firmware response
    response = ser.readline().decode("utf-8", errors="ignore").strip()
    #print(f"Teensy response: {response!r}")
    return response

# TODO: Write this function. This is a placeholder right now
def processResponse(resp):
    print(f"Teensy response: {resp}")


###############################
##### Parse CLI arguments #####
###############################

# Example usage:
# python rfca.py config --file ./config.json
# data ex: {cmd: "config", data: {sp8t_out_port: 3, start_freq: 1000, stop_freq: 1000, step_size: 1000, delay_ms: 1000}}
if args.command == "config":
    config_file = args.file

    try: 
        with open(config_file, "r", encoding="utf-8") as f:
            cfg_obj = json.load(f)
    except Exception as e:
        print(f"Failed to read/parse JSON file: {e}")
        sys.exit(1)

    response = sendJson("config", cfg_obj)
    processResponse(response)

# Example usage: 
# python rfca.py sweep --name "Sweep1"
# data ex: {cmd: "sweep", data: "Sweep1"}   
if args.command == "sweep":
    sweep_name = args.name
    response = sendJson("sweep", sweep_name)
    processResponse(response)

# Example usage:
# python rfca.py retrieve --name "Sweep1"
if args.command == "retrieve":
    sweep_name = args.name
    response = sendJson("retrieve", sweep_name)
    processResponse(response)

# Example usage:
# python rfca.py delete --name "Sweep1"
# data ex: {cmd: "delete", data: "Sweep1"}
if args.command == "delete":
    sweep_name = args.name
    response = sendJson("delete", sweep_name)
    processResponse(response)

# Example usage:
# python rfca.py list
# data ex: {cmd: "list", data: NULL}
if args.command == "list":
    response = sendJson("list", None)
    processResponse(response)
    
ser.close()