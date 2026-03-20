
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
    QMessageBox
)



class SweepViewerSkeleton(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Sweep Viewer Skeleton")
        self.resize(1200, 700)

        # Central widget + main verticaSl layout
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

        # Blank placeholder curve area:
        # we don't plot anything yet; this just makes the plot region visible

        # Add widgets to layout
        layout.addWidget(self.tabs, stretch=0)
        layout.addWidget(self.plot_widget, stretch=1)


def main():
    app = QApplication(sys.argv)

    # Optional: nicer default antialiasing for future plotting
    pg.setConfigOptions(antialias=True)

    window = SweepViewerSkeleton()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()

'''


import sys
import csv
from pathlib import Path

import pyqtgraph as pg

from PySide6.QtWidgets import (
    QApplication,
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QTabWidget,
    QMessageBox,
)


class SweepViewer(QMainWindow):
    def __init__(self, csv_path: str):
        super().__init__()
        self.setWindowTitle("Sweep Viewer")
        self.resize(1200, 700)

        self.csv_path = Path(csv_path)
        self.la_columns = [f"LA{i}" for i in range(10)]

        # Data structure:
        # {
        #   out_port_value: {
        #       "frequency": [...],
        #       "LA0": [...],
        #       ...
        #       "LA9": [...]
        #   },
        #   ...
        # }
        self.port_data = {}
        self.ports = []

        self._load_data()
        self._build_ui()
        self._populate_tabs()
        self._connect_signals()

        if self.ports:
            self.update_plot_for_current_tab()

    def _load_data(self):
        if not self.csv_path.exists():
            raise FileNotFoundError(f"CSV file not found: {self.csv_path}")

        with open(self.csv_path, "r", newline="") as f:
            reader = csv.DictReader(f)

            if reader.fieldnames is None:
                raise ValueError("CSV file appears to have no header row.")

            required_columns = ["out_port", "frequency"] + self.la_columns
            missing = [col for col in required_columns if col not in reader.fieldnames]
            if missing:
                raise ValueError(f"Missing required columns: {missing}")

            for row_num, row in enumerate(reader, start=2):
                try:
                    out_port = int(float(row["out_port"]))
                    frequency = float(row["frequency"])
                    la_values = {la: float(row[la]) for la in self.la_columns}
                except ValueError as e:
                    raise ValueError(f"Invalid numeric data on row {row_num}: {e}")

                if out_port not in self.port_data:
                    self.port_data[out_port] = {"frequency": []}
                    for la in self.la_columns:
                        self.port_data[out_port][la] = []

                self.port_data[out_port]["frequency"].append(frequency)
                for la in self.la_columns:
                    self.port_data[out_port][la].append(la_values[la])

        self.ports = sorted(self.port_data.keys())

        if not self.ports:
            raise ValueError("No valid data rows found in CSV.")

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)

        self.tabs = QTabWidget()
        self.plot_widget = pg.PlotWidget()

        self.plot_widget.showGrid(x=True, y=True, alpha=0.2)
        self.plot_widget.setLabel("bottom", "Frequency")
        self.plot_widget.setLabel("left", "LA Value")
        self.plot_widget.setTitle("Sweep Data")

        layout.addWidget(self.tabs, stretch=0)
        layout.addWidget(self.plot_widget, stretch=1)

    def _populate_tabs(self):
        for port in self.ports:
            tab = QWidget()
            self.tabs.addTab(tab, f"Port {port}")

    def _connect_signals(self):
        self.tabs.currentChanged.connect(self.update_plot_for_current_tab)

    def update_plot_for_current_tab(self):
        index = self.tabs.currentIndex()
        if index < 0 or index >= len(self.ports):
            return

        selected_port = self.ports[index]
        port_entry = self.port_data[selected_port]

        self.plot_widget.clear()
        self.plot_widget.addLegend()
        self.plot_widget.setTitle(f"Sweep Data - Port {selected_port}")

        colors = [
            (255, 0, 0),
            (0, 128, 255),
            (0, 180, 0),
            (255, 140, 0),
            (180, 0, 180),
            (0, 180, 180),
            (120, 120, 0),
            (255, 0, 120),
            (100, 100, 100),
            (0, 0, 0),
        ]

        x = port_entry["frequency"]

        for i, la in enumerate(self.la_columns):
            y = port_entry[la]
            pen = pg.mkPen(color=colors[i], width=2)
            self.plot_widget.plot(x, y, pen=pen, name=la)


def main():
    app = QApplication(sys.argv)
    pg.setConfigOptions(antialias=True)

    csv_path = Path(__file__).parent / "sweepdata.csv"

    try:
        window = SweepViewer(csv_path)
        window.show()
        sys.exit(app.exec())
    except Exception as e:
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setWindowTitle("Error")
        msg.setText(str(e))
        msg.exec()
        sys.exit(1)


if __name__ == "__main__":
    main()

'''