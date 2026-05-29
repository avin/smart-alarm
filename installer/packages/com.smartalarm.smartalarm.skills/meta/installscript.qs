Component.prototype.createOperations = function()
{
    component.createOperations();

    var source = "@TargetDir@/share/SmartAlarm/agents/skills/smart-alarm-cli";
    var target = "@HomeDir@/.agents/skills/smart-alarm-cli";
    component.addOperation("Mkdir", "@HomeDir@/.agents");
    component.addOperation("Mkdir", "@HomeDir@/.agents/skills");
    component.addOperation("Mkdir", target);
    component.addOperation("Copy", source + "/SKILL.md", target + "/SKILL.md");
}
