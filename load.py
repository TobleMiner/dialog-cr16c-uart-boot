#!/usr/bin/env python3

import serial
import struct
import sys
import threading
from time import sleep
from zlib import crc32

BOOTLOADER_BAUDRATE = 9600
STX = 0x02
SOH = 0x01
ACK = 0x06
NACK = 0x15

payload = None
with open(sys.argv[2], 'rb') as f:
	payload = f.read()

print(f"Will send {len(payload)} bytes to SC14441 bootloader")

class Command():
	def __init__(self, cmd):
		self.cmd = cmd

	def get_payload(self):
		return None

	def expect_response(self):
		return True

	def encode(self, id):
		payload = self.get_payload()
		if not payload:
			payload = b''
		sync = LoaderSession.SYNC_BYTE.to_bytes(1, byteorder="little")
		header = struct.pack("<BLL", self.cmd, id, len(payload))
		data = sync + header + crc32(header).to_bytes(4, byteorder="little")
#		print(f'CRC32 python {(header + crc32(header).to_bytes(4, byteorder="little")).hex()}')
		if len(payload) > 0:
			data += payload
			data += crc32(payload).to_bytes(4, byteorder="little")
#		print(f'Command data python {data.hex()}')
		return data

	def get_timeout(self, baudrate):
		return 1

class DispatchedCommand():
	def __init__(self, cmd, id):
		self.cmd = cmd
		self.id = id

	def encode(self):
		return self.cmd.encode(self.id)

	def __repr__(self):
		return f"Dispatch, id: {self.id}, cmd: {str(self.cmd)}"

class PingCommand(Command):
	def __init__(self):
		super().__init__(0x00)

class ReadFlashCommand(Command):
	def __init__(self, start_address, length):
		super().__init__(0x06)
		self.start_address = start_address
		self.length = length

	def get_payload(self):
		return struct.pack("<LL", self.start_address, self.length)

	def get_timeout(self, baudrate):
		base = super().get_timeout(baudrate)
		return base + 2 * self.length / (baudrate / 10)

	def __repr__(self):
		return f"ReadFlash(0x{self.start_address:08x}, {self.length})"

class SetBaudrateCommand(Command):
	def __init__(self, baudrate):
		super().__init__(0x01)
		self.baudrate = baudrate

	def get_payload(self):
		return struct.pack("<L", self.baudrate)

class EraseFlashSectorCommand(Command):
	def __init__(self, address):
		super().__init__(0x03)
		self.address = address

	def get_payload(self):
		return struct.pack("<L", self.address)

	def get_timeout(self, baudrate):
		base = super().get_timeout(baudrate)
		return base + 0.5

	def __repr__(self):
		return f"EraseFlashSector(0x{self.address:08x})"

class ProgramFlashPageCommand(Command):
	def __init__(self, start_address, data):
		super().__init__(0x04)
		self.start_address = start_address
		self.data = data

	def get_payload(self):
		return struct.pack("<L", self.start_address) + self.data

	def get_timeout(self, baudrate):
		base = super().get_timeout(baudrate)
		return base + 2 * len(self.data) / (baudrate / 10) + 0.003

	def __repr__(self):
		return f"ProgramFlashPage(0x{self.start_address:08x})"

class RemoteFlashChecksumCommand(Command):
	def __init__(self, start_address, length):
		super().__init__(0x07)
		self.start_address = start_address
		self.length = length

	def get_payload(self):
		return struct.pack("<LL", self.start_address, self.length)

	def get_timeout(self, baudrate):
		base = super().get_timeout(baudrate)
		return base + self.length * 8 / 100000

	def __repr__(self):
		return f"RemoteFlashChecksum(0x{self.start_address:08x}, {self.length})"

class FlashInfoCommand(Command):
	def __init__(self):
		super().__init__(0x02)

	def __repr__(self):
		return f"FlashInfo()"

class ChipIdCommand(Command):
	def __init__(self):
		super().__init__(0x08)

	def __repr__(self):
		return f"ChipId()"

class ResponseHeader():
	LENGTH = 13

	@staticmethod
	def parse(data):
		if len(data) != ResponseHeader.LENGTH:
			return None
		(response, id, length, checksum) = struct.unpack("<BLLL", data)
		checksum_check = crc32(b'\xA5' + data[:-4])
		if checksum != checksum_check:
			print(data.hex())
			print(f"Corrupted header, checksum incorrect (expected 0x{checksum_check:08x}, but got 0x{checksum:08x})")
			return None
		return ResponseHeader(response, id, length)

	def __init__(self, response, id, payload_length):
		self.response = response
		self.id = id
		self.payload_length = payload_length
		self.payload_length_with_crc = payload_length + 4

class Response():
	@staticmethod
	def parse(header, data):
		RESPONSE_CODE_MAP = {
			ErrorResponse: ErrorResponse.RESPONSE_CODES,
			SyncResponse: SyncResponse.RESPONSE_CODES,
			DebugResponse: DebugResponse.RESPONSE_CODES,
			ChecksumResponse: ChecksumResponse.RESPONSE_CODES,
			FlashInfoResponse: FlashInfoResponse.RESPONSE_CODES,
			ChipIdResponse: ChipIdResponse.RESPONSE_CODES
		}
		payload = b''
		if data:
			if len(data) != header.payload_length_with_crc:
				return None
			payload = data[:-4]
			checksum = int.from_bytes(data[-4:], byteorder='little')
			checksum_check = crc32(payload)
			if checksum != checksum_check:
				print(f"Corrupted header, checksum incorrect (expected 0x{checksum_check:08x}, but got 0x{checksum:08x})")
				print(payload)
				return None

		for (resp_type, response_codes) in RESPONSE_CODE_MAP.items():
			if header.response in response_codes:
				if not resp_type.validate(payload):
					continue
				return resp_type(header, payload)

		return Response(header, payload)

	@classmethod
	def validate(self, payload):
		return True

	def handle(self):
		return False

	def __init__(self, header, payload):
		self.header = header
		self.payload = payload

	def __repr__(self):
		return f"Response to 0x{self.header.id:04x}, type 0x{self.header.response:02x}, {len(self.payload)} bytes of data"

class ErrorResponse(Response):
	RESPONSE_CODES = [ 0x00, 0x02, 0x03, 0x06, 0x08 ]

	def __init__(self, header, payload):
		super().__init__(header, payload)

class SyncResponse(Response):
	RESPONSE_CODES = [ 0x01, 0x04 ]

	def __init__(self, header, payload):
		super().__init__(header, payload)

class DebugResponse(Response):
	RESPONSE_CODES = [ 0x05 ]

	def __init__(self, header, payload):
		super().__init__(header, payload)

	def handle(self):
		print(''.join(chr(b) for b in self.payload), end='')
#		print('DEBUG: ' + ''.join(chr(b) for b in self.payload))
		return True

class ChecksumResponse(Response):
	RESPONSE_CODES = [ 0x09 ]

	@classmethod
	def validate(self, payload):
		return len(payload) == 4

	def __init__(self, header, payload):
		super().__init__(header, payload)
		self.checksum = struct.unpack("<L", payload)[0]

	def __repr__(self):
		return f"ChecksumResponse to 0x{self.header.id:04x}, checksum 0x{self.checksum:08x}"

class FlashInfoResponse(Response):
	RESPONSE_CODES = [ 0x0A ]

	@classmethod
	def validate(self, payload):
		return len(payload) == 4

	def __init__(self, header, payload):
		super().__init__(header, payload)
		self.flash_size_bytes = struct.unpack("<L", payload)[0]

	def __repr__(self):
		return f"FlashInfoResponse to 0x{self.header.id:04x}, flash size {self.flash_size_bytes} bytes"

class ChipIdResponse(Response):
	RESPONSE_CODES = [ 0x0B ]

	@classmethod
	def validate(self, payload):
		return len(payload) == 5

	def __init__(self, header, payload):
		super().__init__(header, payload)
		self.id1 = payload[0]
		self.id2 = payload[1]
		self.id3 = payload[2]
		self.mem_size = payload[3]
		self.revision = payload[4]

	def __repr__(self):
		revision_major = chr(ord('A') + (self.revision >> 4))
		revision_minor = chr(ord('A') + (self.revision & 0x0f))
		return f"ChipIdResponse to {self.header.id:04x}, " + \
			f"chip id: '{chr(self.id1)}{chr(self.id2)}{chr(self.id3)}'(0x{self.id1:02x}{self.id2:02x}{self.id3:02x}), " + \
			f"mem size: 0x{self.mem_size:02x}, revision: {revision_major}x{revision_minor}(0x{self.revision:02x})"

class LoaderSession():
	SYNC_BYTE = 0xA5

	def __init__(self, serial):
		self.serial = serial
		self.next_id = 0
		self.queued_responses = [ ]
		self.response_available = threading.Condition()
		self.start()

	def send_command(self, cmd):
		dispatch = DispatchedCommand(cmd, self.next_id)
		self.next_id += 1
		print(f"Dispatching command {dispatch}")
		self.serial.write(dispatch.encode())
		return dispatch

	def listen(self):
		while not self.exit:
			resp = self.receive_packet()
			if not resp:
				continue

			if resp.handle():
				continue

			self.response_available.acquire()
			print(resp)
			self.queued_responses.append(resp)
			self.response_available.notify_all()
			self.response_available.release()

	def receive_packet(self):
		sync = self.serial.read(1)
		if len(sync) == 0:
			return None
		if sync[0] == LoaderSession.SYNC_BYTE:
			self.serial.timeout = 1
			header_data = self.serial.read(ResponseHeader.LENGTH)
			header = ResponseHeader.parse(header_data)
			if not header:
				return None
			payload_with_crc = None
			if header.payload_length:
				self.serial.timeout = 1 + header.payload_length_with_crc * 10 / self.serial.baudrate
				payload_with_crc = self.serial.read(header.payload_length_with_crc)
			return Response.parse(header, payload_with_crc)

	def find_response(self, id):
		for resp in self.queued_responses:
			if resp.header.id == id:
				return resp
		return None

	def await_response(self, dispatch, timeout=False):
		if isinstance(timeout, bool) and timeout == False:
			timeout = dispatch.cmd.get_timeout(self.serial.baudrate)

		self.response_available.acquire()
		resp = self.find_response(dispatch.id)
		if resp:
			self.queued_responses.remove(resp)
			self.response_available.release()
			return resp

		print(f"Timeout: {timeout}s")
		while self.response_available.wait(timeout):
			resp = self.find_response(dispatch.id)
			if resp:
				self.queued_responses.remove(resp)
				return resp

		return None

	def start(self):
		self.exit = False
		self.listen_thread = threading.Thread(target=self.listen)
		self.listen_thread.start()

	def stop(self):
		self.exit = True
		self.listen_thread.join()

	def read_flash_chunk(self, start, length):
		cmd = ReadFlashCommand(start, length)
		dispatch = self.send_command(cmd)
		resp = self.await_response(dispatch)
		if resp and isinstance(resp, SyncResponse):
			return resp.payload
		return None

	def read_flash(self, start, length, retry=5, chunk_size=4096):
		address = start
		data = b''
		while length:
			read_size = length
			if read_size > chunk_size:
				read_size = chunk_size
			for try_ in range(retry):
				chunk = self.read_flash_chunk(address, read_size)
				if chunk:
					data += chunk
					break
				print(f"Failed to read chunk at 0x{address:08x}, try {try_ + 1}/{retry}")
			else:
				return None

			length -= read_size
			address += read_size

		return data

	def set_baudrate(self, baudrate, connect_retry=5):
		cmd = SetBaudrateCommand(baudrate)
		dispatch = self.send_command(cmd)
		resp = self.await_response(dispatch)
		if resp and isinstance(resp, ErrorResponse):
			return False
		self.stop()
		self.serial.baudrate = baudrate
		self.queued_responses.clear()
		self.start()
		for _ in range(connect_retry):
			dispatch = self.send_command(PingCommand())
			if session.await_response(dispatch):
				return True

		return False

	def erase_flash_sector(self, address):
		cmd = EraseFlashSectorCommand(address)
		dispatch = self.send_command(cmd)
		resp = self.await_response(dispatch)
		return (resp and isinstance(resp, SyncResponse))

	def program_flash_page(self, address, data):
		cmd = ProgramFlashPageCommand(address, data)
		dispatch = self.send_command(cmd)
		resp = self.await_response(dispatch)
		return (resp and isinstance(resp, SyncResponse))

	def remote_flash_checksum(self, address, length):
		cmd = RemoteFlashChecksumCommand(address, length)
		dispatch = self.send_command(cmd)
		resp = self.await_response(dispatch)
		return (resp and isinstance(resp, ChecksumResponse))

	def flash_info(self):
		cmd = FlashInfoCommand()
		dispatch = self.send_command(cmd)
		return self.await_response(dispatch)

	def chip_id(self):
		cmd = ChipIdCommand()
		dispatch = self.send_command(cmd)
		return self.await_response(dispatch)

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

	session = LoaderSession(ser)
#	session.attach()
	dispatch = session.send_command(PingCommand())
	print(session.await_response(dispatch))

	dispatch = session.send_command(PingCommand())
	print(session.await_response(dispatch))

	dispatch = session.send_command(PingCommand())
	print(session.await_response(dispatch))

	print(session.set_baudrate(115200 * 2))

	print(session.remote_flash_checksum(0x0, 0x100))

	print(session.flash_info())

	print(session.chip_id())

#	sleep(1)
	"""
	print("Reading sector 0 before erase...")
	flash_data = session.read_flash(0x0, 0x1000)
	print("Sector 0 before erase:")
	print(flash_data)
	print("Erasing sector 0...")
	print(session.erase_flash_sector(0x0))
	print("Reading sector 0 after erase...")
	flash_data = session.read_flash(0x0, 0x1000)
	print("Sector 0 after erase:")
	print(flash_data)
	print("Programming page 0 after erase...")
	page_data = b''.join([ i.to_bytes(1, byteorder='little') for i in range(256) ])
	print(session.program_flash_page(0x0, page_data))
	print("Reading page 0 after programming...")
	flash_data = session.read_flash(0x0, 0x100)
	print("Sector 0 after programming:")
	print(flash_data)
	"""
	sleep(5)

	session.stop()

