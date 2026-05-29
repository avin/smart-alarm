function isWindows()
{
    return systemInfo.kernelType === "winnt" || systemInfo.productType === "windows";
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (!isWindows()) {
        return;
    }

    var scriptPath = "@TargetDir@/share/SmartAlarm/installer/update_user_path.ps1";
    var binPath = installer.toNativeSeparators("@TargetDir@/bin");
    component.addOperation("Execute", [
        "{0}",
        "powershell.exe",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        scriptPath,
        "add",
        binPath,
        "UNDOEXECUTE",
        "powershell.exe",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        scriptPath,
        "remove",
        binPath
    ]);
}
