#include "ImageExport.h"
#include <IImageWrapper.h>
#include <IImageWrapperModule.h>
#include <Engine/Texture.h>

UImageExport::UImageExport(const FObjectInitializer& Object)
	:Super(Object)
{
	SupportedClass = UTexture2D::StaticClass();
	FormatExtension.Add(TEXT("jpg"));
	FormatExtension.Add(TEXT("png"));
	FormatDescription.Add(TEXT("Export png"));
	FormatDescription.Add(TEXT("Export jpg"));
}

bool UImageExport::SupportsObject(UObject* Object) const
{
	if(Super::SupportsObject(Object))
	{
		const UTexture2D* Tex = Cast<UTexture2D>(Object);
		if (Tex)
			return true;
	}
	return false;
}

bool UImageExport::ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex, uint32 PortFlags)
{
	UTexture2D* Tex = CastChecked<UTexture2D>(Object);
	check(Tex);
	const ETextureSourceFormat SourceFormat = Tex->Source.GetFormat();
	const ERGBFormat ExportFormat = SourceFormat == ETextureSourceFormat::TSF_G8 ? ERGBFormat::Gray : ERGBFormat::BGRA;
	const int32 ExportBitDepth = SourceFormat == ETextureSourceFormat::TSF_RGBA16 ? 16 : 8;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::Get().LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = nullptr;
	FString TypeStr(Type);

	if(TypeStr.Equals(TEXT("png"))||TypeStr.Equals(TEXT("PNG")))
	{
		ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	}else if(TypeStr.Equals(TEXT("jpg")) || TypeStr.Equals(TEXT("JPG")))
	{
		ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
	}

	TArray64<uint8> RawData;
	Tex->Source.GetMipData(RawData, 0);

	if(ImageWrapper.IsValid() && 
		ImageWrapper->SetRaw(
			RawData.GetData(),
			RawData.GetAllocatedSize(),
			Tex->Source.GetSizeX(),
			Tex->Source.GetSizeY(),
			ExportFormat,
			ExportBitDepth))
	{
		const TArray64<uint8>& Data = ImageWrapper->GetCompressed(100);
		if(Data.Num()!=0)
		{
			Ar.Serialize((void*)Data.GetData(), Data.GetAllocatedSize());
			return true;
		}
	}

	return false;
}