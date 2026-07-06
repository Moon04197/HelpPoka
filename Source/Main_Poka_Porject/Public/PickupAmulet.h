#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h" // 상호작용 인터페이스 필수 포함
#include "PickupAmulet.generated.h" // 👈 반드시 가장 마지막 #include 여야 합니다!

UCLASS()
class MAIN_POKA_PORJECT_API APickupAmulet : public AActor, public IInteractable
{
    GENERATED_BODY()

public:
    APickupAmulet();

    // 부적 3D 모델을 담을 메쉬 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* AmuletMesh;

    // =========================================================================
    // IInteractable 인터페이스 오버라이드 함수 3개
    // =========================================================================
    virtual void Interact(class APokaPlayer* PlayerCharacter) override;
    virtual FText GetInteractText() override;
    virtual void Highlight(bool bHighlight) override;
}; 