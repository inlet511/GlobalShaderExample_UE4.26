#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyShaderTest.generated.h"

UCLASS(MinimalAPI)
class UTestShaderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "ShaderTestPlugin", meta = (WorldContext = "WorldContexObject"))
	static void DrawTestShaderRenderTarget(class UTextureRenderTarget2D* OutputRenderTarget, AActor* Ac, FLinearColor MyColor,UTexture2D* MyTexture);
};