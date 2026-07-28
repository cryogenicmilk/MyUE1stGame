// このヘッダを1回だけ読み込む
// 多重インクルード防止
#pragma once

// ========================================
// Include order rule
// 1. Core系ヘッダ
// 2. 親クラスヘッダ
// 3. generated.h（必ず最後）
// UnrealHeaderTool が生成するコードのため
// generated.h の後に include を書くとビルドエラーになる
// ========================================

// UE Core
#include "CoreMinimal.h"
// UE Gameplay
#include "GameFramework/Character.h"
#include "InputActionValue.h" // FInputActionValueは構造体（実体）なのでインクルードが必要
// Generated header (must be last)
// UnrealHeaderTool が自動生成するコード。
// UCLASS / UPROPERTY / UFUNCTION などの
// リフレクションシステムを動かすためのコードが入る。
#include "MyCharacter.generated.h"

// 前方宣言：ヘッダーの軽量化（コンパイル速度向上）
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputComponent;
// 作ったやつ
class UPointJumpTargetComponent;
class UPointJumpActionComponent;

// ========================================
// Player Character Class
// プレイヤー操作を管理するキャラクター
// ========================================

UCLASS()// UEのクラスであることを宣言するマクロ
class MYPROJECT_API AMyCharacter : public ACharacter
{
	// Unreal Engine のリフレクションシステム用コードを生成するマクロ
	GENERATED_BODY()

public:
	AMyCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void PlayDoubleJumpAnimation();

private:
	/// =================================================================
	/// コンポーネント生成
	/// =================================================================
	void CustomCreateCameraComponents();

	/// =================================================================
	/// キャラクター設定
	/// =================================================================
	void CustomApplyCharacterRotationSettings();
	void CustomApplyMovementSettings();
	void CustomSetupEnhancedInputMapping();

	/// =================================================================
	/// 入力バインド
	/// =================================================================
	void CustomBindLookInput(UEnhancedInputComponent* EnhancedInputComp);
	void CustomBindMoveInput(UEnhancedInputComponent* EnhancedInputComp);
	void CustomBindDashInput(UEnhancedInputComponent* EnhancedInputComp);
	void CustomBindJumpInput(UEnhancedInputComponent* EnhancedInputComp);
	void CustomBindPointJumpInput(UEnhancedInputComponent* EnhancedInputComp);

	/// =================================================================
	/// 入力処理
	/// =================================================================
	void CustomHandleLookInput(const FInputActionValue& Value);
	void CustomHandleMoveInput(const FInputActionValue& Value);
	void CustomHandleDashStarted();
	void CustomHandleDashEnded();
	void CustomHandleJumpInput();
	void CustomHandlePointJumpInput();

	/// =================================================================
	/// 通常ジャンプ
	/// =================================================================
	void CustomPerformGroundJump();
	void CustomPerformAirJump();

private: // BP
	/// =================================================================
	/// コンポーネント
	/// =================================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UPointJumpActionComponent* PointJumpActionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera = nullptr;

	/// =================================================================
	/// カメラ設定
	/// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings",meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CharacterYawSpeed = 750.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings",meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CameraArmLength = 500.0f;

	/// =================================================================
	/// Enhanced InputEnhanced Input
	/// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Context", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* DashAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* PointJumpAction = nullptr;


	/// =================================================================
	/// 移動設定・プレイヤーパラーメーター
	/// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DashSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump",
		meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxJumpCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float JumpVelocity = 500.0f;
};
