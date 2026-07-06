#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PokaPlayerController.generated.h"

UCLASS()
class MAIN_POKA_PORJECT_API APokaPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    // 게임이 시작될 때 한 번 실행되는 함수
    virtual void BeginPlay() override;
};