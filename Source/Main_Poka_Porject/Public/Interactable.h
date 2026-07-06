#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
    GENERATED_BODY()
};

class MAIN_POKA_PORJECT_API IInteractable
{
    GENERATED_BODY()

public:
    // E키 상호작용
    virtual void Interact(class APokaPlayer* PlayerCharacter) = 0;

    // 안내 텍스트
    virtual FText GetInteractText() = 0;

    // 테두리 하이라이트 (스텐실 켜고 끄기)
    virtual void Highlight(bool bHighlight) = 0;
};