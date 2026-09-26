#include "OrbitalBloomMode.h"
#include "OrbitalBloomHUD.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "InputCoreTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogOrbitalBloom, Log, All);

FVector AOrbitalBloomMode::ToUnreal(orbital::Vec3 V) { return FVector(V.x, V.y, V.z) * 100.0; }
orbital::Vec3 AOrbitalBloomMode::ToCore(FVector V) { return {V.X / 100.0, V.Y / 100.0, V.Z / 100.0}; }

AOrbitalBloomMode::AOrbitalBloomMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = nullptr;
    HUDClass = AOrbitalBloomHUD::StaticClass();
}

void AOrbitalBloomMode::BeginPlay()
{
    Super::BeginPlay();
    auto MakeMesh = [this](const TCHAR* Name, const TCHAR* MeshPath, const TCHAR* MaterialPath, FVector Scale)
    {
        AActor* Proxy = GetWorld()->SpawnActor<AActor>();
        UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Proxy, Name);
        Proxy->SetRootComponent(Mesh);
        Proxy->AddInstanceComponent(Mesh);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, MeshPath));
        Mesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, MaterialPath));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCastShadow(false);
        Mesh->RegisterComponent();
        Mesh->SetWorldScale3D(Scale);
        return Mesh;
    };
    PlayerMesh = MakeMesh(TEXT("PlayerProxy"), TEXT("/Engine/BasicShapes/Cone.Cone"), TEXT("/Game/MVP/Art/M_Player.M_Player"), FVector(0.6, 0.6, 1.5));
    BossMesh = MakeMesh(TEXT("BossProxy"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), TEXT("/Game/MVP/Art/M_Boss.M_Boss"), FVector(4.0));
    MakeMesh(TEXT("SkyProxy"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), TEXT("/Game/MVP/Art/M_Sky.M_Sky"), FVector(2000.0));
    Camera = GetWorld()->SpawnActor<ACameraActor>();
    Camera->GetCameraComponent()->SetFieldOfView(75.0f);
    auto& Post = Camera->GetCameraComponent()->PostProcessSettings;
    Post.bOverride_AutoExposureMethod = true;
    Post.AutoExposureMethod = AEM_Manual;
    Post.bOverride_AutoExposureBias = true;
    Post.AutoExposureBias = 0.0f;
    Post.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Post.AutoExposureApplyPhysicalCameraExposure = false;
    Post.bOverride_MotionBlurAmount = true;
    Post.MotionBlurAmount = 0.0f;
    Post.bOverride_BloomIntensity = true;
    Post.BloomIntensity = 0.25f;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->SetViewTarget(Camera);
        PC->SetInputMode(FInputModeGameOnly());
    }
    UpdateCamera(0.0f, true);
    UE_LOG(LogOrbitalBloom, Display, TEXT("Foundation ready: Core %u Hz, Follow, independent simulation."), Core.config().tickRate);
}

void AOrbitalBloomMode::ToggleTactical()
{
    const bool bEnter = Snapshot().mode == orbital::Mode::Action;
    Core.setMode(bEnter ? orbital::Mode::Tactical : orbital::Mode::Action);
    bPlacing = false;
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    PC->bShowMouseCursor = bEnter;
    if (bEnter)
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);
        const FVector Centre = ToUnreal(bBossOrbit ? Snapshot().boss.position : Snapshot().player.position);
        const FVector Offset = Camera->GetActorLocation() - Centre;
        OrbitDistance = FMath::Clamp(static_cast<float>(Offset.Size()), 600.0f, 22000.0f);
        OrbitYaw = Offset.Rotation().Yaw;
        OrbitPitch = FMath::Clamp(Offset.Rotation().Pitch, -80.0f, 80.0f);
    }
    else PC->SetInputMode(FInputModeGameOnly());
}

void AOrbitalBloomMode::UpdateInput(float DeltaSeconds)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;
    if (PC->WasInputKeyJustPressed(EKeys::Tab) || PC->WasInputKeyJustPressed(EKeys::SpaceBar)) ToggleTactical();
    orbital::Input Input;
    if (Snapshot().mode == orbital::Mode::Tactical)
    {
        if (PC->WasInputKeyJustPressed(EKeys::V))
        {
            bBossOrbit = !bBossOrbit;
            const FVector Centre = ToUnreal(bBossOrbit ? Snapshot().boss.position : Snapshot().player.position);
            const FVector Offset = Camera->GetActorLocation() - Centre;
            OrbitDistance = FMath::Clamp(static_cast<float>(Offset.Size()), 600.0f, 22000.0f);
            OrbitYaw = Offset.Rotation().Yaw;
            OrbitPitch = FMath::Clamp(Offset.Rotation().Pitch, -80.0f, 80.0f);
        }
        if (PC->WasInputKeyJustPressed(EKeys::C)) { bPlacing = !bPlacing; GhostDepth = OrbitDistance; }
        if (PC->WasInputKeyJustPressed(EKeys::F)) { Core.returnToFollow(); bPlacing = false; }
        float MouseX = 0, MouseY = 0;
        PC->GetInputMouseDelta(MouseX, MouseY);
        if (PC->IsInputKeyDown(EKeys::RightMouseButton))
        {
            OrbitYaw += MouseX * 0.3f;
            OrbitPitch = FMath::Clamp(OrbitPitch + MouseY * 0.3f, -80.0f, 80.0f);
        }
        const float Wheel = (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) ? 1.0f : 0.0f)
            - (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown) ? 1.0f : 0.0f);
        if (bPlacing) GhostDepth = FMath::Clamp(GhostDepth + Wheel * 300.0f, 100.0f, 25000.0f);
        else OrbitDistance = FMath::Clamp(OrbitDistance - Wheel * 400.0f, 600.0f, 22000.0f);
        UpdateGhost();
        if (bPlacing && (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton) || PC->WasInputKeyJustPressed(EKeys::Enter)))
        {
            if (Core.placeConverge(ToCore(Ghost))) bPlacing = false;
        }
    }
    else
    {
        Input.right = (PC->IsInputKeyDown(EKeys::D) ? 1.0 : 0.0) - (PC->IsInputKeyDown(EKeys::A) ? 1.0 : 0.0);
        Input.up = (PC->IsInputKeyDown(EKeys::W) ? 1.0 : 0.0) - (PC->IsInputKeyDown(EKeys::S) ? 1.0 : 0.0);
    }
    Core.advance(DeltaSeconds, Input);
}

void AOrbitalBloomMode::UpdateGhost()
{
    if (!bPlacing) return;
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    float X = 0, Y = 0;
    if (!PC->GetMousePosition(X, Y))
    {
        int32 W = 0, H = 0; PC->GetViewportSize(W, H); X = W * 0.5f; Y = H * 0.5f;
    }
    FVector Origin, Direction;
    if (PC->DeprojectScreenPositionToWorld(X, Y, Origin, Direction)) Ghost = Origin + Direction * GhostDepth;
}

bool AOrbitalBloomMode::GhostIsValid() const
{
    return Core.canPlaceConverge(ToCore(Ghost));
}

void AOrbitalBloomMode::UpdateCamera(float DeltaSeconds, bool bInstant)
{
    const auto& S = Snapshot();
    FVector Target, Position;
    FVector CameraUp = FVector::ZAxisVector;
    if (S.mode == orbital::Mode::Tactical)
    {
        Target = ToUnreal(bBossOrbit ? S.boss.position : S.player.position);
        Position = Target + FRotator(OrbitPitch, OrbitYaw, 0).Vector() * OrbitDistance;
    }
    else
    {
        const FVector Player = ToUnreal(S.player.position);
        const FVector Forward = ToUnreal(S.player.frame.forward) / 100.0;
        const FVector Up = ToUnreal(S.player.frame.up) / 100.0;
        CameraUp = Up;
        Position = Player - Forward * 1100.0 + Up * 350.0;
        Target = Player + Forward * 1800.0;
    }
    const FRotator Rotation = FRotationMatrix::MakeFromXZ(Target - Position, CameraUp).Rotator();
    Camera->SetActorLocation(bInstant ? Position : FMath::VInterpTo(Camera->GetActorLocation(), Position, DeltaSeconds, 12.0f));
    Camera->SetActorRotation(bInstant ? Rotation : FMath::RInterpTo(Camera->GetActorRotation(), Rotation, DeltaSeconds, 12.0f));
}

void AOrbitalBloomMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Camera) return;
    UpdateInput(DeltaSeconds);
    const auto& S = Snapshot();
    PlayerMesh->SetWorldLocation(ToUnreal(S.player.position));
    const FQuat Orientation = FRotationMatrix::MakeFromXZ(ToUnreal(S.player.frame.forward), ToUnreal(S.player.frame.up)).ToQuat();
    PlayerMesh->SetWorldRotation(Orientation * FQuat(FVector::YAxisVector, PI / 2.0));
    BossMesh->SetWorldLocation(ToUnreal(S.boss.position));
    UpdateCamera(DeltaSeconds);
    LogEvents();
}

void AOrbitalBloomMode::LogEvents()
{
    const TCHAR* Names[] = {TEXT("tactical_enter"), TEXT("tactical_exit"), TEXT("converge_placed"), TEXT("arrived_fixed"), TEXT("follow_return")};
    const auto& Events = Core.events();
    while (LoggedEvents < Events.size())
    {
        const auto& E = Events[LoggedEvents++];
        UE_LOG(LogOrbitalBloom, Display, TEXT("event=%s tick=%llu real=%.3f position=(%.2f,%.2f,%.2f)"),
            Names[static_cast<int>(E.kind)], static_cast<unsigned long long>(E.tick), E.realTime, E.position.x, E.position.y, E.position.z);
    }
}

void AOrbitalBloomMode::EndPlay(const EEndPlayReason::Type Reason)
{
    const auto& M = Core.metrics();
    UE_LOG(LogOrbitalBloom, Display, TEXT("session real=%.3f game=%.3f tactical=%.3f entries=%u converge=%u follow=%u travel_m=%.3f"),
        M.realTime, Snapshot().gameTime, M.tacticalTime, M.tacticalEntries, M.convergePlacements, M.followReturns, M.travelDistance);
    Super::EndPlay(Reason);
}
