from pyaccsharedmemory import accSharedMemory
import time
import serial

SERIAL_PORT = "COM9"    # Change to your Arduino/ESP32 port
BAUD = 115200

asm = accSharedMemory()
ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)

try:
    while True:
        sm = asm.read_shared_memory()
        if sm and sm.Physics and sm.Graphics and sm.Static:
            # Fetch fields
            car_name = getattr(sm.Static, "car_model", "N/A")
            # Remove any commas for parsing safety
            car_name = str(car_name).replace(",", " ")
            gear = getattr(sm.Physics, "gear", 0)
            speed_kmh = getattr(sm.Physics, "speed_kmh", None)
            speed_ms = getattr(sm.Physics, "speed_ms", None)
            if speed_kmh is None and speed_ms is not None:
                speed_kmh = speed_ms * 3.6
            elif speed_kmh is None:
                speed_kmh = 0.0
            current_lap_time = getattr(sm.Graphics, "current_time", 0.0)
            is_in_pit = int(getattr(sm.Graphics, "is_in_pit", False))  # 1 or 0
            
            # Compose the message
            msg = f"{int(speed_kmh)},{int(gear)},{float(current_lap_time):.2f},{is_in_pit},{car_name}\n"
            # Print debug message being sent
            print(f"SENDING TO ARDUINO: {msg.strip()}")
            ser.write(msg.encode('utf-8'))
        else:
            print("Shared memory not available. Retrying...")

        time.sleep(0.1)  # 10 Hz
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
    asm.close()
