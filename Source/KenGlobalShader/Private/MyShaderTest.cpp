// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.  

#include "MyShaderTest.h"  

#include "Engine/TextureRenderTarget2D.h"  
#include "Engine/World.h"  
#include "GlobalShader.h"  
#include "PipelineStateCache.h"  
#include "RHIStaticStates.h"  
#include "SceneUtils.h"  
#include "SceneInterface.h"  
#include "ShaderParameterUtils.h"  
#include "Logging/MessageLog.h"  
#include "Internationalization/Internationalization.h"  
#include "StaticBoundShaderState.h"  
#include "RenderResource.h"

#define LOCTEXT_NAMESPACE "TestShader"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FMyUniform, )
SHADER_PARAMETER(FVector4, ColorOne)
SHADER_PARAMETER(FVector4, ColorTwo)
SHADER_PARAMETER(FVector4, ColorThree)
SHADER_PARAMETER(FVector4, ColorFour)
SHADER_PARAMETER(uint32, ColorIndex)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FMyUniform, "MyUniform");

class FMyShaderTest : public FGlobalShader
{
	DECLARE_INLINE_TYPE_LAYOUT(FMyShaderTest, NonVirtual);
public:

	FMyShaderTest() {}

	FMyShaderTest(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		SimpleColorVal.Bind(Initializer.ParameterMap, TEXT("SimpleColor"));
		MyTextureVal.Bind(Initializer.ParameterMap, TEXT("MyTexture"));
		MyTextureSamplerVal.Bind(Initializer.ParameterMap, TEXT("MyTextureSampler"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		//return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);  
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		//OutEnvironment.SetDefine(TEXT("TEST_MICRO"), 1);
	}

	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const FLinearColor& MyColor,
		FRHITexture2D* MyTexture2D,
		FMyShaderStructData& UniformData
	)
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), SimpleColorVal, MyColor);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), MyTextureVal, MyTextureSamplerVal, TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), MyTexture2D);

		FMyUniform uni;
		uni.ColorOne = UniformData.ColorOne;
		uni.ColorTwo = UniformData.ColorTwo;
		uni.ColorThree = UniformData.ColorThree;
		uni.ColorFour = UniformData.ColorFour;
		uni.ColorIndex = UniformData.ColorIndex;

		TUniformBufferRef<FMyUniform> Data = TUniformBufferRef<FMyUniform>::CreateUniformBufferImmediate(uni, UniformBuffer_SingleFrame);
		SetUniformBufferParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), GetUniformBufferParameter<FMyUniform>(), Data);

	}

private:

	LAYOUT_FIELD(FShaderParameter, SimpleColorVal);
	LAYOUT_FIELD(FShaderResourceParameter, MyTextureVal);
	LAYOUT_FIELD(FShaderResourceParameter, MyTextureSamplerVal);

};

class FShaderTestVS : public FMyShaderTest
{
	DECLARE_SHADER_TYPE(FShaderTestVS, Global);

public:
	FShaderTestVS() {}

	FShaderTestVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyShaderTest(Initializer)
	{

	}
};


class FShaderTestPS : public FMyShaderTest
{
	DECLARE_SHADER_TYPE(FShaderTestPS, Global);

public:
	FShaderTestPS() {}

	FShaderTestPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyShaderTest(Initializer)
	{

	}
};

IMPLEMENT_SHADER_TYPE(, FShaderTestVS, TEXT("/GlobalShaderPlugin/MyShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FShaderTestPS, TEXT("/GlobalShaderPlugin/MyShader.usf"), TEXT("MainPS"), SF_Pixel)

struct FMyVertex{
	FVector4 Position;
	FVector2D UV;
};

class FMyVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FMyVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FMyVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FMyVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI->Release();
	}
};

static void DrawTestShaderRenderTarget_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* OutputRenderTargetResource,
	ERHIFeatureLevel::Type FeatureLevel,
	FName TextureRenderTargetName,
	FLinearColor MyColor,
	FRHITexture2D* MyRHITexture2D,
	FMyShaderStructData UniformData
)
{
	check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS  
	FString EventName;
	TextureRenderTargetName.ToString(EventName);
	SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("ShaderTest %s"), *EventName);
#else  
	SCOPED_DRAW_EVENT(RHICmdList, DrawUVDisplacementToRenderTarget_RenderThread);
#endif



	FRHITexture2D* RenderTargetTexture = OutputRenderTargetResource->GetRenderTargetTexture();
	RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::SRVMask, ERHIAccess::RTV));
	FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Load_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawColorPass"));
	{

		// SetViewport
		RHICmdList.SetViewport(
			0, 0, 0.f,
			OutputRenderTargetResource->GetSizeX(), OutputRenderTargetResource->GetSizeY(), 1.f);

		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
		TShaderMapRef<FShaderTestVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FShaderTestPS> PixelShader(GlobalShaderMap);

		FMyVertexDeclaration VertexDesc;
		VertexDesc.InitRHI();

		// Set the graphic pipeline state.  
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = /*GetVertexDeclarationFVector4();*/ VertexDesc.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);


		PixelShader->SetParameters(RHICmdList, MyColor,MyRHITexture2D,UniformData);
		
		// Vertex Buffer Begins --------------------------
		FRHIResourceCreateInfo createInfo;
		FVertexBufferRHIRef MyVertexBufferRHI = RHICreateVertexBuffer(sizeof(FMyVertex) * 4, BUF_Static, createInfo);
		void* VoidPtr = RHILockVertexBuffer(MyVertexBufferRHI, 0, sizeof(FMyVertex) * 4, RLM_WriteOnly);

		FMyVertex v[4];
		// LT
		v[0].Position = FVector4(-1.0f, 1.0f, 0.0f, 1.0f);
		v[0].UV = FVector2D(0, 1.0f);

		// RT
		v[1].Position = FVector4(1.0f, 1.0f, 0.0f, 1.0f);
		v[1].UV = FVector2D(1.0f, 1.0f);

		// LB
		v[2].Position = FVector4(-1.0f, -1.0f, 0.0f, 1.0f);
		v[2].UV = FVector2D(0.0f, 0.0f);

		// RB
		v[3].Position = FVector4(1.0f, -1.0f, 0.0f, 1.0f);
		v[3].UV = FVector2D(1.0f, 0.0f);

		FMemory::Memcpy(VoidPtr, &v, sizeof(FMyVertex) * 4);
		RHIUnlockVertexBuffer(MyVertexBufferRHI);
		// Vertex Buffer Ends --------------------------
		

		// Index Buffer Begins--------------------
		static const uint16 Indices[6] = {
			0,1,2,
			2,1,3
		};
		
		FRHIResourceCreateInfo IndexBufferCreateInfo;
		FIndexBufferRHIRef MyIndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), sizeof(uint16)*6, BUF_Static,IndexBufferCreateInfo);
		void* VoidPtr2 = RHILockIndexBuffer(MyIndexBufferRHI, 0, sizeof(uint16) * 6, RLM_WriteOnly);
		FMemory::Memcpy(VoidPtr2, Indices, sizeof(uint16) * 6);

		RHICmdList.SetStreamSource(0, MyVertexBufferRHI, 0);
		RHIUnlockIndexBuffer(MyIndexBufferRHI);
		// Index Buffer Ends-----------------------

		// Draw Indexed
		RHICmdList.DrawIndexedPrimitive(MyIndexBufferRHI, 0, 0, 4, 0, 2, 1);

		MyIndexBufferRHI.SafeRelease();
		MyVertexBufferRHI.SafeRelease();
	}

	RHICmdList.EndRenderPass();
	RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::RTV, ERHIAccess::SRVMask));
}

void UTestShaderBlueprintLibrary::DrawTestShaderRenderTarget(
	const UObject* WorldContextObject,
	UTextureRenderTarget2D* OutputRenderTarget,
	FLinearColor MyColor,
	UTexture2D* MyTexture,
	FMyShaderStructData UniformData
)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		return;
	}

	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();

	FRHITexture2D* MyRHITexture2D = MyTexture->TextureReference.TextureReferenceRHI->GetReferencedTexture()->GetTexture2D();

	UWorld* World = WorldContextObject->GetWorld();
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
	FName TextureRenderTargetName = OutputRenderTarget->GetFName();
	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[TextureRenderTargetResource, FeatureLevel, MyColor, TextureRenderTargetName,MyRHITexture2D, UniformData](FRHICommandListImmediate& RHICmdList)
	{
		DrawTestShaderRenderTarget_RenderThread(RHICmdList, TextureRenderTargetResource, FeatureLevel, TextureRenderTargetName, MyColor, MyRHITexture2D, UniformData);
	}
	);

}


void UTestShaderBlueprintLibrary::WriteToUTexture(UTexture2D* TargetTexture, AActor* selfref)
{
	check(IsInGameThread());

	if (selfref == nullptr && TargetTexture == nullptr) return;

	UTexture2D* Result = UTexture2D::CreateTransient(256, 256, PF_B8G8R8A8);


	//TargetTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	//TargetTexture->SRGB = 0;

	FTexture2DMipMap& mipmap = TargetTexture->PlatformData->Mips[0];
	uint16* MipData = reinterpret_cast<uint16*>(mipmap.BulkData.Lock(LOCK_READ_WRITE));
	check(MipData);

	int32 textureX = TargetTexture->PlatformData->SizeX;
	int32 textureY = TargetTexture->PlatformData->SizeY;
	TArray<FColor>colors;
	for(int32 i = 0; i< textureX * textureY; i++)
	{
		colors.Add(FColor::Blue);
	}

	int32 stride = (int32)(sizeof(uint8) * 4);
	FMemory::Memcpy(MipData, colors.GetData(), textureX * textureY * stride);
	mipmap.BulkData.Unlock();

	TargetTexture->UpdateResource();
}


#undef LOCTEXT_NAMESPACE  