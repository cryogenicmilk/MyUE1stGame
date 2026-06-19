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

enum class EPointJumpResult : uint8
{
	Normal,
	Good,
	Perfect
};

UENUM(BlueprintType)
enum class EPointJumpState : uint8
{
	None,
	Start,
	Pulling,
	LandingSnap,// ポイントに吸着するのみ
	Landing,	// ポイント上で待機など
	Launch,
	AfterLaunch
};

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

	UFUNCTION(BlueprintCallable, Category = "PointJump|Animation")
	void CustomOnPointJumpLaunchAnimationFinished();

	UFUNCTION(BlueprintCallable, Category = "PointJump|Animation")
	void CustomOnPointJumpLandingAnimationFinished();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Actions", meta = (AllowPrivateAccess = "true"))
	UInputAction* PointJumpAction;

	/** 移動パラメータ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float DashSpeed = 1000.0f;

	bool bIsDashing = false;

	/** ポイントジャンプ調整値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Search", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpSearchRadius = 2500.0f;

	// カメラ前方との一致度。1に近いほど画面中央付近しか拾わない。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Search", meta = (AllowPrivateAccess = "true", ClampMin = "-1.0", ClampMax = "1.0"))
	float PointJumpMinViewDot = 0.55f;

	// ポイントに近づいたとみなす距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Move", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpArriveDistance = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Move", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpPullSpeed = 2800.0f;

	// start, pulling中に方向入力があったら保存する用
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PointJump|Move", meta = (AllowPrivateAccess = "true"))
	FVector PointJumpBufferedMoveDirection = FVector::ZeroVector;
	// 
	bool bHasPointJumpMoveInput = false;

	//ポイントが近い場合、pullingを飛ばしてlandingになる距離
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Move", meta = (AllowPrivateAccess = "true"))
	float PointJumpSkipPullingDistance = 200.0f;
	// pulling中にここまで近づいたらlanding着地吸着へ行く
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Move", meta = (AllowPrivateAccess = "true"))
	float PointJumpLandingSnapDistance = 200.0f;
	// landing吸着速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Move", meta = (AllowPrivateAccess = "true"))
	float PointJumpLandingSnapSpeed = 4000.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpPerfectWindow = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpGoodWindow = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpNormalForwardPower = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpGoodForwardPower = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpPerfectForwardPower = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch", meta = (AllowPrivateAccess = "true"))
	float PointJumpNormalUpPower = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch", meta = (AllowPrivateAccess = "true"))
	float PointJumpGoodUpPower = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch", meta = (AllowPrivateAccess = "true"))
	float PointJumpPerfectUpPower = 850.0f;

	// ポイントジャンプ状態
	// ポイントジャンプ対象位置
	FVector PointJumpTargetLocation = FVector::ZeroVector;
	// ポイントジャンプ可能かどうか
	bool bIsPointJumpingEnableJump = false;
	/** ポイントジャンプ着地までの先行入力 */
	bool bIsBufferedPointJump = false;
	EPointJumpResult BufferedPointJumpResult = EPointJumpResult::Normal;
	// ポイントジャンプの発射は実行されたか
	bool bHasExecutedPointJumpLaunch = false;

	// ポイントジャンプアニメーションstate
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PointJump|Animation", meta = (AllowPrivateAccess = "true"))
	EPointJumpState PointJumpState = EPointJumpState::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Animation", meta = (AllowPrivateAccess = "true"))
	float PointJumpStartTime = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Animation", meta = (AllowPrivateAccess = "true"))
	float PointJumpLandingTime = 0.15f;

	float PointJumpStateTimer = 0.0f;

	UPROPERTY()
	UPointJumpTargetComponent* CurrentPointJumpTarget = nullptr;


private:

	// 内部処理用の初期化・バインドヘルパー（外部に公開しないためprivateへ）
	void CustomSetupCharacterRotation();
	void CustomSetupCamera();
	void CustomSetupJump();
	void CustomSetupEnhancedInputMapping();

	void CustomBindLookInput(UEnhancedInputComponent* EnhancedInputComp);
	void CustomBindMoveInput(UEnhancedInputComponent* EnhancedInputComp);
	void CustomBindJumpInput(UEnhancedInputComponent* EnhancedInputComp);
	void CustomBindDashInput(UEnhancedInputComponent* EnhancedInputComp);
	void CustomBindPointJumpInput(UEnhancedInputComponent* EnhancedInputComp);

	// 入力実行関数
	void CustomLook(const FInputActionValue& Value);
	void CustomMove(const FInputActionValue& Value);
	void CustomOnJumpActionStarted();
	void CustomExecutePointJumpTimingInput();
	void CustomExecuteNormalJump();
	void CustomStartDash();
	void CustomStopDash();
	void CustomStartPointJump();

	// ポイントジャンプ
	void CustomUpdatePointJump(float DeltaTime);
	void CustomUpdatePointJumpStart(float DeltaTime);
	void CustomUpdatePointJumpPulling(float DeltaTime);

	void CustomUpdatePointJumpLandingSnap(float DeltaTime);
	void CustomUpdatePointJumpLanding(float DeltaTime);

	void CustomEnterPointJumpLandingSnap();
	void CustomEnterPointJumpReady();

	void CustomUpdatePointJumpLaunch();

	void CustomResetPointJump();
	bool CustomTryFindPointJumpTarget(UPointJumpTargetComponent*& OutTargetComponent) const;
	bool CustomIsValidPointJumpTarget(const UPointJumpTargetComponent* TargetComponent, const FVector& CameraLocation, const FVector& CameraForward) const;
	void CustomBeginPointJump(UPointJumpTargetComponent* TargetComponent);
	void CustomFinishPointJump(EPointJumpResult Result);
	EPointJumpResult CustomJudgePointJumpInputTiming() const;
	FVector CustomGetPointJumpLaunchDirection() const;
	float CustomGetPointJumpForwardPower(EPointJumpResult Result) const;
	float CustomGetPointJumpUpPower(EPointJumpResult Result) const;
	float CustomGetPointJumpRemainingTime() const;
	float CustomGetCurrentPointJumpArriveDistance() const;
	void CustomSetPointJumpMovementEnabled(bool bEnabled);
};
