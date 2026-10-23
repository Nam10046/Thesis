import serial, time, sys

def p(*a):
    print(*a); sys.stdout.flush()

PORT = "COM4"
BAUD = 115200

ser = serial.Serial()
ser.port = PORT
ser.baudrate = BAUD
ser.timeout = 0.2
ser.dtr = False
ser.rts = False
ser.open()
p("port open, listening only for 8 seconds, no writes at all...")

end = time.time() + 8
total = 0
while time.time() < end:
    n = ser.in_waiting
    if n:
        data = ser.read(n)
        total += len(data)
        p("[%.2fs] got %d bytes: %s" % (time.time()-end+8, len(data), data))
    else:
        time.sleep(0.1)

p("done. total bytes received with zero writes:", total)
ser.close()
