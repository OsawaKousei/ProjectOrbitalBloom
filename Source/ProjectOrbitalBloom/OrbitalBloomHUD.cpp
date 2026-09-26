#include "OrbitalBloomHUD.h"
#include "OrbitalBloomMode.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void AOrbitalBloomHUD::WorldLine(FVector A, FVector B, FLinearColor Color, float Thickness)
{
    FVector Eye; FRotator View;
    PlayerOwner->GetPlayerViewPoint(Eye, View);
    if (FVector::DotProduct(A - Eye, View.Vector()) <= 10 || FVector::DotProduct(B - Eye, View.Vector()) <= 10) return;
    const FVector P = Project(A), Q = Project(B);
    DrawLine(P.X, P.Y, Q.X, Q.Y, Color, Thickness);
}

void AOrbitalBloomHUD::Ring(FVector Centre, FVector Right, FVector Up, float Radius, FLinearColor Color)
{
    for (int32 I = 0; I < 64; ++I)
    {
        const float A = 2 * PI * I / 64, B = 2 * PI * (I + 1) / 64;
        WorldLine(Centre + (Right * FMath::Cos(A) + Up * FMath::Sin(A)) * Radius,
            Centre + (Right * FMath::Cos(B) + Up * FMath::Sin(B)) * Radius, Color);
    }
}

void AOrbitalBloomHUD::Marker(FVector Position, FLinearColor Color, bool bDiamond, const FString& Label)
{
    FVector Eye; FRotator View;
    PlayerOwner->GetPlayerViewPoint(Eye, View);
    if (FVector::DotProduct(Position - Eye, View.Vector()) <= 10) return;
    const FVector P = Project(Position);
    if (bDiamond)
    {
        DrawLine(P.X, P.Y - 12, P.X + 12, P.Y, Color, 1.5f);
        DrawLine(P.X + 12, P.Y, P.X, P.Y + 12, Color, 1.5f);
        DrawLine(P.X, P.Y + 12, P.X - 12, P.Y, Color, 1.5f);
        DrawLine(P.X - 12, P.Y, P.X, P.Y - 12, Color, 1.5f);
    }
    else
    {
        DrawLine(P.X - 9, P.Y, P.X + 9, P.Y, Color);
        DrawLine(P.X, P.Y - 9, P.X, P.Y + 9, Color);
    }
    DrawText(Label, Color, P.X + 18, P.Y - 8, nullptr, 0.9f);
}

void AOrbitalBloomHUD::DrawHUD()
{
    Super::DrawHUD();
    const AOrbitalBloomMode* Mode = GetWorld()->GetAuthGameMode<AOrbitalBloomMode>();
    if (!Mode || !Canvas) return;
    const auto& S = Mode->Snapshot();
    const bool Tactical = S.mode == orbital::Mode::Tactical;
    const FLinearColor Ink(0.035f, 0.13f, 0.23f, 1);
    const FLinearColor Cyan(0.04f, 0.60f, 0.80f, 0.9f);
    const FLinearColor Fine(0.05f, 0.45f, 0.65f, 0.4f);
    const FLinearColor Warning(0.9f, 0.08f, 0.12f, 1);
    const TCHAR* AnchorNames[] = {TEXT("FOLLOW"), TEXT("CONVERGE"), TEXT("FIXED")};
    const FVector Player = Mode->ToUnreal(S.player.position);
    const FVector Boss = Mode->ToUnreal(S.boss.position);
    const FVector Right = Mode->ToUnreal(S.player.frame.right) / 100;
    const FVector Up = Mode->ToUnreal(S.player.frame.up) / 100;
    DrawText(TEXT("O R B I T A L   B L O O M"), Ink, 36, 28, nullptr, 1.0f);
    DrawText(Tactical ? TEXT("TACTICAL  /  CRYSTALLIZE") : TEXT("ACTION  /  FLOW"), Ink, 36, 58, nullptr, 1.3f);
    DrawLine(36, 88, 310, 88, Cyan, 1.5f);
    DrawText(FString::Printf(TEXT("%s    /    %.1f m"), AnchorNames[static_cast<int>(S.anchor.kind)], FVector::Dist(Player, Boss) / 100), Ink, 36, 102);
    DrawText(FString::Printf(TEXT("SIM  %06.2f s"), S.gameTime), Ink, Canvas->SizeX - 180, 36);
    DrawText(TEXT("MOVEMENT STUDY  /  01"), Ink, Canvas->SizeX - 245, 62, nullptr, 0.8f);
    DrawText(Tactical ? TEXT("RMB drag  Orbit    V  Player / Boss    Wheel  Zoom    C  Place anchor    F  Follow    Tab / Space  Resume")
        : TEXT("W A S D  Fly in current plane    Tab / Space  Tactical"), Ink, 36, Canvas->SizeY - 42, nullptr, 0.95f);

    // A sparse boss frame supplies scale and depth without decorative scenery.
    Ring(Boss, FVector::YAxisVector, FVector::ZAxisVector, 500, Fine);
    Ring(Boss, FVector::XAxisVector, FVector::YAxisVector, 500, Fine);
    if (!Tactical) return;
    DrawText(Mode->IsBossOrbit() ? TEXT("OBSERVE  /  BOSS ORBIT") : TEXT("OBSERVE  /  PLAYER ORBIT"), Ink, 36, 132);
    Ring(Player, Right, Up, 700, Cyan);
    Ring(Player, Right, Up, 350, Fine);
    WorldLine(Player - Right * 850, Player + Right * 850, Cyan);
    WorldLine(Player - Up * 850, Player + Up * 850, Cyan);
    for (int32 I = -2; I <= 2; ++I)
    {
        WorldLine(Player + Right * (I * 200) - Up * 500, Player + Right * (I * 200) + Up * 500, Fine);
        WorldLine(Player + Up * (I * 200) - Right * 500, Player + Up * (I * 200) + Right * 500, Fine);
    }
    Marker(Player, Cyan, false, TEXT("PLAYER"));
    Marker(Boss, Ink, false, TEXT("BOSS"));
    if (S.anchor.kind != orbital::Anchor::Follow)
    {
        const FVector Anchor = Mode->ToUnreal(S.anchor.destination);
        WorldLine(Player, Anchor, Fine);
        Marker(Anchor, Cyan, S.anchor.kind == orbital::Anchor::Converge, AnchorNames[static_cast<int>(S.anchor.kind)]);
    }
    if (Mode->IsPlacing())
    {
        const FVector Ghost = Mode->GhostPosition();
        const bool Valid = Mode->GhostIsValid();
        const FLinearColor Color = Valid ? Cyan : Warning;
        WorldLine(Player, Ghost, Color);
        const orbital::Frame Arrival = orbital::turnTowards(S.player.frame, Mode->ToCore(Ghost) - S.player.position, PI);
        Ring(Ghost, Mode->ToUnreal(Arrival.right) / 100, Mode->ToUnreal(Arrival.up) / 100, 400, Fine);
        Marker(Ghost, Color, true, FString::Printf(TEXT("%s  %.1f m"), Valid ? TEXT("CONVERGE") : TEXT("OUT OF RANGE"), FVector::Dist(Player, Ghost) / 100));
        DrawText(TEXT("PLACE  /  Cursor direction + wheel depth    Click / Enter  Confirm    C  Cancel"), Ink, 36, Canvas->SizeY - 72);
    }
}
