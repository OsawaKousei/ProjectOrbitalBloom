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
    const auto& Path = Mode->Trajectory();
    const size_t End = FMath::Min(static_cast<size_t>(S.tick), Path.size());
    const size_t Start = Mode->IsReplay() ? 0 : (End > 90 ? End - 90 : 0);
    if (!Mode->IsReplay() || Mode->ShowTrajectory())
        for (size_t I = Start + 4; I < End; I += 4)
            WorldLine(Mode->ToUnreal(Path[I - 4]), Mode->ToUnreal(Path[I]), Cyan, Mode->IsReplay() ? 1.5f : 2.0f);
    DrawRect(FLinearColor(0.72f, 0.85f, 0.95f, 0.55f), 28, 20, 340, Tactical ? 166 : 112);
    DrawText(TEXT("O R B I T A L   B L O O M"), Ink, 36, 28, nullptr, 1.0f);
    DrawText(Mode->IsReplay() ? TEXT("REPLAY  /  FLIGHT") : (Tactical ? TEXT("TACTICAL  /  CRYSTALLIZE") : TEXT("ACTION  /  FLOW")), Ink, 36, 58, nullptr, 1.3f);
    DrawLine(36, 88, 310, 88, Cyan, 1.5f);
    DrawText(FString::Printf(TEXT("%s    /    %.1f m"), AnchorNames[static_cast<int>(S.anchor.kind)], FVector::Dist(Player, Boss) / 100), Ink, 36, 102);
    DrawText(FString::Printf(TEXT("SIM  %06.2f s"), S.gameTime), Ink, Canvas->SizeX - 180, 36);
    const TCHAR* PatternNames[] = {TEXT("SHELL / FLOWER"), TEXT("HELIX / TUNNEL"), TEXT("WALL / LATTICE")};
    DrawText(PatternNames[static_cast<int>(S.pattern)], Ink, Canvas->SizeX - 255, 62, nullptr, 0.9f);
    if (Mode->ShowDiagnostics())
        DrawText(FString::Printf(TEXT("%s | %d bullets | tick %llu"), Mode->IsStress() ? TEXT("STRESS") : TEXT("DEBUG"), static_cast<int32>(S.bullets.size()), static_cast<unsigned long long>(S.tick)), Ink, 36, Canvas->SizeY - 102, nullptr, 0.9f);
    if (Mode->ResumeAccent() > 0)
    {
        FLinearColor Accent = Cyan; Accent.A *= Mode->ResumeAccent();
        DrawLine(0, 2, Canvas->SizeX, 2, Accent, 3);
        DrawLine(0, Canvas->SizeY - 2, Canvas->SizeX, Canvas->SizeY - 2, Accent, 3);
    }
    if (S.gameTime < S.invulnerableUntil)
    {
        DrawText(TEXT("CONTACT"), Warning, Canvas->SizeX * 0.5f - 35, 36);
        DrawLine(36, 90, Canvas->SizeX - 36, 90, Warning, 2);
    }
    DrawText(Tactical ? TEXT("RMB Orbit  |  V Player/Boss  |  Wheel Zoom  |  C Anchor  |  F Follow  |  H Danger  |  Tab Resume")
        : (Mode->IsReplay() ? TEXT("V Camera  |  T Trajectory  |  X 0.5x / 1x  |  Enter Replay  |  R Restart  |  Esc Exit") : TEXT("W A S D  Fly in current plane    Tab / Space  Tactical    R Restart")), Ink, 36, Canvas->SizeY - 42, nullptr, 0.95f);
    if (Mode->IsReplay())
    {
        DrawText(FString::Printf(TEXT("%s   %s   %s"), Mode->ReplayWide() ? TEXT("WIDE") : TEXT("CHASE"), Mode->ReplaySlow() ? TEXT("0.5x") : TEXT("1x"), Mode->IsReplayFinished() ? TEXT("COMPLETE") : TEXT("")), Ink, 36, 132);
    }

    // A sparse boss frame supplies scale and depth without decorative scenery.
    Ring(Boss, FVector::YAxisVector, FVector::ZAxisVector, 500, Fine);
    Ring(Boss, FVector::XAxisVector, FVector::YAxisVector, 500, Fine);
    if (!Tactical) return;
    DrawText(FString::Printf(TEXT("CONTACTS %u   /   DANGER %s"), S.hits, Mode->ShowDanger() ? TEXT("ON") : TEXT("OFF")), Ink, 36, 158, nullptr, 0.9f);
    if (Mode->ShowDanger())
    {
        for (const auto& B : S.bullets)
        {
            if (!B.dangerous) continue;
            const FVector Position = Mode->ToUnreal(B.position);
            // Keep the amber bullet itself; a separate red diamond marks advisory risk.
            Marker(Position, Warning, true, TEXT(""));
            WorldLine(Position, Position + Mode->ToUnreal(B.velocity) * 0.25, Warning);
        }
    }
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
