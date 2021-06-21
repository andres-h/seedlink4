#!/usr/bin/env python3

import socket
import struct
import subprocess

def expect(fd, s):
    line = fd.readline(100).rstrip()
    if line != s:
        raise Exception()

def copydata(ifd, ofd):
    while True:
        sig = ifd.read(2)
        if sig != b"SL":
            print("invalid signature")
            break

        ifd.read(6)  # throw away sequence number
        msrec = ifd.read(512)

        ofd.write(b"SE2\0")  # signature, format code and reserved byte
        ofd.write(struct.pack("<L", 520)) # length of following data
        ofd.write(struct.pack("<q", -1))  # undefined seq
        ofd.write(msrec)

        ms3rec = subprocess.run("./ms2to3", input=msrec, stdout=subprocess.PIPE).stdout
        ofd.write(b"SE3\0")  # signature, format code and reserved byte
        ofd.write(struct.pack("<L", len(ms3rec) + 8)) # length of following data
        ofd.write(struct.pack("<q", -1))  # undefined seq
        ofd.write(ms3rec)

        ofd.flush()

def main():
    isock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    isock.settimeout(60)
    isock.connect(('geofon.gfz-potsdam.de', 18000))
    ifd = isock.makefile('rwb')

    ifd.write(b"STATION WLF GE\r\n")
    ifd.flush()
    expect(ifd, b"OK")
    ifd.write(b"DATA\r\n")
    ifd.flush()
    expect(ifd, b"OK")
    ifd.write(b"END\r\n")
    ifd.flush()

    osock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    osock.settimeout(60)
    osock.connect(('localhost', 18000))
    ofd = osock.makefile('rwb')
    ofd.write(b"AUTH USERPASS test test\r\n")
    ofd.flush()
    expect(ofd, b"OK")
    ofd.write(b"FEED\r\n")
    ofd.flush()
    expect(ofd, b"OK")

    copydata(ifd, ofd)

main()

