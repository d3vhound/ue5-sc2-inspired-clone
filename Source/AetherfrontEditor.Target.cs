using UnrealBuildTool;
using System.Collections.Generic;

public class AetherfrontEditorTarget : TargetRules
{
    public AetherfrontEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Aetherfront");
    }
}

