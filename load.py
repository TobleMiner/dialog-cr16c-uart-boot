#!/usr/bin/env python3

import serial
import struct
import sys
from time import sleep
from zlib import crc32

BOOTLOADER_BAUDRATE = 9600
STX = 0x02
SOH = 0x01
ACK = 0x06
NACK = 0x15

HEADER_BYTE = 0xA5
RESPONSE_DEBUG = 0x05

payload = None
with open(sys.argv[2], 'rb') as f:
	payload = f.read()

print(f"Will send {len(payload)} bytes to SC14441 bootloader")

def receive_packet(ser):
	header = ser.read(13)
	if len(header) == 13:
		(response, id, length, checksum) = struct.unpack("<BLLL", header)
		checksum_check = crc32(b'\xA5' + header[:-4])
		if checksum != checksum_check:
			print(f"Corrupted header, checksum incorrect (expected 0x{checksum_check:08x}, but got 0x{checksum:08x})")
			return
		if length:
			payload = ser.read(length)
			checksum_buf = ser.read(4)
			if len(payload) == length and len(checksum_buf) == 4:
				checksum = int.from_bytes(checksum_buf, byteorder='little')
				checksum_check = crc32(payload)
				if checksum != checksum_check:
					print(f"Corrupted payload, checksum incorrect")
					return
				print(''.join(chr(b) for b in payload), end='')
			else:
				print(f"Incomplete payload, {len(payload)+ len(checksum_buf)} bytes")
	else:
		print(f"Incomplete header, {len(header)} bytes")

with serial.Serial(sys.argv[1], BOOTLOADER_BAUDRATE, timeout=1) as ser:
	while True:
		byts = ser.read(1)
		if len(byts) < 1:
			print("Timed out waiting for STX")
			continue
		byt = byts[0]
		if byt == STX:
			break
		else:
			print(f"Unexpected byte 0x{byt:02x} from bootloader")

	hdr = struct.pack("<BH", SOH, len(payload))
	ser.write(hdr)

	stxcnt = 0
	while True:
		byts = ser.read(1)
		if stxcnt > 1 or len(byts) < 1:
			print("Timed out waiting for response to header")
			sys.exit(1)
		byt = byts[0]
		if byt == STX:
			stxcnt += 1
			continue
		if byt == ACK:
			break
		if byt == NACK:
			print("Bootloader refused our payload")
			sys.exit(1)
		else:
			print(f"Unexpected response 0x{byt:02x} from bootloader")
			sys.exit(1)

	print("Payload size accepted, sending data")
	ser.write(payload)
	checksum = 0
	for byt in payload:
		checksum ^= byt

	byts = ser.read(1)
	if len(byts) < 1:
		print("Timed out waiting for response to payload")
		sys.exit(1)
	byt = byts[0]
	if byt == checksum:
		print("Response checksum correct, starting payload")
		ser.write(struct.pack("<H", ACK))
	else:
		print("Response checksum incorrect, aborting")
		sys.exit(1)

	checksum = crc32(b'\xff\x42')
	print(checksum);
	print(f"CRC32: 0x{checksum:08x}")

	ser.timeout = 1
	while True:
		hdr = ser.read(1)
		if len(hdr) == 1:
			if hdr[0] == HEADER_BYTE:
				receive_packet(ser)
			else:
				print("Unexpected header byte 0x{hdr[0]:02x}")
"""
		data = ser.read(32)
		if len(data):
			print(''.join(chr(b) for b in data), end='')
"""
