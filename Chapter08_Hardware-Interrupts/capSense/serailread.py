import serial

if __name__ == "__main__":
    PORT = '/dev/ttyUSB1'
    sensor = serial.Serial(port=PORT)
    val = []
    while 1:
        new_val = sensor.read()
        if new_val == b"\r":
            print(''.join(val))
            val.clear()
        else:
            val.append(new_val.decode())