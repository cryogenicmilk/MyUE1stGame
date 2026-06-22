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
class UPointJumpTargetComponent;

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
	
	virtual void Landed(const FHitResult& Hit) override;
	 
	UFUNCTION(BlueprintImplementableEvent)
	void PlayDoubleJumpAnimation();

private:
	/// =================================================================
	/// 初期化
	/// =================================================================
	// 内部処理用の初期化・バインドヘルパー（外部に公開しないためprivateへ）
	void CustomSetupCharacterRotation();
	void CustomSetupCamera();
	void CustomSetupJump();
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
	/// 入力実行関数
	/// =================================================================
	void CustomLook(const FInputActionValue& Value);
	void CustomMove(const FInputActionValue& Value);
	void CustomStartDash();
	void CustomStopDash();
	//ジャンプ入口
	void CustomTryJump();
	void CustomPerformGroundJump();
	void CustomPerformAirJump();

private: // BP
	/// =================================================================
	/// カメラ設定
	/// =================================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Parameter", meta = (AllowPrivateAccess = "true"))
	float CustomCharacterYawSpeed = 750.0f; // UE側の命名とかぶらないように先頭にCustomを追加

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Parameter", meta = (AllowPrivateAccess = "true"))
	float CustomTargetArmLength = 500.0f; // UE側の命名とかぶらないように先頭にCustomを追加

	/// =================================================================
	/// 入力：コンテキストとアクション（カテゴリを階層化）
	/// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Context", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* DashAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* PointJumpAction;

	/// =================================================================
	/// プレイヤーパラーメーター
	/// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float DashSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	int CustomJumpMaxCount = 2; // UE側の命名とかぶらないように先頭にCustomを追加

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float JumpHeight = 500.0f;
};
