#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "../OrbitalBloomMode.h"

namespace
{
class FFlightInputCheck : public IAutomationLatentCommand
{
public:
    explicit FFlightInputCheck(FAutomationTestBase* InTest) : Test(InTest) {}
    virtual bool Update() override
    {
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        AOrbitalBloomMode* Mode = World ? World->GetAuthGameMode<AOrbitalBloomMode>() : nullptr;
        APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
        const double Now = FPlatformTime::Seconds();
        if (Start == 0) Start = Now;
        if (!Mode || !PC)
        {
            if (Now - Start < 10) return false;
            Test->AddError(TEXT("PIE did not start with OrbitalBloomMode. Open L_MovementStudy before running this test."));
            return true;
        }
        if (Now - Start > 25)
        {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Released, 0.0));
            Test->AddError(TEXT("Host input sequence timed out.")); return true;
        }
        if (Now < Next) return false;
        auto Tap = [PC](FKey Key)
        {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.0));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.0));
        };
        const auto& S = Mode->Snapshot();
        switch (Stage)
        {
        case 0: Tap(EKeys::R); break;
        case 1:
            Position = S.player.position;
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Pressed, 1.0));
            break;
        case 2:
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Released, 0.0));
            Test->TestTrue(TEXT("W input moves authoritative GameCore player"), orbital::length(S.player.position - Position) > 1.0);
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if (auto* Mesh = It->FindComponentByClass<UStaticMeshComponent>(); Mesh && Mesh->GetFName() == TEXT("PlayerProxy"))
                {
                    Test->TestTrue(TEXT("Player presentation consumes core snapshot"), FVector::Dist(Mesh->GetComponentLocation(), Mode->ToUnreal(S.player.position)) < 1);
                    FoundProxy = true;
                }
            }
            Test->TestTrue(TEXT("Player render proxy exists"), FoundProxy);
            Tap(EKeys::Tab); break;
        case 3:
            Test->TestTrue(TEXT("Tactical input reaches core"), S.mode == orbital::Mode::Tactical);
            FrozenTick = S.tick; Position = S.player.position;
            Tap(EKeys::Right); break;
        case 4:
            Test->TestTrue(TEXT("Tactical camera does not advance simulation"), S.tick == FrozenTick && orbital::length(S.player.position - Position) < 1e-8);
            {
                int32 W, H; PC->GetViewportSize(W, H); PC->SetMouseLocation(W / 2, H / 2);
            }
            Tap(EKeys::C); break;
        case 5: Tap(EKeys::MouseScrollUp); break;
        case 6:
            Test->TestTrue(TEXT("Ghost preview valid"), Mode->GhostIsValid());
            Tap(EKeys::Enter); break;
        case 7:
            Test->TestTrue(TEXT("Confirm sends converge command"), S.anchor.kind == orbital::Anchor::Converge);
            Tap(EKeys::Tab); break;
        case 8:
            if (S.anchor.kind != orbital::Anchor::Fixed) return false;
            Test->TestTrue(TEXT("Action advances core"), S.tick > FrozenTick);
            Tap(EKeys::Tab); break;
        case 9: Tap(EKeys::F); break;
        case 10:
            Test->TestTrue(TEXT("Follow return command"), S.anchor.kind == orbital::Anchor::Follow);
            Tap(EKeys::Tab);
            return true;
        }
        ++Stage;
        Next = Now + 0.7;
        return false;
    }
private:
    FAutomationTestBase* Test;
    int Stage = 0;
    double Start = 0, Next = 0;
    std::uint64_t FrozenTick = 0;
    orbital::Vec3 Position;
    bool FoundProxy = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlightHostTest, "OrbitalBloom.Host.InputAndAnchors", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlightHostTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FFlightInputCheck(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
