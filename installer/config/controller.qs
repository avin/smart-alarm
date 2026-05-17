function isWindows()
{
    return systemInfo.kernelType === "winnt" || systemInfo.productType === "windows";
}

function isMacOS()
{
    return systemInfo.kernelType === "darwin" || systemInfo.productType === "osx" || systemInfo.productType === "macos";
}

function Controller()
{
    installer.setValue("ProductUUID", "{6a1ac59f-caf4-4ed6-88e7-82eaf6d92033}");
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
    installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);

    if (isWindows()) {
        installer.setValue("TargetDir", "@HomeDir@/AppData/Local/Programs/SmartAlarm");
    } else if (isMacOS()) {
        installer.setValue("TargetDir", "@HomeDir@/Applications/SmartAlarm");
    } else {
        installer.setValue("TargetDir", "@HomeDir@/.local/opt/SmartAlarm");
    }
}
