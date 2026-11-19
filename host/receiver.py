"""32 MiB ClassPad fxlink RAM dump receiver. Uses Windows libfxlink DLLs."""
import argparse
import ctypes as C
import os
import struct
import time
from pathlib import Path

BASE = 0x88000000
SIZE = 32 * 1024 * 1024
CHUNK = 512 * 1024
TOTAL = SIZE // CHUNK

class Filter(C.Structure):
    _fields_ = [("p7",C.c_bool),("mass_storage",C.c_bool),
                ("intf_fxlink",C.c_bool),("intf_cesg502",C.c_bool),
                ("series_cg",C.c_bool),("series_g3",C.c_bool),
                ("serial",C.c_char_p)]

class Message(C.Structure):
    _fields_ = [("version",C.c_uint32),("size",C.c_uint32),
                ("transfer_size",C.c_uint32),("application",C.c_char*16),
                ("type",C.c_char*16),("_padding",C.c_int),
                ("data",C.c_void_p)]

class Timeval(C.Structure):
    _fields_ = [("tv_sec",C.c_long),("tv_usec",C.c_long)]

def receive(path):
    directory=Path(__file__).resolve().parent
    fx=C.CDLL(str(directory/"libfxlink.dll"))
    usb=C.CDLL(str(directory/"libusb-1.0.dll"))
    usb.libusb_init.argtypes=[C.POINTER(C.c_void_p)]
    usb.libusb_init.restype=C.c_int
    usb.libusb_exit.argtypes=[C.c_void_p]
    usb.libusb_handle_events_timeout_completed.argtypes=[
        C.c_void_p,C.POINTER(Timeval),C.c_void_p]
    usb.libusb_handle_events_timeout_completed.restype=C.c_int
    fx.fxlink_device_find.argtypes=[C.c_void_p,C.POINTER(Filter)]
    fx.fxlink_device_find.restype=C.c_void_p
    fx.fxlink_device_claim_fxlink.argtypes=[C.c_void_p]
    fx.fxlink_device_claim_fxlink.restype=C.c_bool
    fx.fxlink_device_start_bulk_IN.argtypes=[C.c_void_p]
    fx.fxlink_device_start_bulk_IN.restype=C.c_bool
    fx.fxlink_device_finish_bulk_IN.argtypes=[C.c_void_p]
    fx.fxlink_device_finish_bulk_IN.restype=C.POINTER(Message)
    fx.fxlink_device_send_bulk_OUT.argtypes=[
        C.c_void_p,C.c_void_p,C.c_char_p,C.c_char_p,C.c_void_p,C.c_int]
    fx.fxlink_device_send_bulk_OUT.restype=C.c_bool
    fx.fxlink_message_free.argtypes=[C.POINTER(Message),C.c_bool]
    fx.fxlink_device_cleanup.argtypes=[C.c_void_p]

    ctx=C.c_void_p()
    if usb.libusb_init(C.byref(ctx)) != 0:
        raise RuntimeError("libusb_init failed")
    dev=None
    tmp=path+".partial"
    try:
        flt=Filter()
        flt.intf_fxlink=True
        print("Waiting for ClassPad fxlink device...",flush=True)
        deadline=time.monotonic()+30
        while time.monotonic()<deadline:
            dev=fx.fxlink_device_find(ctx,C.byref(flt))
            if dev: break
            usb.libusb_handle_events_timeout_completed(
                ctx,C.byref(Timeval(0,100000)),None)
        if not dev:
            raise RuntimeError("No device found. Check cable, USB mode and WinUSB.")
        if not fx.fxlink_device_claim_fxlink(dev):
            raise RuntimeError("Could not claim fxlink USB interface")
        if not fx.fxlink_device_start_bulk_IN(dev):
            raise RuntimeError("Could not start fxlink receiver")

        cmd=C.create_string_buffer(b"GO32")
        if not fx.fxlink_device_send_bulk_OUT(
            ctx,dev,b"ramdump",b"start",cmd,4):
            raise RuntimeError("Could not send start request")
        print("Capturing 32 MiB...",flush=True)
        with open(tmp,"wb") as output:
            done=0
            last=time.monotonic()
            while done<SIZE:
                rc=usb.libusb_handle_events_timeout_completed(
                    ctx,C.byref(Timeval(0,100000)),None)
                if rc<0: raise RuntimeError(f"libusb event error {rc}")
                msg_ptr=fx.fxlink_device_finish_bulk_IN(dev)
                if not msg_ptr:
                    if time.monotonic()-last>15:
                        raise TimeoutError("Timed out waiting for RAM chunk")
                    continue
                try:
                    msg=msg_ptr.contents
                    if msg.size>CHUNK+8:
                        raise RuntimeError("Unexpected oversized fxlink message")
                    app=bytes(msg.application).split(b"\0",1)[0]
                    kind=bytes(msg.type).split(b"\0",1)[0]
                    if app!=b"ramdump" or kind!=b"data":
                        continue
                    if msg.size!=CHUNK+8:
                        raise RuntimeError(f"Bad message length: {msg.size}")
                    payload=C.string_at(msg.data,msg.size)
                finally:
                    # libfxlink automatically re-submits its bulk-IN transfer.
                    fx.fxlink_message_free(msg_ptr,True)
                offset,length=struct.unpack_from("<II",payload)
                if offset!=done or length!=CHUNK:
                    raise RuntimeError(f"Unexpected chunk offset/size: {offset}/{length}")
                output.write(payload[8:])
                done+=length
                last=time.monotonic()
                print(f"\r{done//1048576:2d}/32 MiB  ({done*100//SIZE}%)",end="",flush=True)
        os.replace(tmp,path)
        print(f"\nSaved {path} ({SIZE} bytes)")
    finally:
        if dev: fx.fxlink_device_cleanup(dev)
        usb.libusb_exit(ctx)

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("output",nargs="?",default="ram_88000000.bin")
    args=parser.parse_args()
    receive(args.output)
