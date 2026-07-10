#include "PokaNPC.h"
#include "PokaPlayer.h"
#include "Components/SkeletalMeshComponent.h"

APokaNPC::APokaNPC()
{
    PrimaryActorTick.bCanEverTick = false;
    CurrentDialogueIndex = 0;
}

FText APokaNPC::GetInteractText()
{
    return FText::FromString(TEXT("대화하기"));
}

void APokaNPC::Highlight(bool bHighlight)
{
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetRenderCustomDepth(bHighlight);
        MeshComp->SetCustomDepthStencilValue(252);
    }
}

void APokaNPC::Interact(APokaPlayer* PlayerCharacter)
{
    if (!PlayerCharacter || DialogueLines.Num() == 0)
    {
        return;
    }

    const int32 TotalLines = DialogueLines.Num();

    if (CurrentDialogueIndex < TotalLines)
    {
        ShowDialogueUI(DialogueLines[CurrentDialogueIndex]);
        CurrentDialogueIndex++;
    }
    else if (CurrentDialogueIndex == TotalLines)
    {
        HideDialogueUI();
        CurrentDialogueIndex++;
    }
    else
    {
        const int32 RepeatPress = CurrentDialogueIndex - TotalLines - 1;
        if (RepeatPress % 2 == 0)
        {
            ShowDialogueUI(TEXT("부탁할게, 열쇠를 같이 찾아줘."));
        }
        else
        {
            HideDialogueUI();
        }
        CurrentDialogueIndex++;
    }
}
