"""Read-only radio/UI smoke test. Run without touching the panel during the test.
Requires pyserial. Does not delete SD files or connect to any network.
"""
import argparse
import re
import time
import serial

parser = argparse.ArgumentParser()
parser.add_argument("port", nargs="?", default="COM4")
args = parser.parse_args()
ser = serial.Serial()
ser.port, ser.baudrate, ser.timeout = args.port, 115200, 0.1
ser.dtr = ser.rts = False
ser.open()
last_uptime = 0

def send(command, seconds=0.25):
    ser.write(command.encode("ascii"))
    time.sleep(seconds)
    data = ser.read_all().decode("utf-8", "replace")
    assert not any(x in data for x in ("Guru Meditation", "Brownout", "abort()")), data
    return data

def status(expected=None):
    global last_uptime
    data = send("V", 0.4)
    pattern = r"\[status\] version=(\S+) uptime=(\d+) heap=(\d+) color=(\d+) touchCal=(\d+) screen=(.+)"
    deadline = time.monotonic() + 3
    match = re.search(pattern, data)
    while not match and time.monotonic() < deadline:
        data += ser.read(2048).decode("utf-8", "replace")
        match = re.search(pattern, data)
    assert match, repr(data)
    version, uptime, heap, color, calibration, screen = match.groups()
    assert int(uptime) >= last_uptime, "Unexpected reboot"
    assert int(heap) > 30000, "Low heap"
    if expected:
        assert screen.strip() == expected, repr(screen)
    last_uptime = int(uptime)
    print(f"PASS screen={screen!a} heap={heap} color={color}", flush=True)
    return screen.strip()

try:
    for _ in range(8):
        send("B")
    status("القائمة الرئيسية")
    send("E")
    status("ادوات الواي فاي")
    send("1")  # category page can retain the user-selected page
    status("ماسح الشبكات")  # status must respond while scan is still running
    time.sleep(5)
    status("ماسح الشبكات")
    send("B"); send("B")
    for _ in range(2):
        send("5")
        status("ماسح بلوتوث")
        time.sleep(4)
        status("ماسح بلوتوث")
        send("B")
    for command, name in (("j","شبكات مخفية"),("i","مدير ملفات SD"),("k","مراقب جهاز BLE"),("l","ساعة ومنبه"),("m","خيارات الادوات"),("2","تدقيق الحماية"),("3","محلل القنوات"),("a","كاشف التوأم"),("4","كاشف الفصل"),("b","محلل الحركة"),("n","تدقيق PMF و WPS"),("o","فاحص الخدمات"),("p","تشخيص DNS"),("r","مراقب تغير الشبكات"),("s","محلل اعلانات BLE"),("t","فاحص رؤوس HTTP"),("g","رمز QR")):
        send(command)
        status(name)
        send("B")
    tests=send("J",1)
    assert "[protocol-test] failures=0" in tests
    assert "[lab-test] failures=0" in tests
    send("9")
    status("الاعدادات")
    print("PASS all smoke checks", flush=True)
finally:
    ser.close()
