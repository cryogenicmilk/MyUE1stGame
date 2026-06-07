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

	// デフォルトだと、キャラクターはコントローラーの回転に合わせて回転する設定になっているため、これらを無効にする
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 移動方向にキャラを向ける
	GetCharacterMovement()->bOrientRotationToMovement = true; // これを有効にすると、キャラクターは移動方向に向くようになる
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // 回転速度

	// カメラアームの作成
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")); // USpringArmComponentがあるお陰で、カメラが壁に衝突しても、カメラがキャラクターに近づくようになる
	CameraBoom->SetupAttachment(RootComponent); // キャラクターのルートコンポーネントにアタッチ
	CameraBoom->TargetArmLength = 300.0f;		// カメラとキャラクターの距離
	CameraBoom->bUsePawnControlRotation = true; // コントローラーの回転をカメラアームに適用

	// カメラ作成
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera")); // プレイヤーが見る画面そのもの
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // カメラアームの先端にアタッチ
	FollowCamera->bUsePawnControlRotation = false; // カメラはコントローラーの回転をしない
}

// unityの Start() に相当する関数
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetupEnhancedInputMapping(); // 入力の設定を行うためのおまじない関数（自分で新しく定義）
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

	/// ////////////
	/// c++(UE)では特に準備できていないデータにアクセスするとクラッシュしてしまうため、「本当にこのデータある？壊れてない？」と１つずつ確認していく必要があります。
	/// ////////////
// 入力の設定を行うためのおまじない関数（自分で新しく定義）
void AMyCharacter::SetupEnhancedInputMapping()
{
	// 1. 人間プレイヤーかチェック
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;
	
	// 2. ローカルプレイヤーかチェック
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer) return;
	
	// 3. 入力サブシステム（管理人）がいるかチェック
	auto* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem) return;
	
	// 4. エディタ側でアセットが割り当てられているかチェック
	if (!DefaultMappingContext) return;
	
	// --- すべてのセキュリティを突破！ 本題の処理 ---
	Subsystem->AddMappingContext(DefaultMappingContext, 0);
}

// カメラ回転入力の処理
void AMyCharacter::Look(const FInputActionValue& Value)
{
	//IA_Lookの入力は FVector2D 型なので、Value.Get<FVector2D>() で入力値を取得する
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X); // X = 水平方向の回転
		AddControllerPitchInput(-LookAxisVector.Y); // Y = 垂直方向の回転（上下反転を防ぐためマイナス）
		// CameraBoom->bUsePawnControlRotation = true; になっているため、コントローラーの回転がカメラアームに適用され、カメラがキャラクターを追従して回転するようになる
	}
}

// 移動入力の処理
void AMyCharacter::Move(const FInputActionValue& Value)
{
	//IA_Moveの入力は FVector2D 型なので、Value.Get<FVector2D>() で入力値を取得する
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator ControlRotation = Controller->GetControlRotation(); // 1. まず「今、カメラ（コントローラー）はどっちを向いてる？」と確認します（ControlRotation）
		const FRotator YawRotation(0, ControlRotation.Yaw, 0); // 2. 上下を見上げている角度（Pitch）などは邪魔なので、水平方向の向き（Yaw）だけを抜き出します（YawRotation）
		// 3. その向きを基準にして「前（Forward）」と「右（Right）」のベクトルを計算し、スティックの傾き具合（MovementVector）を掛け合わせてキャラクターを動かしています。

		// キャラクターの前方向と右方向を取得する
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y); // Y = 前後移動 
		AddMovementInput(RightDirection, MovementVector.X); // X = 左右移動
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

