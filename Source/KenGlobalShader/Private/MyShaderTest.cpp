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
#include "AssetRegistryModule.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Engine/Texture2D.h"
#include "MyComputeShader.h"
#include <RHI.h>

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
	//DECLARE_SHADER_TYPE(FMyShaderTest, Global);
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

	// 向渲染线程传输RenderTarget的方法
	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();

	// 向渲染线程传输FRHITexture2D的方法
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

bool UTestShaderBlueprintLibrary::LoadImageToTexture2D(const FString& ImagePath, UTexture2D*& InTexture, float& out_Width, float& out_Height)
{
	TArray<uint8> arr;
	FFileHelper::LoadFileToArray(arr, *ImagePath);

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
	FString extension = FPaths::GetExtension(ImagePath,false);

	EImageFormat format = EImageFormat::Invalid;
	if(extension.Equals(TEXT("jpg"),ESearchCase::IgnoreCase) || extension.Equals(TEXT("jpeg"), ESearchCase::IgnoreCase))
	{
		format = EImageFormat::JPEG;
	}else if(extension.Equals(TEXT("png"), ESearchCase::IgnoreCase))
	{
		format = EImageFormat::PNG;
	} else if (extension.Equals(TEXT("bmp"), ESearchCase::IgnoreCase))
	{
		format = EImageFormat::BMP;
	}

	TSharedPtr<IImageWrapper> iw = ImageWrapperModule.CreateImageWrapper(format);

	if(iw.IsValid())
	{
		bool success = iw->SetCompressed(arr.GetData(), arr.Num() * sizeof(uint8));
		if (!success)
			return false;

		TArray<uint8> data;
		iw->GetRaw(ERGBFormat::BGRA, 8, data);
		InTexture = UTexture2D::CreateTransient(iw->GetWidth(), iw->GetHeight(), PF_B8G8R8A8);
		if(InTexture)
		{
			InTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			uint8* mip0 = (uint8*)InTexture->PlatformData->Mips[0].BulkData.Realloc(data.Num() * sizeof(uint8));
			FMemory::Memcpy(mip0, data.GetData(), data.Num() * sizeof(uint8));
			InTexture->PlatformData->Mips[0].BulkData.Unlock();
			InTexture->UpdateResource();

			out_Width = iw->GetWidth();
			out_Height = iw->GetHeight();

			return true;
		}
	}
	return false;
}

bool UTestShaderBlueprintLibrary::SaveImageFromTexture2D(UTexture2D* InTex, const FString& DesPath)
{
	if (!InTex)
		return false;


	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
	FString extension = FPaths::GetExtension(DesPath, false);
	EImageFormat format = EImageFormat::Invalid;
	if (extension.Equals(TEXT("jpg"), ESearchCase::IgnoreCase) || extension.Equals(TEXT("jpeg"), ESearchCase::IgnoreCase))
	{
		format = EImageFormat::JPEG;
	} else if (extension.Equals(TEXT("png"), ESearchCase::IgnoreCase))
	{
		format = EImageFormat::PNG;
	}

	//创建Wrapper
	TSharedPtr<IImageWrapper> wrapper = ImageWrapperModule.CreateImageWrapper(format);

	
	TArray64<uint8> outData;
	//从贴图中获取原始数据
	InTex->Source.GetMipData(outData, 0);
	int32 width = InTex->Source.GetSizeX();
	int32 height = InTex->Source.GetSizeY();

	int depth = (InTex->Source.GetFormat() == ETextureSourceFormat::TSF_RGBA16) ? 16 : 8;

	if(wrapper.IsValid() && wrapper->SetRaw(outData.GetData(),outData.GetAllocatedSize(),width,height,ERGBFormat::BGRA,depth))
	{
		const TArray64<uint8> CompressedData = wrapper->GetCompressed(100);
		FFileHelper::SaveArrayToFile(CompressedData, *DesPath);
		return true;
	}

	return false;
}

void UTestShaderBlueprintLibrary::CreateAndSaveBitMap()
{
	check(IsInGameThread());

	TArray<FColor> colors;
	for(int32 i = 0; i< 256 * 256; i++)
	{
		colors.Add(FColor::Blue);
	}

	//存储为图片
	IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true);
	const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("VisualizeTexture"));
	uint32 ExtendXWithMSAA = colors.Num() / 256;

	FFileHelper::CreateBitmap(*ScreenFileName, ExtendXWithMSAA, 256, colors.GetData());
	UE_LOG(LogConsoleResponse, Display, TEXT("Content was saved to \"%s\""), *FPaths::ScreenShotDir());
}

void UTestShaderBlueprintLibrary::CreateAndSaveUTexture()
{
	FString TextureName = TEXT("AutoGen");
	FString PackageName = TEXT("/Game/ProceduralTextures/");
	PackageName += TextureName;
	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *TextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);

	NewTexture->AddToRoot();
	NewTexture->PlatformData = new FTexturePlatformData();
	NewTexture->PlatformData->SizeX = 256;
	NewTexture->PlatformData->SizeY = 256;
	NewTexture->PlatformData->SetNumSlices(1);
	NewTexture->PlatformData->PixelFormat = EPixelFormat::PF_B8G8R8A8;

	uint8* Pixels = new uint8[256 * 256 * 4];
	for(int32 y = 0; y< 256; y++)
	{
		for(int32 x=0; x<256; x++)
		{
			int32 curPixelIndex = ((y * 256) + x);
			Pixels[4 * curPixelIndex] = (curPixelIndex+30)%255;
			Pixels[4 * curPixelIndex + 1] = (curPixelIndex*2 + 100) % 255;
			Pixels[4 * curPixelIndex + 2] = (curPixelIndex * 3 + 200) % 255;
			Pixels[4 * curPixelIndex + 3] = 255;
		}
	}

	// Allocate first mipmap
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->PlatformData->Mips.Add(Mip);
	Mip->SizeX = 256;
	Mip->SizeY = 256;

	// Lock the texture so it can be modified
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	uint8* TextureData = (uint8*)Mip->BulkData.Realloc(256 * 256 * 4);
	FMemory::Memcpy(TextureData, Pixels, sizeof(uint8) * 256 * 256 * 4);
	Mip->BulkData.Unlock();

	NewTexture->Source.Init(256, 256, 1, 1, ETextureSourceFormat::TSF_RGBA8, Pixels);

	NewTexture->UpdateResource();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	bool bSaved = UPackage::SavePackage(Package, NewTexture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);

	delete[] Pixels;
}


static void TextureWriting_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	ERHIFeatureLevel::Type FeatureLevel,
	UTexture2D* Texture
)
{
	check(IsInRenderingThread());
	if (Texture == nullptr)
	{
		return;
	}

	// 向渲染线程传输FRHITexture2D的方法
	FRHITexture2D* MyRHITexture2D = Texture->TextureReference.TextureReferenceRHI->GetReferencedTexture()->GetTexture2D();


	TArray<FColor> Bitmap;
	uint32 LolStride = 0;
	char* TextureDataPtr = (char*)RHICmdList.LockTexture2D(MyRHITexture2D, 0, EResourceLockMode::RLM_ReadOnly, LolStride, false);

	for (uint32 Row = 0; Row < MyRHITexture2D->GetSizeY(); ++Row)
	{
		uint32* PixelPtr = (uint32*)TextureDataPtr;
		for (uint32 Col = 0; Col < MyRHITexture2D->GetSizeX(); ++Col)
		{
			uint32 EncodedPixel = *PixelPtr;
			uint8 r = (EncodedPixel & 0x000000FF);
			uint8 g = (EncodedPixel & 0x0000FF00) >> 8;
			uint8 b = (EncodedPixel & 0x00FF0000) >> 16;
			uint8 a = (EncodedPixel & 0xFF000000) >> 24;
			FColor col = FColor(r, g, b, a);
			Bitmap.Add(FColor(b, g, r, a));
			PixelPtr++;
		}
		// move to next row:
		TextureDataPtr += LolStride;
	}
	RHICmdList.UnlockTexture2D(MyRHITexture2D, 0, false);

	if (Bitmap.Num())
	{
		IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true);
		const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("VisualizeTexture"));
		uint32 ExtendXWithMSAA = Bitmap.Num() / Texture->GetSizeY() == 0? 1 : Texture->GetSizeY();
		// Save the contents of the array to a bitmap file. (24bit only so alpha channel is dropped)
		FFileHelper::CreateBitmap(*ScreenFileName, ExtendXWithMSAA, Texture->GetSizeY(), Bitmap.GetData());
		UE_LOG(LogConsoleResponse, Display, TEXT("Content was saved to \"%s\""), *FPaths::ScreenShotDir());
	} else
	{
		UE_LOG(LogConsoleResponse, Error, TEXT("Failed to save BMP, format or texture type is not supported"));
	}
}

void UTestShaderBlueprintLibrary::TextureWriting(UTexture2D* TextureToBeWritten,const UObject* WorldContextObject)
{
	check(IsInGameThread());

	UWorld* World = WorldContextObject->GetWorld();
	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[FeatureLevel, TextureToBeWritten](FRHICommandListImmediate& RHICmdList)
		{
			TextureWriting_RenderThread
			(
				RHICmdList,
				FeatureLevel,
				TextureToBeWritten
			);
		}
	);
}




static void DrawCheckerBoard_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* TextureRenderTargetResource,
	ERHIFeatureLevel::Type FeatureLevel,
	FMyShaderStructData UniformData
)
{
	check(IsInRenderingThread());

	FTexture2DRHIRef RenderTargetTexture = TextureRenderTargetResource->GetRenderTargetTexture();
	uint32 GGroupSize = 32;
	FIntPoint FullResolution = FIntPoint(RenderTargetTexture->GetSizeX(), RenderTargetTexture->GetSizeY());
	uint32 GroupSizeX = FMath::DivideAndRoundUp((uint32)RenderTargetTexture->GetSizeX(), GGroupSize);
	uint32 GroupSizeY = FMath::DivideAndRoundUp((uint32)RenderTargetTexture->GetSizeY(), GGroupSize);

	TShaderMapRef<FCheckerBoardComputeShader>ComputeShader(GetGlobalShaderMap(FeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());


	//创建一个贴图资源和UAV视图 —— 和 RenderTargetTexture无关，这里只是用到了后者尺寸
	FRHIResourceCreateInfo CreateInfo;
	FTexture2DRHIRef GSurfaceTexture2D = RHICreateTexture2D(RenderTargetTexture->GetSizeX(), RenderTargetTexture->GetSizeY(), PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	FUnorderedAccessViewRHIRef GUAV = RHICreateUnorderedAccessView(GSurfaceTexture2D);

	ComputeShader->SetParameters(RHICmdList, RenderTargetTexture, GUAV);

	FRHITransitionInfo UAVTransition(GUAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute);
	const FRHITransition* GFxToAsyncTransition = RHICreateTransition(ERHIPipeline::Graphics, ERHIPipeline::AsyncCompute, ERHICreateTransitionFlags::None, MakeArrayView(&UAVTransition, 1));

	RHICmdList.BeginTransition(GFxToAsyncTransition);
	FRHIAsyncComputeCommandListImmediate& RHICmdListComputeImmediate = FRHICommandListExecutor::GetImmediateAsyncComputeCommandList();	
	DispatchComputeShader(RHICmdList, ComputeShader, GroupSizeX, GroupSizeY, 1);
	RHICmdListComputeImmediate.EndTransition(GFxToAsyncTransition);

	//把CS输出的UAV贴图拷贝回RenderTargetTexture
	/*FRHICopyTextureInfo CopyInfo;
	RHICmdList.CopyTexture(GSurfaceTexture2D, RenderTargetTexture, CopyInfo);*/

	// 下一个渲染函数使用上面生成的贴图
	DrawTestShaderRenderTarget_RenderThread(RHICmdList, TextureRenderTargetResource, FeatureLevel, FName(), FColor::Red, GSurfaceTexture2D, UniformData);
}



void UTestShaderBlueprintLibrary::DrawCheckerBoard(
	const UObject* WorldContextObject, 
	class UTextureRenderTarget2D* OutputRenderTarget,
	FMyShaderStructData UniformData
	)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		FMessageLog("Blueprint").Warning(
			LOCTEXT("UGraphicToolsBlueprintLibrary::DrawCheckerBoard",
				"DrawUVDisplacementToRenderTarget: Output render target is required."));
		return;
	}

	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
	ERHIFeatureLevel::Type FeatureLevel = WorldContextObject->GetWorld()->Scene->GetFeatureLevel();


	ENQUEUE_RENDER_COMMAND(CaptureCommand)
		(
			[TextureRenderTargetResource, FeatureLevel,UniformData](FRHICommandListImmediate& RHICmdList)
			{
				DrawCheckerBoard_RenderThread
				(
					RHICmdList,
					TextureRenderTargetResource,
					FeatureLevel,
					UniformData
				);
			}
	);
}

#undef LOCTEXT_NAMESPACE  