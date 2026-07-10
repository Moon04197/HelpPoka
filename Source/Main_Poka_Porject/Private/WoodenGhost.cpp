#include "WoodenGhost.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimSequence.h"
#include "PokaPlayer.h"

AWoodenGhost::AWoodenGhost()
{
    PrimaryActorTick.bCanEverTick = true;
    CurrentState = EGhostState::Wandering;
    CurrentPlayingAnim = nullptr;
    bCanAttack = true;
}

void AWoodenGhost::BeginPlay()
{
    Super::BeginPlay();
    GetCharacterMovement()->MaxWalkSpeed = WanderSpeed;
}

void AWoodenGhost::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    AAIController* AIController = Cast<AAIController>(GetController());

    if (!PlayerPawn || !AIController) return;

    APokaPlayer* Poka = Cast<APokaPlayer>(PlayerPawn);

    if (Poka && Poka->bIsDead)
    {
        GetMesh()->bPauseAnims = false;
        AIController->StopMovement();
        return;
    }

    const bool bIsInvisible = (Poka && Poka->bIsInvisibleToGhost);
    const float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());

    if (!bIsInvisible && DistanceToPlayer <= AttackRange && bCanAttack)
    {
        CurrentState = EGhostState::Attacking;
        bCanAttack = false;

        AIController->StopMovement();
        GetMesh()->bPauseAnims = false;

        FVector DirToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
        DirToPlayer.Z = 0.0f;
        SetActorRotation(DirToPlayer.Rotation());

        PlayGhostAnimation(AttackAnim, false);

        if (Poka) Poka->OnCaughtByGhost(this);

        GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &AWoodenGhost::ResetAttack, AttackCooldown, false);
        return;
    }

    const bool bIsLookingAtMe = IsPlayerLookingAtMe();

    if (CurrentState == EGhostState::Wandering)
    {
        if (!bIsInvisible && (CanGhostSeePlayer() || DistanceToPlayer < DetectRadius))
        {
            CurrentState = EGhostState::Chasing;
            AIController->StopMovement();
            return;
        }

        if (AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
        {
            UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
            if (NavSys)
            {
                FNavLocation RandomLocation;
                if (NavSys->GetRandomReachablePointInRadius(GetActorLocation(), 1000.0f, RandomLocation))
                {
                    GetCharacterMovement()->MaxWalkSpeed = WanderSpeed;
                    AIController->MoveToLocation(RandomLocation.Location);
                }
            }
        }
        GetMesh()->bPauseAnims = false;
        PlayGhostAnimation(CrawlAnim, true);
    }
    else if (CurrentState == EGhostState::Chasing)
    {
        if (bIsInvisible || DistanceToPlayer > LoseAggroDistance)
        {
            CurrentState = EGhostState::Wandering;
            AIController->StopMovement();
            return;
        }

        if (bIsLookingAtMe)
        {
            GetCharacterMovement()->MaxWalkSpeed = 0.0f;
            GetMesh()->bPauseAnims = true;
            AIController->StopMovement();
        }
        else
        {
            GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
            GetMesh()->bPauseAnims = false;

            if (AIController->GetMoveStatus() != EPathFollowingStatus::Moving)
            {
                AIController->MoveToActor(PlayerPawn, 50.0f);
            }
            PlayGhostAnimation(ChaseAnim, true);
        }
    }
    else if (CurrentState == EGhostState::Attacking)
    {
        GetMesh()->bPauseAnims = false;
    }
}

void AWoodenGhost::PlayGhostAnimation(UAnimSequence* NewAnim, bool bLoop)
{
    if (NewAnim && CurrentPlayingAnim != NewAnim)
    {
        GetMesh()->PlayAnimation(NewAnim, bLoop);
        CurrentPlayingAnim = NewAnim;
    }
}

void AWoodenGhost::ResetAttack()
{
    bCanAttack = true;
    if (CurrentState == EGhostState::Attacking)
    {
        APokaPlayer* Poka = Cast<APokaPlayer>(UGameplayStatics::GetPlayerPawn(this, 0));
        CurrentState = (Poka && Poka->bIsInvisibleToGhost) ? EGhostState::Wandering : EGhostState::Chasing;
    }
}

bool AWoodenGhost::CanGhostSeePlayer()
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn) return false;

    APokaPlayer* Poka = Cast<APokaPlayer>(PlayerPawn);
    if (Poka && Poka->bIsInvisibleToGhost) return false;

    FVector GhostEyeLocation = GetActorLocation() + FVector(0.f, 0.f, 60.f);
    FVector PlayerTargetLocation = PlayerPawn->GetActorLocation();

    float Distance = FVector::Dist(GhostEyeLocation, PlayerTargetLocation);
    if (Distance > SightDistance) return false;

    FVector GhostForward = GetActorForwardVector();
    FVector DirToPlayer = (PlayerTargetLocation - GhostEyeLocation).GetSafeNormal();
    float DotResult = FVector::DotProduct(GhostForward, DirToPlayer);

    if (DotResult > GhostVisionFOV)
    {
        FHitResult HitResult;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, GhostEyeLocation, PlayerTargetLocation, ECC_Visibility, Params);
        if (bHit && HitResult.GetActor() == PlayerPawn) return true;
    }
    return false;
}

bool AWoodenGhost::IsPlayerLookingAtMe()
{
    APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!CameraManager) return false;

    FVector CameraLocation = CameraManager->GetCameraLocation();
    FVector CameraForward = CameraManager->GetCameraRotation().Vector();
    FVector GhostLocation = GetActorLocation() + FVector(0.f, 0.f, 60.f);

    FVector DirectionToGhost = (GhostLocation - CameraLocation).GetSafeNormal();
    float DotResult = FVector::DotProduct(CameraForward, DirectionToGhost);

    if (DotResult > LookAtThreshold)
    {
        FHitResult HitResult;
        FCollisionQueryParams CollisionParams;
        CollisionParams.AddIgnoredActor(UGameplayStatics::GetPlayerPawn(this, 0));

        bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, GhostLocation, ECC_Visibility, CollisionParams);
        if (bHit && HitResult.GetActor() == this) return true;
    }
    return false;
}
