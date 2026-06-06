// Fill out your copyright notice in the Description page of Project Settings.

//Self include first
#include "MyCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Controller.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"

// 
AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// キャラの回転はコントローラーの回転に従わないようにする
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 移動方向にキャラを向ける
	GetCharacterMovement()->bOrientRotationToMovement = true; // キャラクターの移動方向に回転する
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // 回転速度

	// カメラアームの作成
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent); // キャラクターのルートコンポーネントにアタッチ
	CameraBoom->TargetArmLength = 300.0f;		// カメラとキャラクターの距離
	CameraBoom->bUsePawnControlRotation = true; // コントローラーの回転をカメラアームに適用

	// カメラ作成
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // カメラアームの先端にアタッチ
	FollowCamera->bUsePawnControlRotation = false; // カメラはコントローラーの回転をしない
}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (DefaultMappingContext)
				{
					Subsystem->AddMappingContext(DefaultMappingContext, 0);
				}
			}
		}
	}

}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// カメラ回転入力の処理
void AMyCharacter::Look(const FInputActionValue& Value)
{
	//IA_Lookの入力は FVector2D 型なので、Value.Get<FVector2D>() で入力値を取得する
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr) //
	{
		AddControllerYawInput(LookAxisVector.X); // X = 水平方向の回転
		AddControllerPitchInput(-LookAxisVector.Y); // Y = 垂直方向の回転
	}
}

// 移動入力の処理
void AMyCharacter::Move(const FInputActionValue& Value)
{
	//IA_Moveの入力は FVector2D 型なので、Value.Get<FVector2D>() で入力値を取得する
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator ControlRotation = Controller->GetControlRotation(); // コントローラーの回転を取得する
		const FRotator YawRotation(0, ControlRotation.Yaw, 0); // コントローラーの回転からヨー成分だけを抽出する

		// キャラクターの前方向と右方向を取得する
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y); // Y = 前後移動 
		AddMovementInput(RightDirection,   MovementVector.X); // X = 左右移動
	}
}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//Enhanced Input Component にキャストして、入力アクションをバインドする
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyCharacter::Look);
		}

		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
		}
	}
}

