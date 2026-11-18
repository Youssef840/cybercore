"""Validate the 26-tool lab without connecting or modifying SD.
Leave the touch panel and BOOT button alone while this runs.
"""
import argparse
import json
import re
import time
import serial

parser=argparse.ArgumentParser()
parser.add_argument("port",nargs="?",default="COM4")
args=parser.parse_args()
ser=serial.Serial()
ser.port,ser.baudrate,ser.timeout=args.port,115200,0.1
ser.dtr=ser.rts=False
ser.open()
last_uptime=0

def send(command,seconds=.25):
    ser.write(command.encode("ascii"))
    end=time.monotonic()+seconds
    data=""
    while time.monotonic()<end:
        data+=ser.read_all().decode("utf-8","replace")
        time.sleep(.03)
    assert not any(x in data for x in ("Guru Meditation","Brownout","abort()")),data
    return data

def status(expected):
    global last_uptime
    data=send("V",.3)
    match=re.search(r"uptime=(\d+) heap=(\d+) color=(\d+) touchCal=(\d+) screen=(.+)",data)
    assert match,repr(data)
    uptime,heap,color,calibration,name=match.groups()
    assert name.strip()==expected,repr(name)
    assert int(uptime)>=last_uptime and int(heap)>30000
    assert calibration=="1","Saved touch calibration missing"
    last_uptime=int(uptime)
    return int(heap)

names=["جودة اتصال WiFi","اقتراح قناة WiFi","زمن اتصال TCP","استمرارية HTTP","تعريف الخدمة","اختبار وقت NTP","سجلات AAAA","سجلات MX","سجلات TXT","سجلات NS","بحث DNS عكسي","مقارنة DNS","استقرار DNS","حاسبة الشبكة","تصنيف IPv4","تداخل الشبكات","تنسيق IPv6","تحليل عنوان MAC","توليد كلمة مرور","تحويل Base64","تحويل Hex","ترميز URL","بصمة نص SHA256","بصمة ملف SHA256","توليد UUID","زمن نقل ملف"]
fields=[1,0,3,3,2,1,2,2,2,2,2,3,3,1,1,2,1,1,1,2,2,2,1,1,0,2]
try:
    data=send("J",1)
    assert "[protocol-test] failures=0" in data,data
    assert "[lab-test] failures=0" in data,data
    for _ in range(8):send("B",.1)
    send("L")
    status("المختبر المتقدم")
    for index,name in enumerate(names):
        send(f"!{index:02d}")
        heap=status(name)
        if index>=13 and index not in (18,23):
            data=send("N"*fields[index]+"E",.5)
            assert f"[lab-result] id={index} ok=1" in data,repr(data)
        print(json.dumps({"tool":index,"title":name,"heap":heap,"passed":True},ensure_ascii=True),flush=True)
        send("B",.15)
    send("!01")
    data=send("E",.2)
    for _ in range(32):
        if "[lab-result] id=1" in data:break
        data+=send("V",.5)
    assert "[lab-result] id=1 ok=1" in data,data
    send("B")
    send("L")
    status("المختبر المتقدم")
    print("PASS all 26 pages, calculators, DNS/NTP fixtures and passive channel survey",flush=True)
finally:
    ser.close()
