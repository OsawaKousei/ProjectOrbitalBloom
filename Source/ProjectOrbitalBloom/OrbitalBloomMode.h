#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "orbital/Simulation.h"
#include "OrbitalBloomMode.generated.h"

class ACameraActor;
class UStaticMeshComponent;

UCLASS()
class PROJECTORBITALBLOOM_API AOrbitalBloomMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AOrbitalBloomMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    const orbital::RenderSnapshot& Snapshot() const { return Core.snapshot(); }
    bool IsPlacing() const { return bPlacing; }
    bool IsBossOrbit() const { return bBossOrbit; }
    FVector GhostPosition() const { return Ghost; }
    bool GhostIsValid() const;
    static FVector ToUnreal(orbital::Vec3 Value);
    static orbital::Vec3 ToCore(FVector Value);

private:
    void UpdateInput(float DeltaSeconds);
    void UpdateCamera(float DeltaSeconds, bool bInstant = false);
    void UpdateGhost();
    void ToggleTactical();
    void LogEvents();

    orbital::Simulation Core;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> PlayerMesh;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> BossMesh;
    bool bPlacing = false;
    bool bBossOrbit = false;
    float OrbitYaw = 180.0f;
    float OrbitPitch = 15.0f;
    float OrbitDistance = 2500.0f;
    float GhostDepth = 7000.0f;
    FVector Ghost = FVector::ZeroVector;
    size_t LoggedEvents = 0;
};
