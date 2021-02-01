#!/usr/bin/env python3

import socket

def expect(fd, s):
    line = fd.readline(100).rstrip()
    if line != s:
        print("line " + repr(line))
        raise Exception()

def copydata(ifd, ofd):
    while True:
        data = ifd.read(528)  # assume 512b mseed and 16b header

        if data[:2] != b"SE":
            print("invalid signature")
            break

        ofd.write(data[16:])
        ofd.flush()

def main():
    isock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    isock.settimeout(60)
    isock.connect(('localhost', 18000))
    ifd = isock.makefile('rwb')

    # 50 = ord('2') = FMT_MSEED24
    ifd.write(b"ACCEPT 50 51\r\n")
    ifd.flush()
    expect(ifd, b"OK")
    ifd.write(b"STATION WLF GE\r\n")
    ifd.flush()
    expect(ifd, b"OK")
    ifd.write(b"DATA\r\n")
    ifd.flush()
    expect(ifd, b"OK")
    ifd.write(b"END\r\n")
    ifd.flush()

    ofd = open("output.mseed", "wb")

    copydata(ifd, ofd)

main()
