function isWindows()
{
    return systemInfo.kernelType === "winnt" || systemInfo.productType === "windows";
}

function isMacOS()
{
    return systemInfo.kernelType === "darwin" || systemInfo.productType === "osx" || systemInfo.productType === "macos";
}

function isLinux()
{
    return systemInfo.kernelType === "linux";
}

function stopRunningWindowsInstance()
{
    try {
        installer.execute("taskkill", ["/F", "/IM", "SmartAlarm.exe"]);
    } catch(e) {
        console.log("Failed to terminate running SmartAlarm: " + e);
    }
}

function Component()
{
    installer.finishButtonClicked.connect(this, Component.prototype.launchApplication);
}

Component.prototype.executablePath = function()
{
    var targetDir = installer.value("TargetDir");
    if (isWindows()) {
        return targetDir + "/bin/SmartAlarm.exe";
    }
    if (isMacOS()) {
        return targetDir + "/SmartAlarm.app";
    }
    return targetDir + "/bin/SmartAlarm";
}

Component.prototype.installAgentSkill = function()
{
    try {
        var source = "@TargetDir@/share/SmartAlarm/agents/skills/smart-alarm-cli";
        var target = "@HomeDir@/.agents/skills/smart-alarm-cli";
        component.addOperation("Mkdir", "@HomeDir@/.agents");
        component.addOperation("Mkdir", "@HomeDir@/.agents/skills");
        component.addOperation("Mkdir", target);
        component.addOperation("Copy", source + "/SKILL.md", target + "/SKILL.md");
    } catch(e) {
        console.log("Failed to install Smart Alarm agent skill: " + e);
    }
}

Component.prototype.addWindowsPathOperation = function()
{
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

Component.prototype.launchApplication = function()
{
    try {
        if (installer.isInstaller() && installer.status === QInstaller.Success) {
            var targetDir = installer.value("TargetDir");
            var executablePath = installer.toNativeSeparators(this.executablePath());
            if (isMacOS()) {
                installer.executeDetached("open", [executablePath], installer.toNativeSeparators(targetDir));
            } else {
                installer.executeDetached(executablePath, [], installer.toNativeSeparators(targetDir));
            }
        }
    } catch(e) {
        console.log("Failed to launch Smart Alarm: " + e);
    }
}

Component.prototype.createOperations = function()
{
    if (isWindows()) {
        stopRunningWindowsInstance();
    }

    component.createOperations();

    if (isWindows()) {
        component.addOperation(
            "CreateShortcut",
            "@TargetDir@/bin/SmartAlarm.exe",
            "@UserStartMenuProgramsPath@/Smart Alarm.lnk",
            "workingDirectory=@TargetDir@/bin",
            "iconPath=@TargetDir@/bin/SmartAlarm.exe"
        );
        component.addOperation(
            "CreateShortcut",
            "@TargetDir@/bin/SmartAlarm.exe",
            "@UserStartMenuProgramsPath@/Startup/Smart Alarm.lnk",
            "workingDirectory=@TargetDir@/bin",
            "iconPath=@TargetDir@/bin/SmartAlarm.exe"
        );
    } else if (isLinux()) {
        component.addOperation(
            "CreateShortcut",
            "@TargetDir@/bin/SmartAlarm",
            "@HomeDir@/.local/share/applications/smartalarm.desktop",
            "workingDirectory=@TargetDir@/bin",
            "iconPath=@TargetDir@/bin/SmartAlarm",
            "Name=Smart Alarm",
            "GenericName=Alarm",
            "Terminal=false"
        );
    }

    this.installAgentSkill();
    this.addWindowsPathOperation();
}
