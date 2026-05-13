import asyncio
import csv
from datetime import datetime
from bleak import BleakClient, BleakScanner


# ----------------- CONFIG -----------------
DEVICE_NAME = "Nano33_Sensors"
CHARACTERISTIC_UUID = "2A58"  # Must match Arduino code
CSV_FILE = "sensor_log.csv"

# ----------------- BLE HANDLER -----------------
async def run():
    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover()
    arduino_address = None

    for d in devices:
        if d.name and DEVICE_NAME in d.name:  # Fix for None names
            arduino_address = d.address
            print(f"Found Arduino Nano 33: {d.name} [{d.address}]")
            break

    if not arduino_address:
        print("Arduino Nano 33 not found! Make sure it is powered on and in range.")
        return

    async with BleakClient(arduino_address, timeout=20.0) as client:
        print("Connected to Arduino Nano 33")

        # Open CSV file and write headers if not exists
        with open(CSV_FILE, mode="a", newline="") as csvfile:
            csv_writer = csv.writer(csvfile)
            csv_writer.writerow([
                "Timestamp", "Temp_HTU", "Hum_HTU", "Pressure", "Temp_MPL", "Light_Full", "Light_IR"
            ])

            # Callback for incoming notifications
            def handle_data(sender, data):
                text = data.decode("utf-8")
                print("Sensor Data:", text)

                # Parse the comma-separated values from Arduino
                try:
                    parts = text.split(",")
                    values = [p.split(":")[1] for p in parts]
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    csv_writer.writerow([timestamp] + values)
                    csvfile.flush()  # Save immediately
                except Exception as e:
                    print("Error parsing data:", e)

            # Subscribe to notifications
            await client.start_notify(CHARACTERISTIC_UUID, handle_data)
            print("Receiving data... Press Ctrl+C to stop.")

            # Keep running
            try:
                while True:
                    await asyncio.sleep(1)
            except KeyboardInterrupt:
                print("Stopping notifications...")
                await client.stop_notify(CHARACTERISTIC_UUID)
                print("Disconnected")

# ----------------- RUN -----------------
asyncio.run(run())
