import serial
import threading
import time
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
import queue

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

accel_queue = queue.Queue()
tof_queue = queue.Queue()
msg_queue = queue.Queue()
TIMEOUT = 3
ACCEL = "[ACCEL]"
TOF = "[TOF]"

def acquire_line():
    with serial.Serial("/dev/ttyACM0", 115200, timeout=TIMEOUT) as ser:
        while True:
            line = ser.readline()
            msg_queue.put(line)

def process_line():
    while True:
        line = msg_queue.get().decode("utf-8").strip()
        if len(line) > len(ACCEL) and line[:len(ACCEL)] == ACCEL:
            line = line[len(ACCEL):]
            values = line.split(",")
            if len(values) == 4:
                accel_queue.put(Accel(*values))
        elif len(line) > len(TOF) and line[:len(TOF)] == TOF:
            line = line[len(TOF):]
            values = line.split(",")
            if len(values) == 2:
                tof_queue.put(Tof(*values))

def update_graph():
    point = accel_queue.get()
    x_data.append(point.timestamp)
    y_data.append(point.total_force)
    line.set_data(x_data, y_data)

acquire = threading.Thread(target=acquire_line)
process = threading.Thread(target=process_line)

acquire.start()
process.start()

fig, ax = plt.subplots()
line, = ax.plot([], [], color='blue', marker='o')
x_data = []
y_data = []
ax.set_title("Real-Time Data Stream")
ax.set_xlabel("Time (ms)")
ax.set_ylabel("Gs of force")
ani = FuncAnimation(fig, update_graph, interval=1000, cache_frame_data=False)
plt.show()