#include "PokaGameMode.h"
#include "Kismet/GameplayStatics.h"

APokaGameMode::APokaGameMode()
{
    TotalFriendsInMap = 3; // 예시: 맵에 총 3명의 친구가 있다고 가정
    FollowingFriendsCount = 0;
    CapturedFriendsCount = 0;
}

void APokaGameMode::OnFriendFollowed()
{
    FollowingFriendsCount++;
    UpdateFriendStatusUI();
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("친구를 구출했습니다! 이제 뒤를 따라옵니다."));
}

void APokaGameMode::OnFriendCaptured()
{
    if (FollowingFriendsCount > 0)
    {
        FollowingFriendsCount--;
        CapturedFriendsCount++;
        UpdateFriendStatusUI();
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("경고: 친구가 귀신에게 잡혀갔습니다! 감옥으로 끌려갑니다."));
    }
}

void APokaGameMode::OnFriendRescuedFromPrison()
{
    if (CapturedFriendsCount > 0)
    {
        CapturedFriendsCount--;
        FollowingFriendsCount++;
        UpdateFriendStatusUI();
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("감옥에서 친구를 구출했습니다!"));
    }
}

void APokaGameMode::AttemptEscape()
{
    // 친구를 한 명도 안 데리고 탈출할 경우와 모두 데리고 탈출할 경우 판정을 다르게 할 수 있습니다.
    if (FollowingFriendsCount == 0 && CapturedFriendsCount > 0)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("찝찝한 탈출: 친구들을 두고 혼자 살아남았습니다..."));
    }
    else
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("진 엔딩: 모든 친구와 함께 탈출에 성공했습니다!"));
    }

    // 게임 클리어 처리
}

void APokaGameMode::PlayerDied()
{
    // 주인공이 잡히면 바로 게임 오버
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("주인공이 잡혔습니다. 게임 오버."));
    // Restart 로직 등 추가
}