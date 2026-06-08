// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// このヘッダを1回だけ読み込む
// 多重インクルード防止

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
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"

// Generated header (must be last)
// UnrealHeaderTool が自動生成するコード。
// UCLASS / UPROPERTY / UFUNCTION などの
// リフレクションシステムを動かすためのコードが入る。
#include "MyCharacter.generated.h"

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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* DashAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* JumpAction;

	bool bIsDashing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float DashSpeed = 1000.0f;

	// 入力の設定を行うためのおまじない関数（自分で新しく定義）
	void SetupEnhancedInputMapping();

	void BindLookInput(UEnhancedInputComponent* EnhancedInputComp);
	void BindMoveInput(UEnhancedInputComponent* EnhancedInputComp);
	void BindJumpInput(UEnhancedInputComponent* EnhancedInputComp);
	void BindDashInput(UEnhancedInputComponent* EnhancedInputComp);

	void SetupCamera();
	void SetupCharacterRotation();
	void SetupJump();

	void Look(const FInputActionValue& Value);
	void Move(const FInputActionValue& Value);
	void StartDash();
	void StopDash();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
