#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PokaGameMode.generated.h"

UENUM(BlueprintType)
enum class EPokaGameState : uint8
{
    Playing,
    GameOver,
    GameCleared
};

UCLASS()
class MAIN_POKA_PORJECT_API APokaGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    APokaGameMode();

    // --- [게임 상태 변수] ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Logic")
    int32 TotalFriendsInMap; // 맵에 있는 전체 친구 수

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Logic")
    int32 FollowingFriendsCount; // 현재 플레이어를 따라오는 친구 수

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Logic")
    int32 CapturedFriendsCount; // 현재 감옥에 갇힌 친구 수

    // --- [핵심 게임 로직 함수] ---

    // 친구를 발견해서 E키로 따라오게 했을 때 호출
    UFUNCTION(BlueprintCallable, Category = "Game Logic")
    void OnFriendFollowed();

    // 귀신이 친구를 잡아갔을 때 호출
    UFUNCTION(BlueprintCallable, Category = "Game Logic")
    void OnFriendCaptured();

    // 감옥에 가서 친구를 다시 구출했을 때 호출
    UFUNCTION(BlueprintCallable, Category = "Game Logic")
    void OnFriendRescuedFromPrison();

    // 탈출구에 도달했을 때 (친구 유무에 따라 엔딩이 달라질 수 있음)
    UFUNCTION(BlueprintCallable, Category = "Game Logic")
    void AttemptEscape();

    // 주인공이 잡혔을 때
    void PlayerDied();

    // --- [UI 이벤트] ---
    UFUNCTION(BlueprintImplementableEvent, Category = "Game Logic")
    void UpdateFriendStatusUI(); // 친구 숫자가 바뀔 때마다 UI 갱신 신호
};