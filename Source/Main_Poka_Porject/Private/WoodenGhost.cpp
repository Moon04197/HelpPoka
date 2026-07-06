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
    bCanAttack = true; // 처음엔 무조건 공격 가능
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

    // 1. 플레이어가 죽었으면 귀신도 이동을 멈추고 구경함 (단, 애니메이션은 멈추지 않음!)
    if (APokaPlayer* Poka = Cast<APokaPlayer>(PlayerPawn))
    {
        if (Poka->bIsDead)
        {
            GetMesh()->bPauseAnims = false; // 👈 [안전장치 1] 플레이어가 죽어있을 땐 귀신이 절대 얼어붙지 않음!
            AIController->StopMovement();
            return;
        }
    }

    float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());

    // =========================================================================
       // 👇 사정거리 내에 들어오면 즉시 공격 시작
       // =========================================================================
    if (DistanceToPlayer <= AttackRange && bCanAttack)
    {
        CurrentState = EGhostState::Attacking;
        bCanAttack = false;

        AIController->StopMovement();
        GetMesh()->bPauseAnims = false;

        // ---------------------------------------------------------------------
        // ⭐ [여기 3줄 필수 추가!] 귀신이 플레이어가 있는 방향을 즉시 정면으로 바라보게 회전!
        // ---------------------------------------------------------------------
        FVector DirToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
        DirToPlayer.Z = 0.0f; // 바닥 위 수평으로만 회전하도록 Z축 각도 제거
        SetActorRotation(DirToPlayer.Rotation());
        // ---------------------------------------------------------------------

        PlayGhostAnimation(AttackAnim, false); // 1번만 공격 모션 재생

        if (APokaPlayer* Poka = Cast<APokaPlayer>(PlayerPawn))
        {
            Poka->OnCaughtByGhost(this);
        }

        GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &AWoodenGhost::ResetAttack, AttackCooldown, false);
        return;
    }

    bool bIsLookingAtMe = IsPlayerLookingAtMe();

    // ==========================================
    // 상태 1. 배회 상태 (Wandering)
    // ==========================================
    if (CurrentState == EGhostState::Wandering)
    {
        if (CanGhostSeePlayer() || DistanceToPlayer < DetectRadius)
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
    // ==========================================
    // 상태 2. 추격 상태 (Chasing)
    // ==========================================
    else if (CurrentState == EGhostState::Chasing)
    {
        if (DistanceToPlayer > LoseAggroDistance)
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
    // ==========================================
    // 상태 3. 공격 중 상태 (Attacking)
    // ==========================================
    else if (CurrentState == EGhostState::Attacking)
    {
        GetMesh()->bPauseAnims = false;
    }
}

void AWoodenGhost::PlayGhostAnimation(UAnimSequence* NewAnim, bool bLoop)
{
    if (NewAnim && CurrentPlayingAnim != NewAnim)
    {
        GetMesh()->PlayAnimation(NewAnim, bLoop); // 전달받은 bLoop 값에 따라 1번만 재생하거나 반복 재생!
        CurrentPlayingAnim = NewAnim;
    }
}

// [추가됨] 쿨타임이 끝나면 다시 추격 모드로 복귀
void AWoodenGhost::ResetAttack()
{
    bCanAttack = true;
    if (CurrentState == EGhostState::Attacking)
    {
        CurrentState = EGhostState::Chasing;
    }
}

bool AWoodenGhost::CanGhostSeePlayer()
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn) return false;

    // 귀신의 눈높이 (이건 귀신의 시야 시작점이니 +60 유지)
    FVector GhostEyeLocation = GetActorLocation() + FVector(0.f, 0.f, 60.f);

    // [수정된 부분] 플레이어 과녁 위치 (+60 삭제!)
    // GetActorLocation()은 항상 캡슐 중앙이므로 서있든 앉아있든 무조건 몸통을 맞춥니다.
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

        // 수정된 과녁(PlayerTargetLocation)으로 레이저 발사!
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