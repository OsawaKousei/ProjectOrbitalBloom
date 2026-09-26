#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "orbital/Simulation.h"
#include "orbital/Replay.h"
#include "OrbitalBloomMode.generated.h"

class ACameraActor;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UOrbitalBloomAudio;

UCLASS()
class PROJECTORBITALBLOOM_API AOrbitalBloomMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AOrbitalBloomMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    const orbital::RenderSnapshot& Snapshot() const { return Playback ? Playback->snapshot() : Core.snapshot(); }
    bool IsPlacing() const { return bPlacing; }
    bool IsBossOrbit() const { return bBossOrbit; }
    FVector GhostPosition() const { return Ghost; }
    bool GhostIsValid() const;
    bool ShowDanger() const { return bShowDanger; }
    bool IsReplay() const { return Playback.IsValid(); }
    bool IsReplayFinished() const { return Playback && Playback->finished(); }
    bool ReplayWide() const { return bReplayWide; }
    bool ReplaySlow() const { return Playback && Playback->slow(); }
    bool ShowTrajectory() const { return bShowTrajectory; }
    bool ShowDiagnostics() const { return bDiagnostics; }
    bool IsStress() const { return bStress; }
    float ResumeAccent() const { return ResumeRemaining / 0.3f; }
    const std::vector<orbital::Vec3>& Trajectory() const { return Core.recording().trajectory; }
    static FVector ToUnreal(orbital::Vec3 Value);
    static orbital::Vec3 ToCore(FVector Value);

private:
    void UpdateInput(float DeltaSeconds);
    void UpdateCamera(float DeltaSeconds, bool bInstant = false);
    void UpdateGhost();
    void ToggleTactical();
    void LogEvents();
    void UpdateBullets();
    void StartReplay();
    void RestartRun();
    void LogSession() const;

    orbital::Simulation Core{orbital::Config{120, 12.0, 20.0, 8.0, true}};
    TUniquePtr<orbital::Replay> Playback;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> PlayerMesh;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> BossMesh;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> BulletMesh;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BulletMaterial;
    UPROPERTY() TObjectPtr<UOrbitalBloomAudio> Audio;
    unsigned PreviousHits = 0;
    TArray<FTransform> BulletTransforms;
    bool bShowDanger = true;
    bool bReplayWide = true;
    bool bShowTrajectory = true;
    bool bDiagnostics = false;
    bool bStress = false;
    float ResumeRemaining = 0;
    double SimulationSeconds = 0;
    double SubmissionSeconds = 0;
    uint64 FrameCount = 0;
    bool bPlacing = false;
    bool bBossOrbit = false;
    float OrbitYaw = 180.0f;
    float OrbitPitch = 15.0f;
    float OrbitDistance = 2500.0f;
    float GhostDepth = 7000.0f;
    FVector Ghost = FVector::ZeroVector;
    size_t LoggedEvents = 0;
};
