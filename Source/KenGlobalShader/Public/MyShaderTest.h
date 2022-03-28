#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyShaderTest.generated.h"


USTRUCT(BlueprintType)
struct FMyShaderStructData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorOne;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorTwo;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorThree;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	FLinearColor ColorFour;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = ShaderData)
	int32 ColorIndex;
};

UCLASS(MinimalAPI)
class UTestShaderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "ShaderTestPlugin", meta = (WorldContext = "WorldContexObject"))
	static void DrawTestShaderRenderTarget(
		const UObject* WorldContext, 
		class UTextureRenderTarget2D* OutputRenderTarget, 
		FLinearColor MyColor,
		UTexture2D* MyTexture,
		FMyShaderStructData UniformData
	);


	UFUNCTION(BlueprintCallable, Category = "ImageTool", meta = (WorldContext = "WorldContexObject"))
	static bool LoadImageToTexture2D(const FString& ImagePath, UTexture2D* &InTexture, float& out_Width, float& out_Height);

	UFUNCTION(BlueprintCallable, Category = "ImageTool")
	static bool SaveImageFromTexture2D(UTexture2D* InTex, const FString& DesPath);

	UFUNCTION(BlueprintCallable, Category = "ImageTool", meta = (WorldContext = "WorldContexObject"))
	static void CreateAndSaveBitMap();

	UFUNCTION(BlueprintCallable, Category = "ImageTool", meta = (WorldContext = "WorldContexObject"))
	static void CreateAndSaveUTexture();

	UFUNCTION(BlueprintCallable, Category = "ImageTool")
	static void TextureWriting(UTexture2D* TextureToBeWritten, const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "ImageTool")
	static void DrawCheckerBoard(const UObject* WorldContextObject, class UTextureRenderTarget2D* OutputRenderTarget);
};