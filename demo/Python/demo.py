import ctypes
import time
from ctypes import wintypes


class HWEngine:
    def __init__(self, dll_path="./CHWEngine.dll"):
        try:
            self.dll = ctypes.WinDLL(dll_path)
        except Exception as e:
            print(f"Error: Could not load DLL from {dll_path}\n{e}")
            exit(1)

        # Buffer configuration for string outputs
        self._buf_size = 256
        self._buffer = ctypes.create_unicode_buffer(self._buf_size)

    def get_count(self, hw_name):
        """Calls Get<hwName>Count"""
        func = getattr(self.dll, f"Get{hw_name}Count")
        return func()

    def get_str(self, func_name, index=0):
        """Generic wrapper for string-based Getters"""
        func = getattr(self.dll, func_name)
        
        # Explicitly define argument types for safety (Optional but recommended)
        # index: int, buffer: wchar_p, maxLen: int
        func.argtypes = [ctypes.c_int, wintypes.LPWSTR, ctypes.c_int]
        
        # Use the correct attribute name: self._buf_size
        func(index, self._buffer, self._buf_size)
        return self._buffer.value

    def get_val(self, func_name, restype, index=0):
        """Generic wrapper for value-based Getters (DWORD, UINT64, etc.)"""
        func = getattr(self.dll, func_name)
        func.restype = restype
        # Signature: type func(int index)
        return func(index)

def run_hardware_demo():
    engine = HWEngine()
    
    print("=" * 50)
    print("CHWEngine SDK - Python Demo")
    print("=" * 50)

    # Track execution time
    start_time = time.perf_counter()

    mb_count = engine.get_count("Board")
    print(f"\n[Motherboard]")
    for i in range(mb_count):
        make = engine.get_str("GetBoardMake", i)
        sys_name = engine.get_str("GetBoardSystemName", i)
        model = engine.get_str("GetBoardModel", i)
        chipset_name = engine.get_str("GetBoardChipsetName", i)
        bios_version = engine.get_str("GetBoardBiosVersion", i)
        print(f"Manufacturer: {make}")
        print(f"System Name: {sys_name}")
        print(f"Model: {model}")
        print(f"Chipset: {chipset_name}")
        print(f"BIOS version: {bios_version}")


    cpu_count = engine.get_count("Cpu")
    print(f"\n[CPU] Found {cpu_count} processor(s)")
    for i in range(cpu_count):
        model = engine.get_str("GetCpuModel", i)
        cores = engine.get_val("GetCpuCoreCount", ctypes.c_ulong, i)
        threads = engine.get_val("GetCpuThreadCount", ctypes.c_ulong, i)
        print(f"  > #{i}: {model} ({cores}C/{threads}T)")

    ram_count = engine.get_count("Ram")
    print(f"\n[RAM] Found {ram_count} stick(s)")
    for i in range(ram_count):
        make = engine.get_str("GetRamMake", i)
        type = engine.get_str("GetRamType",i)
        cap_mb = engine.get_val("GetRamCapacityMb", ctypes.c_uint64, i)
        speed = engine.get_val("GetRamSpeedMts", ctypes.c_ulong, i)
        print(f"  > #{i}: {make} | {type} | {cap_mb} MB | {speed} MT/s")

    gpu_count = engine.get_count("Gpu")
    print(f"\n[GPU] Found {gpu_count} graphics adapter(s)")
    for i in range(gpu_count):
        model = engine.get_str("GetGpuModel", i)
        vram = engine.get_val("GetGpuVRamSizeByte", ctypes.c_uint64, i)
        print(f"  > #{i}: {model} | VRAM: {vram / (1024**3):.2f} GB")

    mon_count = engine.get_count("Monitor")
    print(f"\n[Monitor] Found {mon_count} display(s)")
    USE_WMI = True
    for i in range(mon_count):
        if USE_WMI == False :
            vendor_name = engine.get_str("GetMonitorVendorName",i)
            model = engine.get_str("GetMonitorModel", i)
            print(f"  > #{i}: {vendor_name} {model}")
        elif USE_WMI == True:
            vendor_name = engine.get_str("GetMonitorVendorNameWmi",i)
            model = engine.get_str("GetMonitorModelWmi", i)
            vendor_id = engine.get_str("GetMonitorVendorIdWmi",i)
            product_id = engine.get_str("GetMonitorProductIdWmi",i)
            year = engine.get_val("GetMonitorYearWmi", ctypes.c_ulong, i)
            print(f"  > #{i}: {vendor_name} {model} [{vendor_id}{product_id}] ({year})")

    disk_count = engine.get_count("Disk")
    print(f"\n[Storage] Found {disk_count} storage device(s)")
    for i in range(disk_count):
        model = engine.get_str("GetDiskModel", i)
        #sn = engine.get_str("GetDiskSerialNumber", i)
        capacity_byte = engine.get_val("GetDiskTotalSizeByte", ctypes.c_uint64, i)
        size_gb = capacity_byte // (1024 * 1024 * 1024)
        print(f"  > #{i}: {model} | {size_gb} GB")

    nic_count = engine.get_count("Nic")
    print(f"\n[NIC] Found {nic_count} network interface(s)")
    for i in range(nic_count):
        model = engine.get_str("GetNicModel", i)
        print(f"  > #{i}: {model}")
    
    audio_count = engine.get_count("Audio")
    print(f"\n[Audio] Found {audio_count} audio device(s)")
    for i in range(audio_count):
        model = engine.get_str("GetAudioModel", i)
        print(f"  > #{i}: {model}")

    end_time = time.perf_counter()
    duration = (end_time - start_time) * 1000 # Convert to milliseconds
    print("\n" + "=" * 50)
    print(f"Detection completed in {duration:.2f} ms")
    print("=" * 50)


if __name__ == "__main__":
    run_hardware_demo()