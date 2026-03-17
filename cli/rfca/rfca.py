
# TODO: Create consistent dependency environment (multiple libraries for 'serial')
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
                        print("[INFO] Firmware has received command.")
                        continue
                    elif msg.get("type") == "complete":
                        print("[INFO] Firmware has finished processed command.")
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

    # Should be all cases (for now)
    if (response_data.get("type") == "data"):
        cmd = response_data.get("cmd")

        if(cmd == "config"):
            pass
        elif(cmd == "retrieve"):
            sweep_name = response_data.get("data").get("sweep_name")
            sweep_data = response_data.get("data").get("sweep_data")
            
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

        else:
            print(cmd)

def main():
    
    # TODO: Check OS for clear function
    os.system('cls')

    print("\n=== RFCA CLI ===")
    print("Enter commands below")
    print("Type 'q' to quit\n")

    while True:
        try:
            # Prompt user
            user_input = input("rfca> ").strip()

            # Quit condition
            if user_input.lower() in ["q", "quit", "exit"]:
                print("Exiting RFCA CLI...")
                break

            # Ignore empty input
            if not user_input:
                continue

            cmd, data = parse_user_input(user_input)
            sendJson(cmd, data)
            wait_for_response()
            
        except KeyboardInterrupt:
            print("\n[INFO] Interrupted. Type 'q' to quit.")
        except EOFError:
            print("\n[INFO] EOF received. Exiting.")
            break


if __name__ == "__main__":
    main()