using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Diagnostics;

class Program
{
    private const string DllPath = "CHWEngine.dll";
    private const int BufSize = 256;

    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetBoardCount();
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetBoardMake(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetBoardSystemName(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetBoardModel(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetBoardChipsetName(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetBoardBiosVersion(int index, StringBuilder buffer, int size);

    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetCpuCount();
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetCpuModel(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern uint GetCpuCoreCount(int index);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern uint GetCpuThreadCount(int index);

    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetRamCount();
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetRamMake(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetRamType(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern ulong GetRamCapacityMb(int index);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern uint GetRamSpeedMts(int index);

    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetGpuCount();
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetGpuModel(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetGpuSubVendor(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern ulong GetGpuVRamSizeByte(int index);

    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetMonitorCount();
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetMonitorVendorName(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetMonitorModel(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetMonitorVendorId(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetMonitorProductId(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern uint GetMonitorYear(int index);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetMonitorPhysWidth(int index);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetMonitorPhysHeight(int index);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern float GetMonitorDiagonal(int index);

    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetDiskCount();
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetDiskModel(int index, StringBuilder buffer, int size);
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern ulong GetDiskTotalSizeByte(int index);

    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetNicCount();
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetNicModel(int index, StringBuilder buffer, int size);

    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern int GetAudioCount();
    [DllImport(DllPath, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetAudioModel(int index, StringBuilder buffer, int size);

    static string GetStr(Action<int, StringBuilder, int> func, int index)
    {
        StringBuilder sb = new StringBuilder(BufSize);
        func(index, sb, BufSize);
        return sb.ToString();
    }

    static void Main()
    {
        Console.WriteLine(new string('-', 50));
        Console.WriteLine("CHWEngine SDK - C# Demo");
        Console.WriteLine(new string('-', 50));

        Stopwatch sw = Stopwatch.StartNew();

        int mbCount = GetBoardCount();
        Console.WriteLine($"\n[Motherboard] Count: {mbCount}");
        for (int i = 0; i < mbCount; i++)
        {
            Console.WriteLine($"Board #{i}:");
            Console.WriteLine($"  Manufacturer: {GetStr(GetBoardMake, i)}");
            Console.WriteLine($"  System Model: {GetStr(GetBoardSystemName, i)}");
            Console.WriteLine($"  Board Model:  {GetStr(GetBoardModel, i)}");
            Console.WriteLine($"  Chipset:      {GetStr(GetBoardChipsetName, i)}");
            Console.WriteLine($"  BIOS:         {GetStr(GetBoardBiosVersion, i)}");
        }

        int cpuCount = GetCpuCount();
        Console.WriteLine($"\n[CPU] Count: {cpuCount}");
        for (int i = 0; i < cpuCount; i++)
        {
            string model = GetStr(GetCpuModel, i);
            uint cores = GetCpuCoreCount(i);
            uint threads = GetCpuThreadCount(i);
            Console.WriteLine($"Socket #{i}: {model} | {cores}C{threads}T");
        }

        int ramCount = GetRamCount();
        Console.WriteLine($"\n[RAM] Count: {ramCount}");
        for (int i = 0; i < ramCount; i++)
        {
            string make = GetStr(GetRamMake, i);
            string type = GetStr(GetRamType, i);
            ulong mb = GetRamCapacityMb(i);
            uint speed = GetRamSpeedMts(i);
            Console.WriteLine($"RAM #{i}: {make} | {type} | {mb} MB | {speed} MT/s");
        }

        int gpuCount = GetGpuCount();
        Console.WriteLine($"\n[GPU] Count: {gpuCount}");
        for (int i = 0; i < gpuCount; i++)
        {
            string model = GetStr(GetGpuModel, i);
            string sub = GetStr(GetGpuSubVendor, i);
            ulong bytes = GetGpuVRamSizeByte(i);
            if (bytes >= 1073741824)
                Console.WriteLine($"GPU #{i}: {model} ({bytes / 1073741824.0:F1}GB / {sub})");
            else
                Console.WriteLine($"GPU #{i}: {model} ({bytes / 1048576.0:F0}MB / {sub})");
        }

        int monCount = GetMonitorCount();
        Console.WriteLine($"\n[Monitor] Count: {monCount}");
        for (int i = 0; i < monCount; i++)
        {
            string vendor = GetStr(GetMonitorVendorName, i);
            string model = GetStr(GetMonitorModel, i);
            if (string.IsNullOrEmpty(model)) model = "Unknown Model";
            string vid = GetStr(GetMonitorVendorId, i);
            string pid = GetStr(GetMonitorProductId, i);
            uint year = GetMonitorYear(i);
            int pw = GetMonitorPhysWidth(i);
            int ph = GetMonitorPhysHeight(i);
            float diag = GetMonitorDiagonal(i);
            Console.WriteLine($"Monitor #{i}: {vendor} {model} [{vid}{pid}] ({year}) | {pw} x {ph} | {diag:F1} Inch");
        }

        int diskCount = GetDiskCount();
        Console.WriteLine($"\n[Storage] Count: {diskCount}");
        for (int i = 0; i < diskCount; i++)
        {
            string model = GetStr(GetDiskModel, i);
            ulong bytes = GetDiskTotalSizeByte(i);
            Console.WriteLine($"Disk #{i}: {model} | {bytes / (1024 * 1024 * 1024)} GB");
        }

        int nicCount = GetNicCount();
        Console.WriteLine($"\n[NIC] Count: {nicCount}");
        for (int i = 0; i < nicCount; i++)
            Console.WriteLine($"NIC #{i}: {GetStr(GetNicModel, i)}");

        int audioCount = GetAudioCount();
        Console.WriteLine($"\n[Audio] Count: {audioCount}");
        for (int i = 0; i < audioCount; i++)
            Console.WriteLine($"Audio #{i}: {GetStr(GetAudioModel, i)}");

        sw.Stop();
        Console.WriteLine("\n" + new string('-', 50));
        Console.WriteLine($"Detection completed in: {sw.Elapsed.TotalMilliseconds:F2} ms");
        Console.WriteLine(new string('-', 50));
        Console.ReadKey();
    }
}