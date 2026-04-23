## CHWEngine

A Win32 C library for hardware information. Lightweight and fast. Built using Dev-C++ w/ MinGW.

## Performance

### On a typical 12th Intel Core platform:

* C: ~6ms
* Python (ctypes): ~10ms
* C# (P/Invoke): ~15ms

## Supported Components

| Component   | Field               |
| ----------- | ------------------- |
| CPU         | Model               |
|             | Cores               |
|             | Threads             |
| Motherboard | Manufacturer        |
|             | System Model        |
|             | Model               |
|             | Chipset             |
|             | BIOS                |
| RAM         | Manufacturer        |
|             | Type                |
|             | Capacity            |
|             | Speed               |
| GPU         | Vendor              |
|             | Model               |
|             | Sub-Vendor          |
|             | VRAM Size           |
| Disk        | Model               |
|             | Total Size          |
|             | Serial Number       |
| Monitor     | Vendor              |
|             | Model               |
|             | Code                |
|             | Desktop Resolution  |
|             | Physical Resolution |
|             | Diagonal Size       |
| Audio       | Model               |
| NIC         | Model               |

## Database

The ID-Translation mapping databases for chipset, graphics, memory type and monitors are stored in CSV files in the 'db/' directory and are embedded as resources into the DLL binaries. Contributions can be made by adding rows into the CSV files and submit a pull request. The DLL binaries MUST be re-compiled to apply changes to databases.

## Exported APIs

Both low-level direct probing functions and encapsulated getter functions are exported. Direct probing functions requires invoker to create a HW_REPORT structure in advance and call the functions to populate the structure, which is suitable for low-level programming using C/C++. Encapsulated functions either return numerical values directly or write wide-char encoded text data to the buffer passed in, which is suitable for most of the modern high-level languages like C# and Python. Detailed examples of the usage of the exported APIs are demonstrated in examples of supported programming languages in the SDK package.

## Supported Programming Languages

C/C++, C# and Python are officially supported as invokers. Demos of them are included in the SDK package.

## Supported Architectures

x86 (i386) and x86_64 (amd64/x64) are currently supported while x86 binaries are NOT fully tested and might be deprecated in the future. x64 is the recommended architecture for production use.

## SDK Package

Compiled DLLs of supported architectures, C/C++ header files and examples of the supported languages are included. MSVC-styled static linking libraries of supported architectures are provided as well.

## Example demo output

```
--------------------------------------------------
CHWEngine SDK - C Demo
--------------------------------------------------

[Motherboard] Count: 1
Board #0:
  Manufacturer: ASUSTeK COMPUTER INC.
  System Model: System Product Name
  Board Model:  ROG MAXIMUS Z690 HERO
  Chipset:      Intel Z690 Chipset
  BIOS:         4301

[CPU] Count: 1
Socket #0: 12th Gen Intel(R) Core(TM) i9-12900KS | 16C24T

[RAM] Count: 2
RAM #0: KingBank | DDR5 | 16384 MB | 6000 MT/s
RAM #1: KingBank | DDR5 | 16384 MB | 6000 MT/s

[GPU] Count: 1
GPU #0: NVIDIA GeForce RTX 5070 Ti (15.6GB / PALIT)

[Monitor] Count: 1
Monitor #0: Philips 27M2N5810 [PHLC32C] (2024) | 3840 x 2160 | 27.2 Inch

[Storage] Count: 2
Disk #0: KIOXIA-EXCERIA PLUS G3 SSD | 931 GB
Disk #1: WD Blue SN5000 1TB | 931 GB

[NIC] Count: 2
NIC #0: Intel(R) Ethernet Controller (3) I225-V
NIC #1: Intel(R) Wi-Fi 6E AX211 160MHz

[Audio] Count: 7
Audio #0: NVIDIA High Definition Audio
Audio #1: Shure MV5
Audio #2: WH-1000XM4 Stereo
Audio #3: NVIDIA Virtual Audio Device (Wave Extensible) (WDM)
Audio #4: WH-1000XM4 Hands-Free AG Audio
Audio #5: Realtek USB Audio
Audio #6: Sonic Studio Virtual Mixer

--------------------------------------------------
Detection completed in: 6.57 ms
--------------------------------------------------
```

