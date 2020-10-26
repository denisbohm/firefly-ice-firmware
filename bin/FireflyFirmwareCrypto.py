#!/usr/bin/env python3

import binascii
from Crypto.Cipher import AES
from collections import namedtuple
import hashlib
from intelhex import IntelHex
import os
import struct
import sys

Version = namedtuple(
    "Version",
    "major minor revision commit"
)

Metadata = namedtuple(
    "Metadata",
    "format flags version address length initialization_vector encrypted_hash uncrypted_hash"
)

Firmware = namedtuple(
    "Firmware",
    "metadata data"
)


class FireflyFirmwareCrypto:

    def __init__(self):
        pass

    @staticmethod
    def load_firmware(path):
        intel_hex = IntelHex(path)
        segments = intel_hex.segments()
        address = segments[0][0]
        address_end = segments[-1][1]
        size = address_end - address
        print("encrypting " + path + " " + str(size) + " bytes at " + ("0x%0.8X" % address))
        data = intel_hex.tobinarray(start=address, size=size)
        return address, data

    @staticmethod
    def replace(collection, start, replacement):
        collection[start:start + len(replacement)] = replacement

    @staticmethod
    def sha1_hash(data):
        sha1_fn = hashlib.sha1()
        sha1_fn.update(data)
        return sha1_fn.digest()

    @staticmethod
    def encrypt_data(key, initialization_vector, data):
        aes = AES.new(key, AES.MODE_CBC, IV=initialization_vector)
        return aes.encrypt(data)

    @staticmethod
    def pad(data, block_size):
        if (len(data) % block_size) == 0:
            return data
        count = block_size - (len(data) % block_size)
        random = os.urandom(count)
        padded_data = data + random
        return padded_data

    @staticmethod
    def encrypt(firmware_path, key, version, comment, addendum, encrypted_firmware_path):
        metadata_block_size = 1024

        (address, data_array) = FireflyFirmwareCrypto.load_firmware(firmware_path)
        data = bytearray(data_array)
        padded_data = bytes(FireflyFirmwareCrypto.pad(data, 16))
        length = len(data)
        uncrypted_hash = FireflyFirmwareCrypto.sha1_hash(data)

        initialization_vector = os.urandom(16)
        encrypted_data = FireflyFirmwareCrypto.encrypt_data(key, initialization_vector, padded_data)
        encrypted_hash = FireflyFirmwareCrypto.sha1_hash(encrypted_data)

        metadata_format = 0x50555746
        flags = 0

        metadata = bytearray()
        metadata += bytearray(struct.pack(
            "<LLLLL", metadata_format, flags, version.major, version.minor, version.revision
        ))
        metadata += version.commit
        metadata += bytearray(struct.pack("<LL", address, length))
        metadata += initialization_vector
        metadata += encrypted_hash
        metadata += uncrypted_hash
        metadata += bytearray(struct.pack("<L", len(comment)))
        metadata += comment.encode("utf-8")
        metadata += bytearray(struct.pack("<L", len(addendum)))
        metadata += addendum.encode("utf-8")
        padded_metadata = metadata + b"\0" * (metadata_block_size - len(metadata))

        uncrypted_padded_metadata = bytes(metadata + os.urandom(metadata_block_size - len(metadata)))
        encrypted_metadata = FireflyFirmwareCrypto.encrypt_data(
            key, initialization_vector, uncrypted_padded_metadata
        )

        binary = bytes(padded_metadata + encrypted_metadata + encrypted_data)

        print("writing " + encrypted_firmware_path)
        with open(encrypted_firmware_path, "wb") as f:
            f.write(binary)

    @staticmethod
    def main(argv):
        firmware_path = "application.hex"
        encrypted_firmware_path = "application.bin"

        key = binascii.unhexlify("000102030405060708090a0b0c0d0e0f")

        version_major = 0
        version_minor = 0
        version_revision = 0
        version_commit = binascii.unhexlify("000102030405060708090a0b0c0d0e0f10111213")

        comment = "testing 1... 2... 3..."

        addendum = "{\"testing\": true}"

        i = 1
        while i < len(argv):
            arg = argv[i]
            if arg == "-firmware":
                i += 1
                firmware_path = argv[i]
            elif arg == "-encrypted-firmware":
                i += 1
                encrypted_firmware_path = argv[i]
            elif arg == "-key":
                i += 1
                key = binascii.unhexlify(argv[i])
            elif arg == "-version":
                i += 1
                version_major = int(argv[i])
                i += 1
                version_minor = int(argv[i])
                i += 1
                version_revision = int(argv[i])
                i += 1
                version_commit = binascii.unhexlify(argv[i])
            elif arg == "-comment":
                i += 1
                comment = argv[i]
            elif arg == "-addendum":
                i += 1
                addendum = argv[i]
            else:
                print("unexpected argument")
            i += 1

        version = Version(major=version_major, minor=version_minor, revision=version_revision, commit=version_commit)
        FireflyFirmwareCrypto.encrypt(firmware_path, key, version, comment, addendum, encrypted_firmware_path)

FireflyFirmwareCrypto.main(sys.argv)
