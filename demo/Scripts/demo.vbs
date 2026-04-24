' CHWEngine SDK - VBScript Wrapper for ExportReportToFile
Set objShell = CreateObject("WScript.Shell")

' Default file name is HardwareReport.txt
' To specify output file name, use "rundll32 CHWEngine.dll,ExportReportToFile <filename>"
strCommand = "rundll32 CHWEngine.dll,ExportReportToFile"

objShell.Run strCommand, 1, True

MsgBox "Hardware detection completed.", 64, "CHWEngine"