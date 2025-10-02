
from argparse import ArgumentParser, Namespace

import serial
import time

PORT = "COM3"
BAUD = 9600

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

# Create the 'cancel' subcommand
cancel_parser = subparsers.add_parser("cancel", help="Cancels a running sweep")

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
    # Send config command and config.json file to Teensy 4.1 via USB
    # Wait for 'success' signal and print to console
    print("User chose to configure")
    print("Filename provided:", args.file)
    print("\nEstablishing Serial connection...")
    ser = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(2)

    print("\nSuccessfully connected, sending test message...")
    ser.write(b"Test Teensy!\n")

    print("\nMessage sent, awaiting response...")
    resp = ser.readline().decode(errors='ignore').strip()
    print("Response", resp)

    print("\nClosing serial...")
    ser.close()
    

if args.command == "sweep":
    # Send sweep command to USB with sweep name
    # Receive update percentages (print update chars to console...)
    # Receive 'complete', print to console
    print("User chose to start a sweep")
    print("Sweep name: ", args.name)

if args.command == "retrieve":
    print("User chose to retrieve a sweep")
    print("Retrieved sweep: ", args.name)

if args.command == "delete":
    print("User chose to delete a sweep")
    print("Deleted sweep: ", args.name)

if args.command == "cancel":
    print("User chose to cancel the sweep")

if args.command == "list":
    print("User chose to list previous sweeps")