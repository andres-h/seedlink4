#!/usr/bin/env python3

import socket

def expect(fd, s):
    line = fd.readline(100).rstrip()
    if line != s:
        raise Exception()

def copydata(ifd, ofd):
    # Copy data to SL4 in legacy format with sequence number
    while True:
        data = ifd.read(520)

        if data[:2] != b"SL":
            print("invalid signature")
            break

        ofd.write(data)
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
    ofd.write(b"FEED\r\n")
    ofd.flush()
    expect(ofd, b"OK")

    copydata(ifd, ofd)

main()

