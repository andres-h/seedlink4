#!/usr/bin/env python3

import socket
import struct

def expect(fd, s):
    line = fd.readline(100).rstrip()
    if line != s:
        print("line " + repr(line))
        raise Exception()

def copydata(ifd, ofd):
    while True:
        sig = ifd.read(4)

        if sig[:3] != b"SE3":
            print("invalid signature")
            break

        plen = struct.unpack("<L", ifd.read(4))[0]
        ifd.read(8)  # discard sequence number
        slen = struct.unpack("<B", ifd.read(1))[0]
        ifd.read(slen)  # discard station ID
        msrec = ifd.read(plen)
        ofd.write(msrec)
        ofd.flush()

def main():
    isock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    isock.settimeout(60)
    isock.connect(('localhost', 18000))
    ifd = isock.makefile('rwb')

    ifd.write(b"SLPROTO 4.0\r\n")
    ifd.flush()
    expect(ifd, b"OK")
    ifd.write(b"STATION * GE\r\n")
    ifd.flush()
    expect(ifd, b"OK")
    ifd.write(b"SELECT *.3\r\n")
    ifd.flush()
    expect(ifd, b"OK")
    ifd.write(b"DATA\r\n")
    ifd.flush()
    expect(ifd, b"OK")
    ifd.write(b"END\r\n")
    ifd.flush()

    ofd = open("output.ms3", "wb")

    copydata(ifd, ofd)

main()
