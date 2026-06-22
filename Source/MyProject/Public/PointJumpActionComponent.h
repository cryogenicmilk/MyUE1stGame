// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h" // アクタをinclude
#include "PointJumpActionComponent.generated.h"

// 前方宣言：循環インクルードを防ぎ、コンパイルを速くする
class ACharacter;
class UPointJumpTargetComponent;

enum class EPointJumpResult : uint8
{
	Normal,
	Perfect
};

UENUM(BlueprintType)
enum class EPointJumpState : uint8
{
	None,
	Startup,
	Pulling,
	LandingSnap,// ポイントに吸着するのみ
	Landing,	// ポイント上で待機など
	Launch,
	AfterLaunch
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent)) // マクロをコンポーネント用に変更
class MYPROJECT_API UPointJumpActionComponent : public UActorComponent // 先頭U
{
	GENERATED_BODY()
	
public:	
	UPointJumpActionComponent();

	UFUNCTION(BlueprintCallable, Category = "PointJump|Animation")
	void CustomOnPointJumpLaunchAnimationFinished();

	UFUNCTION(BlueprintCallable, Category = "PointJump|Animation")
	void CustomOnPointJumpLandingAnimationFinished();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 外部から呼ばれるメインロジック入口
	void CustomTryStartPointJump();

private:
	/// =================================================================
	/// a
	/// =================================================================
	// 入力実行関数
	void CustomOnJumpActionStarted();
	void CustomExecutePointJumpTimingInput();

	// ポイントジャンプstate
	void CustomUpdatePointJump(float DeltaTime);

	void CustomUpdateStartup(float DeltaTime);

	void CustomUpdatePulling(float DeltaTime);

	void CustomEnterLandingSnap();
	void CustomUpdateLandingSnap(float DeltaTime);

	void CustomEnterLanding();
	void CustomUpdateLanding(float DeltaTime);

	void CustomEnterLaunch();
	void CustomUpdateLaunch();

	// Timing
	void CustomBufferPointJumpInput();

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

private: // BP
	// 親となるキャラクターへのポインタ
	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr;
	// AMyCharacter* ← コンポーネントが AMyCharacter に強く依存すると、せっかく分けたのに結合が強くなる

	UPROPERTY()
	UPointJumpTargetComponent* CurrentPointJumpTarget = nullptr;

	FVector PointJumpTargetLocation = FVector::ZeroVector;
	/// =================================================================
	/// パラメータ
	/// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Search", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SearchPointTargetRadius = 2500.0f;

	// カメラ前方との一致度。1に近いほど画面中央付近しか拾わない。
	// 視野角
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Search", meta = (AllowPrivateAccess = "true", ClampMin = "-1.0", ClampMax = "1.0"))
	float PointJumpMinViewDot = 0.55f;

	// ポイントが近かいとする距離
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Move", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointTargetArriveDistance = 90.0f;

	// ターゲットに近づく速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Move", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PullSpeed = 2800.0f;

	// Start, Pulling中に方向入力があったら保存する用
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PointJump|Move", meta = (AllowPrivateAccess = "true"))
	FVector PointJumpBufferedMoveDirection = FVector::ZeroVector;
	// 
	bool bHasPointJumpMoveInput = false;

	//ポイントが最初から近い場合Pullingを飛ばしてlandingになる距離
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Transition", meta = (AllowPrivateAccess = "true"))
	float PullingSkipDistance = 250.0f;
	// Pulling中ターゲットにこの距離まで近づいたら LandingSnap に切り替える距離
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Transition", meta = (AllowPrivateAccess = "true"))
	float LandingSnapStartDistance = 200.0f;
	// LandingSnap中にターゲットへ吸着する速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|LandingSnap", meta = (AllowPrivateAccess = "true"))
	float LandingSnapSpeed = 4000.0f;
	// LandingSnap中、この距離以内なら吸着完了
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|LandingSnap", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float LandingSnapCompleteDistance = 5.0f;

	/// =================================================================
	/// タイミング判定 → 結果・Result
	/// =================================================================
	
	// Perfect受付時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpPerfectWindow = 0.12f;

	// 前
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpNormalForwardPower = 1100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpPerfectForwardPower = 1600.0f;
	
	// 上
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch", meta = (AllowPrivateAccess = "true"))
	float PointJumpNormalUpPower = 450.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch", meta = (AllowPrivateAccess = "true"))
	float PointJumpPerfectUpPower = 850.0f;

	// ポイントジャンプ状態
	// ポイントジャンプ対象位置
	FVector PointJumpTargetLocation = FVector::ZeroVector;

	// ポイントジャンプ可能かどうか制御
	bool bIsPointJumpingEnableJump = false;

	// Landing位置までの先行入力
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
};
