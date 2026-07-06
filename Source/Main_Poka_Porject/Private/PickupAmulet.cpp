#include "PickupAmulet.h"
#include "PokaPlayer.h"
#include "Components/StaticMeshComponent.h" // 스태틱 메쉬 컴포넌트 인식용 필수 추가

APickupAmulet::APickupAmulet()
{
    PrimaryActorTick.bCanEverTick = false;

    // 부적 메쉬 컴포넌트 생성 및 루트로 지정
    AmuletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AmuletMesh"));
    RootComponent = AmuletMesh;
}

// 1. 플레이어가 카메라로 바라봤을 때 띄울 안내 글씨
FText APickupAmulet::GetInteractText()
{
    return FText::FromString(TEXT("신비로운 부적 [E 키를 눌러 획득]"));
}

// 2. E키를 눌렀을 때 실행되는 상호작용 로직
void APickupAmulet::Interact(APokaPlayer* PlayerCharacter)
{
    if (PlayerCharacter)
    {
        // 플레이어의 부적 소지 개수 1 증가
        PlayerCharacter->AmuletCount++;

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("신비로운 부적을 획득했습니다!"));
        }

        // 맵에서 부적 액터 제거 (줍기 완료)
        Destroy();
    }
}

// 3. 플레이어 시선이 닿았을 때 희미한 초록색 테두리(Custom Depth) 켜고 끄기
void APickupAmulet::Highlight(bool bHighlight)
{
    if (AmuletMesh)
    {
        AmuletMesh->SetRenderCustomDepth(bHighlight);
        AmuletMesh->SetCustomDepthStencilValue(252); // 초록색 아웃라인 스텐실 번호
    }
}