#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OrbitalBloomHUD.generated.h"

UCLASS()
class PROJECTORBITALBLOOM_API AOrbitalBloomHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    void WorldLine(FVector A, FVector B, FLinearColor Color, float Thickness = 1.0f);
    void Ring(FVector Centre, FVector Right, FVector Up, float Radius, FLinearColor Color);
    void Marker(FVector Position, FLinearColor Color, bool bDiamond, const FString& Label);
};
