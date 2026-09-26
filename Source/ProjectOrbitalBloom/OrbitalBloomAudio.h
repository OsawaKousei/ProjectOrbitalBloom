#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OrbitalBloomAudio.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

// A small synthesized score and interaction tones; no external audio assets.
UCLASS()
class PROJECTORBITALBLOOM_API UOrbitalBloomAudio : public UActorComponent
{
    GENERATED_BODY()
public:
    UOrbitalBloomAudio();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    void SetTactical(bool bValue);
    void Accent(float Frequency);
private:
    UPROPERTY() TObjectPtr<UAudioComponent> Music;
    UPROPERTY() TObjectPtr<UAudioComponent> Effect;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> MusicWave;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> EffectWave;
    TArray<uint8> Loop;
    float EffectRemaining = 0;
    bool bTactical = false;
};
