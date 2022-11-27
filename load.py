#!/usr/bin/env python3

import serial
import struct
import sys

BOOTLOADER_BAUDRATE = 9600
STX = 0x02
SOH = 0x01
ACK = 0x06
NACK = 0x15

payload = None
with open(sys.argv[2], 'rb') as f:
	payload = f.read()

print(f"Will send {len(payload)} bytes to SC14441 bootloader")

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

	byts = ser.read(1)
	if len(byts) < 1:
		print("Timed out waiting for response to header")
		sys.exit(1)
	byt = byts[0]
	if byt == NACK:
		print("Bootloader refused our payload")
		sys.exit(1)
	if byt != ACK:
		print(f"Unexpected reponse 0x{byt:02x} from bootloader")
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
