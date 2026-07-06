#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h" // 인터페이스 포함
#include "PickupBattery.generated.h" // 👈 무조건 가장 마지막 #include

UCLASS()
class MAIN_POKA_PORJECT_API APickupBattery : public AActor, public IInteractable
{
    GENERATED_BODY()

public:
    APickupBattery();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* BatteryMesh;

    // 인터페이스 오버라이드 함수 3개 명시
    virtual void Interact(class APokaPlayer* PlayerCharacter) override;
    virtual FText GetInteractText() override;
    virtual void Highlight(bool bHighlight) override;
};