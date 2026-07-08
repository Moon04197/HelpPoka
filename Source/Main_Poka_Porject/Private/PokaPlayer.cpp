#include "PokaPlayer.h"
#include "GameFramework/PlayerController.h" // [필수 추가!] APlayerController 인식용 헤더
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimMontage.h" 
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h" // OpenLevel 및 DisableInput 기능용 필수 헤더
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h" // GEngine 디버그 메시지 인식용
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Engine/Texture2D.h"
#include "PickupBattery.h"
#include "PickupAmulet.h"

APokaPlayer::APokaPlayer()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. 카메라 설정
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // 2. 이동 및 앉기 설정
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;

    // 3. 손전등 컴포넌트 세팅
    FlashlightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlashlightMesh"));
    FlashlightMesh->SetupAttachment(GetMesh(), TEXT("FlashlightSocket"));
    FlashlightMesh->SetVisibility(false);

    FlashlightSpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("FlashlightSpotLight"));
    FlashlightSpotLight->SetupAttachment(FlashlightMesh);
    FlashlightSpotLight->SetIntensity(0.0f);

    // 4. 배터리 초기화
    CurrentBattery = MaxBattery;
}

void APokaPlayer::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    if (FlashlightSpotLight)
    {
        FlashlightSpotLight->SetIntensity(0.0f);
        bIsLightOn = false;
    }

    GetWorldTimerManager().SetTimer(FootstepTimerHandle, this, &APokaPlayer::ReportFootstepNoise, 0.4f, true);
}

void APokaPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsDead)
    {
        CheckForInteractables();
    }

    // 상하 각도 계산
    FRotator AimRotation = GetBaseAimRotation();
    FRotator ActorRotation = GetActorRotation();
    FRotator DeltaRotation = AimRotation - ActorRotation;
    DeltaRotation.Normalize();
    AimPitch = DeltaRotation.Pitch;

    // 불빛 시선 고정 보정
    if (bIsLightOn && FlashlightSpotLight && FollowCamera)
    {
        FRotator CameraRot = FollowCamera->GetComponentRotation();
        FlashlightSpotLight->SetWorldRotation(FMath::RInterpTo(FlashlightSpotLight->GetComponentRotation(), CameraRot, DeltaTime, 20.0f));
    }

    // 스태미나 계산
    if (bIsSprinting && GetVelocity().Size() > 0.0f)
    {
        CurrentStamina -= StaminaDrainRate * DeltaTime;
        if (CurrentStamina <= 0.0f)
        {
            CurrentStamina = 0.0f;
            StopSprint();
        }
    }
    else
    {
        if (CurrentStamina < MaxStamina)
        {
            CurrentStamina += StaminaRecoveryRate * DeltaTime;
            if (CurrentStamina > MaxStamina)
            {
                CurrentStamina = MaxStamina;
            }
        }
    }
}

void APokaPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APokaPlayer::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APokaPlayer::Look);
        EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APokaPlayer::StartSprint);
        EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APokaPlayer::StopSprint);
        EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &APokaPlayer::StartCrouch);
        EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APokaPlayer::StopCrouch);

        EnhancedInputComponent->BindAction(EquipFlashlightAction, ETriggerEvent::Started, this, &APokaPlayer::EquipFlashlight);
        EnhancedInputComponent->BindAction(ToggleLightAction, ETriggerEvent::Started, this, &APokaPlayer::ToggleLight);

        //  상호작용 및 퀵슬롯 아이템 제어 바인딩
        EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &APokaPlayer::TryInteract);
        EnhancedInputComponent->BindAction(SwitchItemAction, ETriggerEvent::Started, this, &APokaPlayer::SwitchItem);
        EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Started, this, &APokaPlayer::UseCurrentItem);
    }
}

void APokaPlayer::Move(const FInputActionValue& Value)
{
    // 입력받은 조이스틱/키보드의 이동 방향 값을 가져옴 ㅇㅇ
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        //  플레이어 컨트롤러를 가져와서 키보드를 누르고 있는지 검사
        if (APlayerController* PC = Cast<APlayerController>(Controller))
        {
            // W, A, S, D 키 중 단 하나라도 꾹 눌려 있는 상태인지 확인합니다.
            if (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::A) ||
                PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::D))
            {
                bIsUsingGamepad = false; // 키보드 모드 확정!
            }
            // WASD는 안 눌렸는데 이동 입력값(MovementVector)이 0.1 이상 들어오면 게임패드입니다.
            else if (MovementVector.Size() > 0.1f)
            {
                bIsUsingGamepad = true; // 게임패드 모드 확정!
            }
        }

        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(ForwardDirection, MovementVector.Y);

        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void APokaPlayer::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void APokaPlayer::StartSprint()
{
    UnCrouch();
    if (CurrentStamina > 0.0f)
    {
        bIsSprinting = true;
        GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
    }
}

void APokaPlayer::StopSprint()
{
    bIsSprinting = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APokaPlayer::StartCrouch()
{
    Crouch();
}

void APokaPlayer::StopCrouch()
{
    UnCrouch();
}

// ==========================================
// 귀신에게 잡혔을 때 처형 연출 함수
// ==========================================
void APokaPlayer::OnCaughtByGhost(AActor* GhostActor)
{
    if (bIsDead) return;
    bIsDead = true;

    // 1. 조작 차단 및 이동 완전 동결
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        DisableInput(PC);
    }

    // 👇 [철통 방어 1] DisableMovement 보다 훨씬 확실하게 캐릭터의 이동 물리를 완전 동결시킵니다!
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(MOVE_None);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    if (bIsLightOn) ToggleLight();
    GetWorldTimerManager().ClearTimer(FootstepTimerHandle);

    // 2. 귀신 쪽으로 강제 회전
    if (GhostActor && GetController())
    {
        FVector LookDir = GhostActor->GetActorLocation() - GetActorLocation();
        LookDir.Z = 0.0f;
        FRotator TargetRot = LookDir.Rotation();

        SetActorRotation(TargetRot);
        GetController()->SetControlRotation(TargetRot);
    }

    // =========================================================================
    // 👇 [철통 방어 2] 프레임 드랍(0.05초) 타이머 없이, 즉시 기존 모션을 죽이고 강제 재생!
    // =========================================================================
    float DeathDuration = 3.0f;
    if (DeathMontage)
    {
        DeathDuration = DeathMontage->GetPlayLength();

        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            // 손전등 장착 등 혹시나 재생 중이던 다른 몽타주를 0초 만에 강제 종료!
            AnimInstance->StopAllMontages(0.0f);

            // 사망 몽타주를 최우선 순위로 즉시 강제 재생!
            AnimInstance->Montage_Play(DeathMontage, 1.0f);
        }
    }

    // 4. 시네마틱 암전(Fade Out) 및 리셋 타임라인
    float LieOnGroundDelay = 2.0f;
    float FadeDuration = 2.0f;

    FTimerHandle FadeTimerHandle;
    GetWorldTimerManager().SetTimer(FadeTimerHandle, [this, FadeDuration]()
        {
            if (APlayerController* PC = Cast<APlayerController>(GetController()))
            {
                if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
                {
                    CameraManager->StartCameraFade(0.0f, 1.0f, FadeDuration, FLinearColor::Black, true, true);
                }
            }
        }, DeathDuration + LieOnGroundDelay, false);

    float TotalTimeToRespawn = DeathDuration + LieOnGroundDelay + FadeDuration + 1.0f;
    GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &APokaPlayer::Respawn, TotalTimeToRespawn, false);
}

// ==========================================
// 시작점으로 다시 로딩
// ==========================================
void APokaPlayer::Respawn()
{
    UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
}

void APokaPlayer::ReportFootstepNoise()
{
    if (GetVelocity().Size() <= 10.0f || GetCharacterMovement()->IsFalling()) return;

    float CurrentLoudness = WalkNoiseLoudness;
    if (GetCharacterMovement()->IsCrouching())
    {
        CurrentLoudness = CrouchNoiseLoudness;
    }
    else if (GetCharacterMovement()->MaxWalkSpeed == SprintSpeed)
    {
        CurrentLoudness = SprintNoiseLoudness;
    }

    if (CurrentLoudness > 0.0f)
    {
        MakeNoise(CurrentLoudness, this, GetActorLocation());
    }
}

void APokaPlayer::EquipFlashlight()
{
    if (!bHasFlashlight) return;
    bIsFlashlightEquipped = !bIsFlashlightEquipped;

    if (bIsFlashlightEquipped)
    {
        if (EquipFlashlightMontage) PlayAnimMontage(EquipFlashlightMontage);
        FlashlightMesh->SetVisibility(true);
    }
    else
    {
        if (UnequipFlashlightMontage) PlayAnimMontage(UnequipFlashlightMontage);
        FlashlightMesh->SetVisibility(false);
        bIsLightOn = false;
        FlashlightSpotLight->SetIntensity(0.0f);
        GetWorldTimerManager().ClearTimer(BatteryTimerHandle);
    }
}

void APokaPlayer::ToggleLight()
{
    if (!bIsFlashlightEquipped) return;
    bIsLightOn = !bIsLightOn;

    if (bIsLightOn && CurrentBattery > 0.0f)
    {
        FlashlightSpotLight->SetIntensity(5000.0f);
        GetWorldTimerManager().SetTimer(BatteryTimerHandle, this, &APokaPlayer::DrainBattery, 1.0f, true);
    }
    else
    {
        bIsLightOn = false;
        FlashlightSpotLight->SetIntensity(0.0f);
        GetWorldTimerManager().ClearTimer(BatteryTimerHandle);
    }
}

void APokaPlayer::DrainBattery()
{
    CurrentBattery -= BatteryDrainRate;
    if (CurrentBattery <= 0.0f)
    {
        CurrentBattery = 0.0f;
        bIsLightOn = false;
        FlashlightSpotLight->SetIntensity(0.0f);
        GetWorldTimerManager().ClearTimer(BatteryTimerHandle);
    }
}

// 퀵슬롯 아이템 교체 (피의 거짓 벨트 교체 방식)
void APokaPlayer::SwitchItem()
{
    if (bIsDead) return;

    if (CurrentSelectedItem == EItemType::Battery)
    {
        CurrentSelectedItem = EItemType::Amulet;
    }
    else
    {
        CurrentSelectedItem = EItemType::Battery;
    }

    UpdateQuickSlotUI();
}

// 현재 선택된 아이템 사용
void APokaPlayer::UseCurrentItem()
{
    if (bIsDead) return;

    if (CurrentSelectedItem == EItemType::Battery && BatteryCount > 0)
    {
        BatteryCount--;
        CurrentBattery = MaxBattery;
        UpdateQuickSlotUI();
    }
    else if (CurrentSelectedItem == EItemType::Amulet && AmuletCount > 0)
    {
        AmuletCount--;
        bIsInvisibleToGhost = true;
        GetWorldTimerManager().SetTimer(AmuletTimerHandle, this, &APokaPlayer::DeactivateAmulet, 60.0f, false);
        UpdateQuickSlotUI();
    }
}

void APokaPlayer::UpdateQuickSlotUI()
{
    if (!QuickSlotWidget) return;

    struct FSlotArgs
    {
        UTexture2D* Icon = nullptr;
        int32 Count = 0;
    };
    FSlotArgs Args;

    if (CurrentSelectedItem == EItemType::Battery)
    {
        Args.Icon = Icon_Battery;
        Args.Count = BatteryCount;
    }
    else if (CurrentSelectedItem == EItemType::Amulet)
    {
        Args.Icon = Icon_Amulet;
        Args.Count = AmuletCount;
    }

    if (UFunction* Func = QuickSlotWidget->FindFunction(FName("UpdateSlot")))
    {
        QuickSlotWidget->ProcessEvent(Func, &Args);
    }
}

void APokaPlayer::DeactivateAmulet()
{
    bIsInvisibleToGhost = false;
}


// 카메라 정면으로 레이저를 쏴서 상호작용 물체 감지
void APokaPlayer::CheckForInteractables()
{
    if (!FollowCamera) return;

    // 1. 카메라 위치에서 앞쪽으로 뻗어나갈 레이저의 시작점과 끝점(사거리) 계산
    FVector Start = FollowCamera->GetComponentLocation();
    FVector End = Start + (FollowCamera->GetForwardVector() * InteractDistance);

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // 내 캐릭터 몸통은 레이저에 안 맞도록 무시!

    // ⭐ [핵심 1] 바늘 레이저 대신, 반경 80.0f 크기의 거대한 공 모양 레이저를 쏩니다!
    float DetectRadius = 80.0f;
    bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult, Start, End, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(DetectRadius), Params
    );

    // 공 모양 레이저에 맞은 물체가 상호작용 가능한 아이템(IInteractable)인지 확인
    IInteractable* NewInteractable = nullptr;
    if (bHit && HitResult.GetActor())
    {
        NewInteractable = Cast<IInteractable>(HitResult.GetActor());
    }

    // ⭐ [핵심 2] 이번 프레임에 바라보는 대상이 아까랑 달라졌을 때만 UI를 조작합니다!
    if (CurrentInteractable != NewInteractable)
    {
        // 1) 전에 보던 아이템에서 시선이 벗어났다면? -> 켜져 있던 UI 위젯 안 보이게 숨기기!
        if (CurrentInteractable && InteractPromptWidget)
        {
            InteractPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
        }

        // 2) 새로운 아이템을 바라보게 되었다면? -> 숨겨둔 UI 위젯 화면에 켜기!
        if (NewInteractable && InteractPromptWidget)
        {
            InteractPromptWidget->SetVisibility(ESlateVisibility::Visible);

            // C++에서 위젯 블루프린트의 'UpdatePrompt' 이벤트를 실행하기 위한 데이터 포장지 만들기
            struct FPromptArgs
            {
                FText InItemName;
                bool bIsGamepad;  
            };
            FPromptArgs Args;
            Args.InItemName = NewInteractable->GetInteractText(); // 아이템 이름 포장
            Args.bIsGamepad = bIsUsingGamepad;                    // 패드 사용 여부 포장

            // 위젯 안에서 "UpdatePrompt"라는 이름의 이벤트를 찾아서 데이터를 전달하며 실행!
            if (UFunction* Func = InteractPromptWidget->FindFunction(FName("UpdatePrompt")))
            {
                InteractPromptWidget->ProcessEvent(Func, &Args);
            }
        }

        // 현재 바라보고 있는 아이템 정보를 최신으로 갱신
        CurrentInteractable = NewInteractable;
    }
}

void APokaPlayer::TryInteract()
{
    if (bIsDead || !CurrentInteractable) return;

    AActor* TargetActor = Cast<AActor>(CurrentInteractable);
    if (!TargetActor) return;

    UTexture2D* AcquiredIcon = nullptr;
    FText AcquiredName = FText::GetEmpty();

    if (Cast<APickupBattery>(TargetActor))
    {
        AcquiredIcon = Icon_Battery;
        AcquiredName = FText::FromString(TEXT("배터리"));
    }
    else if (Cast<APickupAmulet>(TargetActor))
    {
        AcquiredIcon = Icon_Amulet;
        AcquiredName = FText::FromString(TEXT("신비로운 부적"));
    }

    CurrentInteractable->Interact(this);
    CurrentInteractable = nullptr;

    if (InteractPromptWidget)
    {
        InteractPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (ItemAcquiredWidget && AcquiredIcon)
    {
        struct FAcquiredArgs
        {
            UTexture2D* ItemIcon = nullptr;
            FText ItemName = FText::GetEmpty();
        };
        FAcquiredArgs Args;
        Args.ItemIcon = AcquiredIcon;
        Args.ItemName = AcquiredName;

        if (UFunction* Func = ItemAcquiredWidget->FindFunction(FName("ShowAcquired")))
        {
            ItemAcquiredWidget->ProcessEvent(Func, &Args);
        }
    }

    UpdateQuickSlotUI();
}