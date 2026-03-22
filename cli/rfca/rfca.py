
# TODO: Create consistent dependency environment (multiple libraries for 'serial')

# For FW interaction
import os
import json
import shlex
import serial
import time
import sys


PORT = "COM4"
BAUD = 115200

# Connect to Teensy
print("Connecting to Firmware...")
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)


# CLI input
def parse_user_input(user_input):
    parts = shlex.split(user_input) # Space-delimited

    cmd = parts[0]

    if len(parts) == 1:
        data = None
    elif len(parts) == 2:
        arg = parts[1]

        # Only the 'config' commands needs special handling as it sends a JSON file to the firmware
        if cmd == "config":  
            config_file = arg

            try: 
                with open(config_file, "r", encoding="utf-8") as f:
                    data = json.load(f)
            except Exception as e:
                print(f"Failed to read/parse JSON file: {e}")
                return None, None # TODO, check this error handling
        else:
            data = arg

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
        processError()
        return
    
    # If no errors, process the response based on the type
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
        pass
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
    elif(cmd == "delete"): 
        sweep_name = data.get("sweep_name")
        print(f"[INFO] Deleted {sweep_name} from the firmware")
    else:
        # This shouldn't happen TODO: Throw error?
        print(cmd)

# TODO
def processError():
    print("Error detected")


def main():
    
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

            # Don't query firmware if cmd/data are not present
            if (cmd == "view"):
                print("got the view command")

                path = data

                continue

            sendJson(cmd, data)
            wait_for_response()
            
        except KeyboardInterrupt:
            print("\n[INFO] Interrupted. Type 'q' to quit.")
        except EOFError:
            print("\n[INFO] EOF received. Exiting.")
            break


if __name__ == "__main__":
    main()