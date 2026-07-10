#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interactable.h"
#include "PokaNPC.generated.h"

UCLASS()
class MAIN_POKA_PORJECT_API APokaNPC : public ACharacter, public IInteractable
{
    GENERATED_BODY()

public:
    APokaNPC();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TArray<FString> DialogueLines;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
    int32 CurrentDialogueIndex = 0;

    virtual void Interact(class APokaPlayer* PlayerCharacter) override;
    virtual FText GetInteractText() override;
    virtual void Highlight(bool bHighlight) override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
    void ShowDialogueUI(const FString& TextToDisplay);

    UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
    void HideDialogueUI();
};
