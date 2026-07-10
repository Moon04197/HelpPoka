#include "PickupBattery.h"
#include "PokaPlayer.h"
#include "Components/StaticMeshComponent.h" // 👈 스태틱메쉬 컴포넌트 인식용 헤더 필수 추가

APickupBattery::APickupBattery()
{
    PrimaryActorTick.bCanEverTick = false;

    BatteryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatteryMesh"));
    RootComponent = BatteryMesh;
}

FText APickupBattery::GetInteractText()
{
    return FText::FromString(TEXT("획득한다"));
}

void APickupBattery::Interact(APokaPlayer* PlayerCharacter)
{
    if (PlayerCharacter)
    {
        PlayerCharacter->BatteryCount++; // 플레이어 배터리 개수 증가
        Destroy(); // 월드에서 삭제
    }
}

// ⭐ 새로 추가된 하이라이트 구현부
void APickupBattery::Highlight(bool bHighlight)
{
    if (BatteryMesh)
    {
        BatteryMesh->SetRenderCustomDepth(bHighlight);
        BatteryMesh->SetCustomDepthStencilValue(252); // 초록색 아웃라인용 스텐실 값
    }
}