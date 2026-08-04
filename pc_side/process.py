import serial
import threading
import time
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
import queue as q

class Accel:
    def __init__(self, timestamp, x, y, z):
        self.timestamp = timestamp
        self.x = int(x)
        self.y = int(y)
        self.z = int(z)
        self.total_force = np.sqrt(self.x ** 2 + self.y ** 2 + self.z ** 2)

class Tof:
    def __init__(self, timestamp, distance):
        self.timestamp = timestamp
        self.distance = int(distance)

queue = q.Queue()
msg_queue = q.Queue()
TIMEOUT = 3
ACCEL = "[ACCEL]"
TOF = "[TOF]"

def acquire_line():
    global msg_queue
    with serial.Serial("/dev/ttyACM0", 115200, timeout=TIMEOUT) as ser:
        while True:
            line = ser.readline()
            msg_queue.put(line)

def process_line():
    global msg_queue
    while True:
        line = msg_queue.get().decode("utf-8").strip()
        if len(line) > len(ACCEL) and line[:len(ACCEL)] == ACCEL:
            line = line[len(ACCEL):]
            values = line.split(",")
            if len(values) == 4:
                queue.put(Accel(*values))
        elif len(line) > len(TOF) and line[:len(TOF)] == TOF:
            line = line[len(TOF):]
            values = line.split(",")
            if len(values) == 2:
                queue.put(Tof(*values))

def update_graph(frame):
    global x_data, queue, y_data_accel, y_data_tof, tof_line
    point = queue.get()
    x_data.append(point.timestamp)
    if isinstance(point, Accel):
        y_data_accel.append(point.total_force)
        accel_line.set_data(x_data, y_data_accel)
    elif isinstance(point, Tof):
        y_data_tof.append(point.distance)
        tof_line.set_data(x_data, y_data_tof)

acquire = threading.Thread(target=acquire_line)
process = threading.Thread(target=process_line)

acquire.start()
process.start()

# Create the main plot
fig, ax1 = plt.subplots()
x_data = []
y_data_accel = []
y_data_tof = []
# Plot first dataset on the left Y-axis
color = 'tab:blue'
ax1.set_xlabel('Time (ms)')
ax1.set_ylabel('Gs of force', color=color)
accel_line = ax1.plot(x_data, y_data_accel, color=color)
ax1.tick_params(axis='y', labelcolor=color)     
# Create the twin axes sharing the x-axis
ax2 = ax1.twinx()  
# Plot second dataset on the right Y-axis
color = 'tab:red'
ax2.set_ylabel('Suspension travel distance (mm)', color=color)
tof_line = ax2.plot(x_data, y_data_tof, color=color, linestyle='--')
ax2.tick_params(axis='y', labelcolor=color)
ani = FuncAnimation(fig, update_graph, interval=250, cache_frame_data=False)
plt.show()