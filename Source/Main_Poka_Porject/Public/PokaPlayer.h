#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h" //  TOptional 관련 엔진 내부 빌드 에러 방지
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Interactable.h" //  상호작용 인터페이스 포함
#include "PokaPlayer.generated.h" // 반드시 가장 마지막 #include 여야함

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UAnimMontage;
class UStaticMeshComponent;
class USpotLightComponent;
class UUserWidget;
class UTexture2D;

//  피의 거짓 스타일 퀵슬롯 구분을 위한 열거형
UENUM(BlueprintType)
enum class EItemType : uint8
{
    Battery, // 배터리
    Amulet   // 부적
};

UCLASS()
class MAIN_POKA_PORJECT_API APokaPlayer : public ACharacter
{
    GENERATED_BODY()

public:
    APokaPlayer();

    // 매 프레임 실행될 Tick 함수
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // --- [입력 액션] ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputMappingContext* DefaultMappingContext = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* LookAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* SprintAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* CrouchAction = nullptr;

    // F키 (장착/해제)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* EquipFlashlightAction = nullptr;

    // 좌클릭 (불빛 켜기/끄기)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* ToggleLightAction = nullptr;

    // 상호작용 및 아이템 제어용 입력 액션 (E, Q, R키 바인딩용)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* InteractAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* SwitchItemAction = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* UseItemAction = nullptr;

    // --- [행동 실행 함수] ---
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartSprint();
    void StopSprint();

    // 앉기/일어서기 입력 함수
    void StartCrouch();
    void StopCrouch();

    void EquipFlashlight(); // F키 로직
    void ToggleLight();     // 좌클릭 로직

    // 배터리가 닳게 만드는 타이머 함수
    void DrainBattery();

    // 주기적으로 AI에게 발소리를 전파하는 함수
    void ReportFootstepNoise();

    // 사망 애니메이션 후 맵을 다시 불러오는 부활 함수
    void Respawn();

    // --- [속도 설정] ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float WalkSpeed = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeed = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float CrouchSpeed = 100.f;

public:
    // --- [컴포넌트] ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* CameraBoom = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* FollowCamera = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
    UStaticMeshComponent* FlashlightMesh = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
    USpotLightComponent* FlashlightSpotLight = nullptr;

    // --- [애니메이션 및 카메라 각도] ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* EquipFlashlightMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* UnequipFlashlightMontage = nullptr;

    // 상하 척추 구부리기를 위한 카메라 각도 변수
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    float AimPitch = 0.0f;

    // --- [상태 및 배터리 변수] ---
    UPROPERTY(BlueprintReadOnly, Category = "Flashlight")
    bool bHasFlashlight = true;

    UPROPERTY(BlueprintReadOnly, Category = "Flashlight")
    bool bIsFlashlightEquipped = false;

    UPROPERTY(BlueprintReadOnly, Category = "Flashlight")
    bool bIsLightOn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flashlight|Battery")
    float MaxBattery = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Flashlight|Battery")
    float CurrentBattery = 100.0f;

    // 1초당 닳는 배터리 양
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flashlight|Battery")
    float BatteryDrainRate = 0.5f;

    // --- [발소리 소음 설정 (AI 감지용)] ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Noise")
    float CrouchNoiseLoudness = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Noise")
    float WalkNoiseLoudness = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth|Noise")
    float SprintNoiseLoudness = 1.0f;

    FTimerHandle FootstepTimerHandle;

    // --- [스태미나 시스템 변수] ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float MaxStamina = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Stamina")
    float CurrentStamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float StaminaDrainRate = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float StaminaRecoveryRate = 15.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stamina")
    bool bIsSprinting = false;

    // --- [사망 및 처형 연출 시스템] ---
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsDead = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* DeathMontage = nullptr;

    // 귀신이 플레이어를 잡았을 때 호출할 블루프린트 호출 가능 함수
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnCaughtByGhost(AActor* GhostActor);

    // 상호작용 시스템 변수 및 함수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    float InteractDistance = 400.0f;

    IInteractable* CurrentInteractable = nullptr;

    void CheckForInteractables();
    void TryInteract();

    // 퀵슬롯 인벤토리 및 부적 은신 변수
    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    int32 BatteryCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    int32 AmuletCount = 0; 

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    EItemType CurrentSelectedItem = EItemType::Battery;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    bool bIsInvisibleToGhost = false;

    void SwitchItem();
    void UseCurrentItem();

    // UI 및 아이콘 변수 (블루프린트 Details에서 설정)
    UPROPERTY(BlueprintReadWrite, Category = "UI")
    class UUserWidget* InteractPromptWidget = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "UI")
    class UUserWidget* ItemAcquiredWidget = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "UI")
    class UUserWidget* QuickSlotWidget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Icons")
    class UTexture2D* Icon_Battery = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Icons")
    class UTexture2D* Icon_Amulet = nullptr;

    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateQuickSlotUI();

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    bool bIsUsingGamepad = false;

private:
    // 배터리 감소 및 부활 타이머
    FTimerHandle BatteryTimerHandle;
    FTimerHandle RespawnTimerHandle;

    // 부적 지속시간 타이머 및 해제 함수
    FTimerHandle AmuletTimerHandle;
    void DeactivateAmulet();
};