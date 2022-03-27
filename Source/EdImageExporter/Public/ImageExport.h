#pragma once
#include "CoreMinimal.h"
#include "Exporters/Exporter.h"
#include "ImageExport.generated.h"

UCLASS()
class UImageExport:public UExporter
{
	GENERATED_UCLASS_BODY()
public:
	virtual bool SupportsObject(UObject* Object) const override;
	virtual bool ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex = 0, uint32 PortFlags = 0) override;

};