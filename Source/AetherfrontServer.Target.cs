using UnrealBuildTool;
using System.Collections.Generic;

public class AetherfrontServerTarget : TargetRules
{
    public AetherfrontServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Aetherfront");
    }
}

