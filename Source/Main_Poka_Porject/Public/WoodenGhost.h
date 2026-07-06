#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WoodenGhost.generated.h"

UENUM(BlueprintType)
enum class EGhostState : uint8
{
    Wandering,
    Chasing,
    Attacking // [추가됨] 공격 상태
};

UCLASS()
class MAIN_POKA_PORJECT_API AWoodenGhost : public ACharacter
{
    GENERATED_BODY()

public:
    AWoodenGhost();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Settings")
    float WanderSpeed = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Settings")
    float ChaseSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Settings")
    float DetectRadius = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Settings")
    float LookAtThreshold = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Settings")
    float SightDistance = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Settings")
    float GhostVisionFOV = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Settings")
    float LoseAggroDistance = 2000.0f;

    // ==========================================
    // [추가됨] 공격 관련 세팅
    // ==========================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Attack")
    float AttackDamage = 25.0f; // 공격력 (4방이면 100)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Attack")
    float AttackRange = 120.0f; // 공격 사거리 (이만큼 가까워지면 때림)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Attack")
    float AttackCooldown = 2.0f; // 공격 후 다음 공격까지 걸리는 시간(초)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Animation")
    class UAnimSequence* CrawlAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Animation")
    class UAnimSequence* ChaseAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost Animation")
    class UAnimSequence* AttackAnim; // [추가됨] 공격 애니메이션

    EGhostState CurrentState;

private:
    bool IsPlayerLookingAtMe();
    bool CanGhostSeePlayer();

    class UAnimSequence* CurrentPlayingAnim;
    void PlayGhostAnimation(class UAnimSequence* NewAnim, bool bLoop = true);

    // [추가됨] 공격 타이머 관련
    bool bCanAttack;
    FTimerHandle AttackTimerHandle;
    void ResetAttack();
};