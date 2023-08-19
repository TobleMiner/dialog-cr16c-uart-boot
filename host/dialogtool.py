#!/usr/bin/env python3

from argparse import ArgumentParser
import os
import serial
import struct
import sys
import threading
from time import sleep
from zlib import crc32
from hexdump import hexdump

RAW_DEBUG = False
DO_HEXDUMP = False

class Bootrom():
	BAUDRATE = 9600
	STX = 0x02
	SOH = 0x01
	ACK = 0x06
	NACK = 0x15

	def __init__(self, port, baudrate=BAUDRATE):
		self.port = port
		self.baudrate = baudrate

	def __enter__(self):
		self.serial = serial.Serial(self.port, self.baudrate, timeout=1)
		return self

	def __exit__(self, *kwargs):
		self.serial.close()

	def reset(self):
		self.serial.rts = True
		self.serial.dtr = False
		sleep(0.1)
		self.serial.rts = False

	def read(self, count):
		data = self.serial.read(count)
		red = "\x1b[31;20m"
		green = "\x1b[32;20m"
		reset = "\x1b[0m"

		if RAW_DEBUG:
			print(f"{green}[Bootrom] RX:\n")
			if DO_HEXDUMP:
				hexdump(data)
			else:
				print((data))
			print(f"\n{reset}")
		return data

	def write(self, data):
		red = "\x1b[31;20m"
		green = "\x1b[32;20m"
		reset = "\x1b[0m"
		if RAW_DEBUG:
			print(f"{red}[Bootrom] TX:\n")
			if DO_HEXDUMP:
				hexdump(data)
			else:
				print((data))
			print(f"\n{reset}")
		return self.serial.write(data)

	def uart_boot(self, payload):
		self.serial.rts = True
		self.serial.dtr = False
		sleep(0.1)
		self.serial.dtr = True
		self.serial.rts = False

		print(f"Will send {len(payload)} bytes to SC14441 bootloader")
		while True:
			byts = self.read(1)
			if len(byts) < 1:
				print("Timed out waiting for STX")
				continue
			byt = byts[0]
			if byt == Bootrom.STX:
				break
			else:
				print(f"Unexpected byte 0x{byt:02x} from bootloader")

		hdr = struct.pack("<BH", Bootrom.SOH, len(payload))
		self.write(hdr)

		stxcnt = 0
		while True:
			byts = self.read(1)
			if stxcnt > 1 or len(byts) < 1:
				print("Timed out waiting for response to header")
				return False
			byt = byts[0]
			if byt == Bootrom.STX:
				stxcnt += 1
				continue
			if byt == Bootrom.ACK:
				break
			if byt == Bootrom.NACK:
				print("Bootloader refused our payload")
				return False
			else:
				print(f"Unexpected response 0x{byt:02x} from bootloader")
				return False

		print("Payload size accepted, sending data")
		self.write(payload)
		checksum = 0
		for byt in payload:
			checksum ^= byt

		byts = self.read(1)
		if len(byts) < 1:
			print("Timed out waiting for response to payload")
			return False
		byt = byts[0]
		if byt == checksum:
			print("Response checksum correct, starting payload")
			self.write(struct.pack("<H", Bootrom.ACK))
			return True
		else:
			print("Response checksum incorrect, aborting")
			return False

	def uart_boot_file(self, file):
		with open(file, 'rb') as f:
			return self.uart_boot(f.read())

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

class ReadNCommand(Command):
	def __init__(self, start_address, length):
		super().__init__(0x09)
		self.start_address = start_address
		self.length = length

	def get_payload(self):
		return struct.pack("<LL", self.start_address, self.length)

	def get_timeout(self, baudrate):
		base = super().get_timeout(baudrate)
		return base + 2 * self.length / (baudrate / 10)

	def __repr__(self):
		return f"ReadFlash(0x{self.start_address:08x}, {self.length})"

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

	def __init__(self, port, baudrate=Bootrom.BAUDRATE):
		self.port = port
		self.baudrate = baudrate
		self.next_id = 0
		self.queued_responses = [ ]
		self.response_available = threading.Condition()

	def __enter__(self):
		self.serial = serial.Serial(self.port, self.baudrate, timeout=1)
		self.start()
		return self

	def __exit__(self, *kwargs):
		self.stop()
		self.serial.close()

	def read(self, count):
		data = self.serial.read(count)
		red = "\x1b[31;20m"
		green = "\x1b[32;20m"
		reset = "\x1b[0m"

		if RAW_DEBUG:
			print(f"{green}[loader] RX:\n")
			if DO_HEXDUMP:
				hexdump(data)
			else:
				print((data))
			print(f"\n{reset}")
		return data

	def write(self, data):
		red = "\x1b[31;20m"
		green = "\x1b[32;20m"
		reset = "\x1b[0m"
		if RAW_DEBUG:
			print(f"{red}[loader] TX:\n")
			if DO_HEXDUMP:
				hexdump(data)
			else:
				print((data))
			print(f"\n{reset}")
		return self.serial.write(data)

	def send_command(self, cmd):
		dispatch = DispatchedCommand(cmd, self.next_id)
		self.next_id += 1
		print(f"Dispatching command {dispatch}")
		self.write(dispatch.encode())
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
		sync = self.read(1)
		if len(sync) == 0:
			return None
		if sync[0] == LoaderSession.SYNC_BYTE:
			self.serial.timeout = 1
			header_data = self.read(ResponseHeader.LENGTH)
			header = ResponseHeader.parse(header_data)
			if not header:
				return None
			payload_with_crc = None
			if header.payload_length:
				self.serial.timeout = 1 + header.payload_length_with_crc * 10 / self.serial.baudrate
				payload_with_crc = self.read(header.payload_length_with_crc)
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

	def ping(self):
		cmd = PingCommand()
		dispatch = self.send_command(cmd)
		resp = self.await_response(dispatch)
		return resp and isinstance(resp, SyncResponse)

	def sync(self, tries=3):
		for try_ in range(tries):
			if self.ping():
				print(f"Synchronized in {try_ + 1} attempts")
				return True
		return False

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

	def read_mem_chunk(self, start, length):
		cmd = ReadNCommand(start, length)
		dispatch = self.send_command(cmd)
		resp = self.await_response(dispatch)
		if resp and isinstance(resp, SyncResponse):
			return resp.payload
		return None

	def read_mem(self, start, length, retry=5, chunk_size=4096):
		address = start
		data = b''
		while length:
			read_size = length
			if read_size > chunk_size:
				read_size = chunk_size
			for try_ in range(retry):
				chunk = self.read_mem_chunk(address, read_size)
				if chunk:
					data += chunk
					break
				print(f"Failed to read chunk at 0x{address:08x}, try {try_ + 1}/{retry}")
			else:
				return None

			length -= read_size
			address += read_size

		return data

	def set_baudrate(self, baudrate):
		cmd = SetBaudrateCommand(baudrate)
		dispatch = self.send_command(cmd)
		resp = self.await_response(dispatch)
		if resp and isinstance(resp, ErrorResponse):
			return False
		self.stop()
		self.serial.baudrate = baudrate
		self.queued_responses.clear()
		self.start()
		return True

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

def int_autobase(x):
	return int(x, 0)

class CliCommand():
	def __init__(self):
		self.args = None

	def run(self, args, parser):
		if not self.parse_args(parser):
			sys.exit(1)

		if not args.skip_loader:
			with Bootrom(args.port, args.initial_baudrate) as bootrom:
				bootrom.uart_boot_file(args.loader)

		with LoaderSession(args.port, args.initial_baudrate) as session:
			if not session.sync():
				print(f"Failed to synchronize with loader")
				sys.exit(1)

			if session.baudrate != args.baudrate:
				print(f"Changing baudrate {session.baudrate} -> {args.baudrate}")
				session.set_baudrate(args.baudrate)
				if not session.sync():
					print(f"Failed to synchronize with loader after baudrate change")
					sys.exit(1)

			self.execute(session)

	def parse_args(self, parser):
		return True

	def execute(self, session):
		raise NotImplementedError()

class CliCommandChipId(CliCommand):
	def __init__(self):
		super().__init__()

	def execute(self, session):
		print(session.chip_id())


class CliCommandFlashInfo(CliCommand):
	def __init__(self):
		super().__init__()

	def execute(self, session):
		print(session.flash_info())

class CliCommandReadFlash(CliCommand):
	def __init__(self):
		super().__init__()

	def parse_args(self, parser):
		parser.add_argument("filename")
		parser.add_argument("offset", type=int_autobase, nargs="?")
		parser.add_argument("length", type=int_autobase, nargs="?")
		self.args = parser.parse_args()
		return True

	def execute(self, session):
		offset = self.args.offset
		if offset is None:
			offset = 0
		length = self.args.length
		if length is None:
			flash_info = session.flash_info()
			if not flash_info or not isinstance(flash_info, FlashInfoResponse):
				print("Failed to determine flash size, must specify read length manually")
				return
			length = flash_info.flash_size_bytes
		print(f"Will read {length} bytes from 0x{offset:08x} - 0x{offset + length - 1:08x}")
		with open(self.args.filename, 'wb') as f:
			f.write(session.read_flash(offset, length))

class CliCommandWriteFlash(CliCommand):
	def __init__(self):
		super().__init__()
		self.offset = None
		self.length = None

	def parse_args(self, parser):
		parser.add_argument("filename")
		parser.add_argument("offset", type=int_autobase, nargs="?")
		parser.add_argument("length", type=int_autobase, nargs="?")
		self.args = parser.parse_args()

		offset = self.args.offset
		if offset is None:
			offset = 0

		if offset % 0x1000:
			print("Unaligned flash writes not supported, offset must be aligned with 4096 byte sectors")
			return False
		self.offset = offset

		length = self.args.length
		if length is None:
			length = os.path.getsize(self.args.filename)
		if (offset + length) % 0x1000:
			print("Unaligned flash writes not supported, (offset + size) must be aligned with 4096 byte sectors")
			return False
		self.length = length

		return True


	def execute(self, session):
		flash_data = None
		with open(self.args.filename, 'rb') as f:
			flash_data = f.read()

		if len(flash_data) < self.offset + self.length:
			print(f"Failed to write to flash, input file shorter than (offset + length)")
			return False

		for sector_address in range(self.offset, self.offset + self.length, 0x1000):
			if not session.erase_flash_sector(sector_address):
				print(f"Failed to erase sector @0x{sector_address:08x}")
				return False

		for page_address in range(self.offset, self.offset + self.length, 256):
			if not session.program_flash_page(page_address, flash_data[page_address:page_address + 256]):
				print(f"Failed to write page @0x{sector_address:08x}")
				return False

		return True

class CliCommandReset(CliCommand):
	def run(self, args, parser):
		with Bootrom(args.port) as bootrom:
			bootrom.reset()

class CliCommandReadMem(CliCommand):
	def __init__(self):
		super().__init__()

	def parse_args(self, parser):
		parser.add_argument("filename")
		parser.add_argument("offset", type=int_autobase, nargs="?")
		parser.add_argument("length", type=int_autobase, nargs="?")
		self.args = parser.parse_args()
		return True

	def execute(self, session):
		offset = self.args.offset
		if offset is None:
			offset = 0
		length = self.args.length
		print(f"Will read {length} bytes from 0x{offset:08x} - 0x{offset + length - 1:08x}")
		with open(self.args.filename, 'wb') as f:
			f.write(session.read_mem(offset, length))

CLI_COMMANDS = {
	"chip_id": CliCommandChipId,
	"flash_info": CliCommandFlashInfo,
	"read_flash": CliCommandReadFlash,
	"write_flash": CliCommandWriteFlash,
	"reset": CliCommandReset,
	"read_mem": CliCommandReadMem,
}

script_dir = os.path.dirname(os.path.realpath(__file__))
parser = ArgumentParser(prog="dialogtool.py", description="Dialog UART bootloader tool")
parser.add_argument("-p", "--port", default="/dev/ttyUSB0")
parser.add_argument("-b", "--baudrate", type=int, default=230400)
parser.add_argument("-l", "--loader", default=f"{script_dir}/../device/test.bin")
parser.add_argument("--skip-loader", action="store_true", help="Skip loader upload")
parser.add_argument("--initial-baudrate", type=int, default=Bootrom.BAUDRATE, help="Set baudrate used for intial communication")
parser.add_argument("command", choices=CLI_COMMANDS.keys())
(args, excess_args) = parser.parse_known_args()

cmd = CLI_COMMANDS[args.command]()
cmd.run(args, parser)
