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

        self._buf_size = 256
        self._buffer = ctypes.create_unicode_buffer(self._buf_size)

    def get_count(self, hw_name):
        func = getattr(self.dll, f"Get{hw_name}Count")
        return func()

    def get_str(self, func_name, index=0):
        func = getattr(self.dll, func_name)
        func.argtypes = [ctypes.c_int, wintypes.LPWSTR, ctypes.c_int]
        func(index, self._buffer, self._buf_size)
        return self._buffer.value

    def get_val(self, func_name, restype, index=0):
        func = getattr(self.dll, func_name)
        func.restype = restype
        return func(index)

def run_hardware_demo():
    engine = HWEngine()
    
    print("-" * 50)
    print("CHWEngine SDK - Python Demo")
    print("-" * 50)

    start_time = time.perf_counter()

    # --- Motherboard ---
    mb_count = engine.get_count("Board")
    print(f"\n[Motherboard] Count: {mb_count}")
    for i in range(mb_count):
        make = engine.get_str("GetBoardMake", i)
        sys_model = engine.get_str("GetBoardSystemName", i)
        board_model = engine.get_str("GetBoardModel", i)
        chipset = engine.get_str("GetBoardChipsetName", i)
        bios = engine.get_str("GetBoardBiosVersion", i)
        print(f"Board #{i}:")
        print(f"  Manufacturer: {make}")
        print(f"  System Model: {sys_model}")
        print(f"  Board Model:  {board_model}")
        print(f"  Chipset:      {chipset}")
        print(f"  BIOS:         {bios}")

    # --- CPU ---
    cpu_count = engine.get_count("Cpu")
    print(f"\n[CPU] Count: {cpu_count}")
    for i in range(cpu_count):
        model = engine.get_str("GetCpuModel", i)
        cores = engine.get_val("GetCpuCoreCount", ctypes.c_ulong, i)
        threads = engine.get_val("GetCpuThreadCount", ctypes.c_ulong, i)
        print(f"Socket #{i}: {model} | {cores}C{threads}T")

    # --- RAM ---
    ram_count = engine.get_count("Ram")
    print(f"\n[RAM] Count: {ram_count}")
    for i in range(ram_count):
        make = engine.get_str("GetRamMake", i)
        type_str = engine.get_str("GetRamType", i)
        cap_mb = engine.get_val("GetRamCapacityMb", ctypes.c_uint64, i)
        speed = engine.get_val("GetRamSpeedMts", ctypes.c_ulong, i)
        print(f"RAM #{i}: {make} | {type_str} | {cap_mb} MB | {speed} MT/s")

    # --- GPU ---
    gpu_count = engine.get_count("Gpu")
    print(f"\n[GPU] Count: {gpu_count}")
    for i in range(gpu_count):
        model = engine.get_str("GetGpuModel", i)
        sub_vendor = engine.get_str("GetGpuSubVendor", i)
        vram_byte = engine.get_val("GetGpuVRamSizeByte", ctypes.c_uint64, i)
        
        if vram_byte >= 1073741824:
            print(f"GPU #{i}: {model} ({vram_byte / 1073741824:.1f}GB / {sub_vendor})")
        else:
            print(f"GPU #{i}: {model} ({vram_byte / 1048576:.0f}MB / {sub_vendor})")

    # --- Monitor ---
    mon_count = engine.get_count("Monitor")
    print(f"\n[Monitor] Count: {mon_count}")
    for i in range(mon_count):
        vendor_name = engine.get_str("GetMonitorVendorName", i)
        model = engine.get_str("GetMonitorModel", i)
        if not model: model = "Unknown Model"
        v_id = engine.get_str("GetMonitorVendorId", i)
        p_id = engine.get_str("GetMonitorProductId", i)
        year = engine.get_val("GetMonitorYear", ctypes.c_ulong, i)
        p_w = engine.get_val("GetMonitorPhysWidth", ctypes.c_int, i)
        p_h = engine.get_val("GetMonitorPhysHeight", ctypes.c_int, i)
        diagonal = engine.get_val("GetMonitorDiagonal", ctypes.c_float, i)
        
        print(f"Monitor #{i}: {vendor_name} {model} [{v_id}{p_id}] ({year}) | {p_w} x {p_h} | {diagonal:.1f} Inch")

    # --- Storage ---
    disk_count = engine.get_count("Disk")
    print(f"\n[Storage] Count: {disk_count}")
    for i in range(disk_count):
        model = engine.get_str("GetDiskModel", i)
        capacity_byte = engine.get_val("GetDiskTotalSizeByte", ctypes.c_uint64, i)
        size_gb = capacity_byte // (1024**3)
        print(f"Disk #{i}: {model} | {size_gb} GB")

    # --- NIC ---
    nic_count = engine.get_count("Nic")
    print(f"\n[NIC] Count: {nic_count}")
    for i in range(nic_count):
        model = engine.get_str("GetNicModel", i)
        print(f"NIC #{i}: {model}")
    
    # --- Audio ---
    audio_count = engine.get_count("Audio")
    print(f"\n[Audio] Count: {audio_count}")
    for i in range(audio_count):
        model = engine.get_str("GetAudioModel", i)
        print(f"Audio #{i}: {model}")

    end_time = time.perf_counter()
    duration = (end_time - start_time) * 1000
    
    print("\n" + "-" * 50)
    print(f"Detection completed in: {duration:.2f} ms")
    print("-" * 50)

if __name__ == "__main__":
    run_hardware_demo()