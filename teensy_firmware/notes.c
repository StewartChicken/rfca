
// Serial cmd input commands:
// {cmd: "a", data: NULL} // 1000 MHz output
// {cmd: "b", data: NULL} // 2000 MHz output
// {cmd: "c", data: NULL} // 3000 MHz output
// {cmd: "d", data: NULL} // 4000 MHz output
// {cmd: "config", data: {sp8t_out_port: 3, start_freq: 1000, stop_freq: 1000, step_size: 1000, delay_ms: 1000}} // Config update
// {cmd: "sweep", data: "sweep1"} // call (unfinished) sweep function

// Notes for 2/24:
// - Added a delay option to the initFreq function which allows you to pass the delay between register writes as an argument to the Serial command line. 
//    This lets us control the delays and monitor individual register writes if we want to
//
//    EX: {cmd: "a", data: 1200} // set to 1000 MHz with 1200 ms delay between register writes
//
// - Adding 1.5k resistors fixed the write issue
// - Can output frequencies between 850 and 6800 MHz
// - Cleaned up code redistributing to driver files
// 
// Next steps
// - Prep for log-amp reading
// - Store log amp data (file type/format?)
// - Integrate switch
//
// Done as of 3/12/2026
// - Full frequency control from 800 MHz to 6.8 GHz (SPI)
// - SP8T control
// - SD card control for state persistence
// - LogAmp reading
// - Record LogAmp read values to SD
// - 
//
// TODO
// - Data retrieval from firmware SD to CLI
// - Finalize sweep process
// - Thru cal. process
// - Error handling
// - EXPO GUI
// - Finalize all drivers
// - Finalize configurable parameters
// 
// PLAN
// - Finalize Teensy/CLI communication
// - Finalize config, calibration, sweep process
// - Finalize Error handling
// - EXPO GUI
//
// 3/17
// - Finished retrieve command between Teensy and CLI so .csv can be transferred to CLI
// - Switched CLI to loop-based implementation creating better UX
//    - Incorporated pseudo 'handshake' where Teensy sends ack and complete messages to indicate progress
// - .csv files are saved locally, can now begin working on EXPO GUI which will display their contents
// - Error handling has structure but needs to be completed rigorously
// - Calibration process needs to be done
//  
// 3/20
// - Want to add a 'view' command (with sweep name) which looks for data on local machine
//   - If not found, offer option to search/retrieve from teensy SD?
// 
// 3/22
// - GUI can display sweep data from CSV now (test_visual2.py provides a classless implementation)
// Next: Write the 'visualize' function within test_visual2.py which can be called from a CLI loop
//       - Need to organize where data is instantiated and where functions are defined
// 
// 3/24
// - Instead of a separate 'view' command, I'll just incorporate the plot through the 'retrieve' command 
//   - (it'll auto display once data is loaded from firmware)
// ^^^ This was completed
// 
// Checkin (3/24):
// - Done:
//   - EXPO GUI
//   - Config/sweep process
//   - Teensy/CLI communication
// - TODO:
//   - Cal
//   - Robust Error Handling 
// 
//  Started working on the cal process
//   - Created firmware data structure to store cal data
//   - Created JSON on SD to persist cal data
//   - TODO: Cal config data? 
//
// 3/26: Cal ideas
// - Instead of one full blown cal process that incorporates calibration for all 17 ports that are needed,
//    we have the user manually select the in/out port
// - This fits the existing architecture much closer
//   - User command -> Teensy process -> Teensy response -> CLI receives response
//   - If a mistake is made by the user (wrong port), they don't have to restart the entire process. 
// - To start, we will assume a fixed set of frequencies to be calibrated
// - After implementation, it will be made customizeable (maybe based on the config file?)
//
// Also need to implement the signal which outlines unexpected measurement
// - Maybe we have the sweep command send a JSON file with expected loss values?
// - The Teensy FW would then return data comparing expected with measurement in the response
//
// Flow: 
// CLI cal command: 'calibrate out in'
//  - This saves cal data for a specified sp8t output and log-amp input to the SD card (1 of 17 needed)
//  - The frequencies at which the ports are calibrated are hard coded: 800 - 6800 MHz w/ 100 MHz steps
//  - The firmware also retains a cal_data buffer which stores the information in RAM (so the SD card doesn't have to be accessed)
//    - This buffer is populated on boot by reading the SD card
//  - When a sweep is conducted, cal. information is subtracted from measured data before being saved as sweep information. 
//
// Update config command to bypass argument: Just update config.json file 
//
// 3/29
// - When retrieving sweep data, there is not enough ram to send large files? (Times out w/ too much data)
// - May need to send data in parts and reconstruct on the CLI end
// - Not an issue for the acceptance test, we'll just sample at 200 MHz steps
//
// 3/30
// CAL data structure example
// Each 0 represents the loss at a specific frequency, ranging from 800 MHz to 6.8 GHz (61 total steps per port)
// 
//{
// "out1_in1":  [0, 0, 0...],  
// "out2_in1":  [0],
// "out3_in1":  [0],
// "out4_in1":  [0],
// "out5_in1":  [0],
// "out6_in1":  [0],
// "out7_in1":  [0],
// "out8_in1":  [0],
// "out1_in2":  [0],
// "out1_in3":  [0],
// "out1_in4":  [0],
// "out1_in5":  [0],
// "out1_in6":  [0],
// "out1_in7":  [0],
// "out1_in8":  [0],
// "out1_in9":  [0],
// "out1_in10": [0]
// }
//
//
// 3/31
// TODO:
// - Power up sequence: +3.3, -3.3, Switch CTRL, RF
// - Power dn sequence: RF, Switch CTRL, -3.3, +3.3
// - Send data to CLI in chunks not in one segment (RAM limitation)
// - Error handling
// - Add more debug/dev functionality
//
// Done 4/1: 
// - Added cal reset capabilities from CLI
// - Moved FW connection to main in CLI
// - Added manual connect/disconnect commands for CLI -> FW
//   - Also did error handling, CLI starts by calling connectFW()
// - Update log amp power curve coefficients
// - CLI would terminate when calling commands while disconnected from FW
//   - Modified this so the loop doesn't exit
// - CLI throws error on unrecognized command
// - Added 'clear/cls' commands
// - Added 'freq' & 'port' dev commands from CLI
// - Added err, warn, info commands
// - Removed processing spinner
// - Sweep data saved to a dedicated directory rather than root
// - COM port selection is dynamic 
// - Basic argument validation
//
// Done 4/7:
// - Added function to adf5356 driver to disable RF output A
// - Added 'help' command to CLI for detailed info about command list
// - CLI timeout increases if firmware is sending progress reports
// - Remove .csv suffix from user inputs
// - Reorganized common_defs.h
//
// To Test:
// - Disabling RFOUTA works as expected