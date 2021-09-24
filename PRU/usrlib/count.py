import time
import io

def poll_write(pru_msg):
    while True:
        try:
            pru_msg.write("-")
            break
        except io.UnsupportedOperation:
            continue

def count(time_base: float):
    with open("/dev/rpmsg_pru30") as pru_msg:
        poll_write(pru_msg)
        time.sleep(time_base)
        poll_write(pru_msg)
        raw_counts = pru_msg.read()
        counts = []

        print(raw_counts.encode("utf-8").hex())
        #for b in raw_counts:
        #    data[i] = (readBuf[offset+3] << 24) | (readBuf[offset+2] << 16) | (readBuf[offset+1] << 8) | readBuf[offset];
        #    offset+=4;

        pru_msg.read()

count(1)
