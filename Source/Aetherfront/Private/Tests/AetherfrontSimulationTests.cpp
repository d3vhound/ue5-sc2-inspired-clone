#if WITH_DEV_AUTOMATION_TESTS

#include "AetherfrontBuilding.h"
#include "AetherfrontTypes.h"
#include "JsonObjectConverter.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAetherfrontEconomyCatalogTest,
    "Aetherfront.Simulation.EconomyCatalog",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAetherfrontEconomyCatalogTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const int32 RelayCost = AAetherfrontBuilding::GetAlloyCost(EAetherfrontBuildingKind::Relay);
    const int32 ExtractorCost = AAetherfrontBuilding::GetAlloyCost(EAetherfrontBuildingKind::Extractor);
    const int32 CitadelCost = AAetherfrontBuilding::GetAlloyCost(EAetherfrontBuildingKind::Citadel);

    TestTrue(TEXT("Relay cost is positive"), RelayCost > 0);
    TestTrue(TEXT("Extractor costs more than a relay"), ExtractorCost > RelayCost);
    TestTrue(TEXT("Citadel costs more than an extractor"), CitadelCost > ExtractorCost);
    TestTrue(
        TEXT("Citadel has the largest footprint"),
        AAetherfrontBuilding::GetFootprintRadius(EAetherfrontBuildingKind::Citadel)
            > AAetherfrontBuilding::GetFootprintRadius(EAetherfrontBuildingKind::Relay));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAetherfrontSaveSchemaRoundTripTest,
    "Aetherfront.Persistence.SaveSchemaRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAetherfrontSaveSchemaRoundTripTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FAetherfrontWorldSaveData Original;
    Original.WorldSeed = 42;
    Original.AuthoritativeTick = 9001;

    FAetherfrontBuildingSaveRecord Record;
    Record.EntityId = TEXT("entity-test");
    Record.OwnerId = TEXT("commander-test");
    Record.Kind = EAetherfrontBuildingKind::Relay;
    Record.Transform = FTransform(FRotator(0.0f, 35.0f, 0.0f), FVector(100.0f, 200.0f, 95.0f));
    Record.BuildProgress = 0.75f;
    Original.Buildings.Add(Record);

    FString Json;
    TestTrue(TEXT("Save schema serializes"), FJsonObjectConverter::UStructToJsonObjectString(Original, Json));

    FAetherfrontWorldSaveData Restored;
    TestTrue(TEXT("Save schema deserializes"), FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Restored, 0, 0));
    TestEqual(TEXT("World seed survives"), Restored.WorldSeed, Original.WorldSeed);
    TestEqual(TEXT("Authoritative tick survives"), Restored.AuthoritativeTick, Original.AuthoritativeTick);
    TestEqual(TEXT("Building count survives"), Restored.Buildings.Num(), 1);
    if (!Restored.Buildings.IsEmpty())
    {
        TestEqual(TEXT("Entity ID survives"), Restored.Buildings[0].EntityId, Record.EntityId);
        TestEqual(TEXT("Owner ID survives"), Restored.Buildings[0].OwnerId, Record.OwnerId);
        TestEqual(
            TEXT("Building kind survives"),
            static_cast<uint8>(Restored.Buildings[0].Kind),
            static_cast<uint8>(Record.Kind));
        TestTrue(
            TEXT("Build progress survives"),
            FMath::IsNearlyEqual(Restored.Buildings[0].BuildProgress, Record.BuildProgress));
    }
    return true;
}

#endif
