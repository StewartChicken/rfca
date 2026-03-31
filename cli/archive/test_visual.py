
import sys
from pathlib import Path

import pandas as pd
import pyqtgraph as pg

from PySide6.QtCore import QEventLoop
from PySide6.QtWidgets import (
    QApplication, 
    QMainWindow, 
    QWidget, 
    QVBoxLayout, 
    QTabWidget,
    QMessageBox
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

class SweepViewerSkeleton(QMainWindow):
    def __init__(self, csv_path: Path):
        super().__init__()
        self.setWindowTitle("Sweep Viewer Skeleton")
        self.resize(1200, 700)

        self.csv_path = csv_path
        self.port_data = {}

        # Central widget + main vertical layout
        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)

        # Top tabs
        self.tabs = QTabWidget()
        for i in range(1, 9):
            tab = QWidget()  # empty placeholder page
            self.tabs.addTab(tab, f"Port {i}")

        # Central plot area
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground("w")
        self.plot_widget.showGrid(x=True, y=True, alpha=0.2)
        self.plot_widget.setLabel("bottom", "Frequency")
        self.plot_widget.setLabel("left", "Value")
        self.plot_widget.setTitle("Measurement Plot Area")
        self.plot_widget.addLegend()

        # Blank placeholder curve area:
        # we don't plot anything yet; this just makes the plot region visible

        # Add widgets to layout
        layout.addWidget(self.tabs, stretch=0)
        layout.addWidget(self.plot_widget, stretch=1)

        self.tabs.currentChanged.connect(self.on_tab_changed)

        self.load_data()
        self.on_tab_changed(0)
    
    def load_data(self):
        df = load_sweep_data(self.csv_path)
        self.port_data = split_data_by_port(df)

    def clear_plot(self):
        self.plot_widget.clear()
        self.plot_widget.addLegend()

    def plot_port_data(self, port: int):
        self.clear_plot()

        if port not in self.port_data:
            self.plot_widget.setTitle(f"Port {port} (no data)")
            return

        df = self.port_data[port]
        freq = df["frequency"].to_numpy()

        self.plot_widget.setTitle(f"Port {port}")

        for i in range(10):
            col = f"LA{i}"
            self.plot_widget.plot(
                freq,
                df[col].to_numpy(),
                name=col,
                pen=pg.intColor(i, hues=10),
            )

    def on_tab_changed(self, index: int):
        port = index + 1
        self.plot_port_data(port)


def main():
    app = QApplication([])
    csv_path = Path("sweep_test.csv")

    window = SweepViewerSkeleton(csv_path)
   
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
