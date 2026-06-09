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
	// Sets default values for this character's properties
	AMyCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UFUNCTION(BlueprintImplementableEvent)
	void PlayDoubleJumpAnimation();

private:
	/** カメラコンポーネント */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** 入力：コンテキストとアクション（カテゴリを階層化） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Context", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* DashAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** 移動パラメータ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float DashSpeed = 1000.0f;

	bool bIsDashing = false;

private:

	// 内部処理用の初期化・バインドヘルパー（外部に公開しないためprivateへ）
	void SetupCharacterRotation();
	void SetupCamera();
	void SetupJump();
	void SetupEnhancedInputMapping();

	void BindLookInput(UEnhancedInputComponent* EnhancedInputComp);
	void BindMoveInput(UEnhancedInputComponent* EnhancedInputComp);
	void BindJumpInput(UEnhancedInputComponent* EnhancedInputComp);
	void BindDashInput(UEnhancedInputComponent* EnhancedInputComp);

	// 入力実行関数
	void Look(const FInputActionValue& Value);
	void Move(const FInputActionValue& Value);
	void StartJump();
	void StartDash();
	void StopDash();
};
