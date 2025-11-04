
# TODO: dynamically assign COM port


# Used to build the Command Line Interface (CLI)
from argparse import ArgumentParser, Namespace

# Used for USB communication with the Teensy 4.1
import serial
import time
import json
import os
import sys
PORT = "COM3"
BAUD = 9600


# Connect to Teensy
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)

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

# Check which command was used
if args.command == "config":
    # Flow:
    # - Get filename from args
    # - Load config file based on filename
    # - Send 'config' command identifier and JSON
    # - Wait for OK response

    #python rfca.py config --file config.json

    # 1) Open specified config.json file
    try: 
        with open(args.file, "r", encoding="utf-8") as f:
            cfg_obj = json.load(f)
    except Exception as e:
        print(f"Failed to read/parse JSON file: {e}")
        sys.exit(1)

    # 2) Create JSON
    envelope = {
        "cmd": "config",
        "data": cfg_obj
    }
    body = json.dumps(envelope, separators=(",", ":")).encode("utf-8")

    # 3) Write to USB
    header = f"LEN {len(body)}\n".encode("utf-8")
    ser.write(header)
    ser.write(body)
    ser.flush()

    # 4) Wait for acknowledge
    ack = ser.readline().decode("utf-8", errors="ignore").strip()
    if ack == "OK":
        print("Teensy acknowledged config: OK")
    else:
        print(f"Teensy response: {ack!r}")

if args.command == "sweep":
    # Send sweep command to USB with sweep name
    # Receive update percentages (print update chars to console...)
    # Receive 'complete', print to console
    sweep_name = args.name

    envelope = {
        "cmd": "sweep",
        "data": sweep_name
    }
    body = json.dumps(envelope, separators=(",", ":")).encode("utf-8")

    # 3) Write to USB
    header = f"LEN {len(body)}\n".encode("utf-8")
    ser.write(header)
    ser.write(body)
    ser.flush()

    # 4) Wait for acknowledge
    ack = ser.readline().decode("utf-8", errors="ignore").strip()
    if ack == "OK":
        print("Teensy acknowledged config: OK")
    else:
        print(f"Teensy response: {ack!r}")

if args.command == "retrieve":

    sweep_name = args.name
    ser.write(b"retrieve\n")
    resp = ser.readline().decode(errors='ignore').strip()
    print("Response:", resp)

if args.command == "delete":

    sweep_name = args.name

    envelope = {
        "cmd": "delete",
        "data": sweep_name
    }
    body = json.dumps(envelope, separators=(",", ":")).encode("utf-8")

    # 3) Write to USB
    header = f"LEN {len(body)}\n".encode("utf-8")
    ser.write(header)
    ser.write(body)
    ser.flush()

    # 4) Wait for acknowledge
    ack = ser.readline().decode("utf-8", errors="ignore").strip()
    if ack == "OK":
        print("Teensy acknowledged config: OK")
    else:
        print(f"Teensy response: {ack!r}")

if args.command == "cancel":
    ser.write(b"cancel\n")
    resp = ser.readline().decode(errors='ignore').strip()
    print("Response:", resp)

if args.command == "list":
    ser.write(b"list\n")
    resp = ser.readline().decode(errors='ignore').strip()
    print("Response:", resp)




ser.close()