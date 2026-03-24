
import time

import sys
from pathlib import Path

import pandas as pd
import pyqtgraph as pg

from PySide6.QtWidgets import (
    QApplication, 
    QMainWindow, 
    QWidget, 
    QVBoxLayout, 
    QTabWidget,
)


CSV_COLUMNS = ["out_port", "frequency"] + [f"LA{i}" for i in range(10)]

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

def visualize():
    pass

def main():
    
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

    csv_path = Path("sweep_test.csv")
    port_data = split_data_by_port(load_sweep_data(csv_path))

    # Port tab switch functions
    def plot_port_data(port: int):
        GUI.plot_widget.clear()
        GUI.plot_widget.addLegend()

        if port not in port_data:
            GUI.plot_widget.setTitle(f"Port {port} (no data)")
            return

        df = port_data[port]
        freq = df["frequency"].to_numpy()

        GUI.plot_widget.setTitle(f"Port {port}")

        for i in range(10):
            col = f"LA{i}"
            GUI.plot_widget.plot(
                freq,
                df[col].to_numpy(),
                name=col,
                pen=pg.intColor(i, hues=10),
            )

    def on_tab_changed(index: int):
        port = index + 1
        plot_port_data(port)
        
    GUI.tabs.currentChanged.connect(on_tab_changed)
    on_tab_changed(0) # Initial tab


    print("Enter commands below")
    print("Type 'q' to quit\n")

    while True:

        try:
            # Prompt user
            user_input = input("input>> ").strip()

            # Quit condition
            if user_input.lower() in ["q", "quit", "exit"]:
                print("Exiting CLI...")
                break

            # Ignore empty input
            if not user_input:
                continue
           
            if (user_input == 'view'):
                print("WE gonna visualize dis chart or sum")
            elif (user_input == 'show'):
                GUI.show()
            elif (user_input == 'hide'):
                GUI.hide() 
            else:
                print(user_input)

        except KeyboardInterrupt:
            print("\n[INFO] Interrupted. Type 'q' to quit.")
        except EOFError:
            print("\n[INFO] EOF received. Exiting.")
            break
            
            


if __name__ == "__main__":
    main()