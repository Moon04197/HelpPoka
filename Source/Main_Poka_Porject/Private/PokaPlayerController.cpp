#include "PokaPlayerController.h"

void APokaPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 1. 마우스 커서를 화면에서 숨깁니다.
    bShowMouseCursor = false;

    // 2. 입력 모드를 '게임 전용'으로 설정하여 클릭 시 엉뚱한 창이 선택되지 않게 합니다.
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
}