
# TODO: dynamically assign COM port

# Used to build the Command Line Interface (CLI)
from argparse import ArgumentParser, Namespace

# Used for USB communication with the Teensy 4.1
import serial
import time
import json
import sys
PORT = "COM3"
BAUD = 9600

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
    ack = ser.readline().decode("utf-8", errors="ignore").strip()
    print(f"Teensy response: {ack!r}")


###############################
##### Parse CLI arguments #####
###############################

# Example usage:
# python rfca.py config --file ./config.json
if args.command == "config":
    config_file = args.file

    try: 
        with open(config_file, "r", encoding="utf-8") as f:
            cfg_obj = json.load(f)
    except Exception as e:
        print(f"Failed to read/parse JSON file: {e}")
        sys.exit(1)

    sendJson("config", cfg_obj)

# Example usage: 
# python rfca.py sweep --name "Sweep1"
if args.command == "sweep":
    sweep_name = args.name
    sendJson("sweep", sweep_name)

# Example usage:
# python rfca.py retrieve --name "Sweep1"
if args.command == "retrieve":
    sweep_name = args.name
    sendJson("retrieve", sweep_name)

# Example usage:
# python rfca.py delete --name "Sweep1"
if args.command == "delete":
    sweep_name = args.name
    sendJson("delete", sweep_name)

# Example usage:
# python rfca.py list
if args.command == "list":
    sendJson("list", None)
    
ser.close()