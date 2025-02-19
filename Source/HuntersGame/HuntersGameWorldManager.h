#pragma once


#include "HuntersGameWorldManager.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct FPlayerCoords
{
	GENERATED_BODY()

	FPlayerCoords(): Longitude(0), Latitude(0){}
	FPlayerCoords(float Lat, float Lon)
	{
		Longitude = Lon;
		Latitude = Lat;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Longitude;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Latitude;
	
	FPlayerCoords GetPlayerCoords()
	{
		return {Longitude, Latitude};
	}
};

USTRUCT(Blueprintable, BlueprintType)
struct FPlayerTileLocation
{
	GENERATED_BODY()
	
	FPlayerTileLocation(): TileX(0), TileY(0){}
	FPlayerTileLocation(int32 X, int32 Y)
	{
		TileX = X;
		TileY = Y;
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TileX;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TileY;

	FPlayerTileLocation GetPlayerTile()
	{
		return {TileX, TileY};
	}
};

UCLASS()
class AHuntersGameWorldManager : public AActor
{
	GENERATED_BODY()
public:
	AHuntersGameWorldManager();

	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor, Category = "Grid")
	void RegenerateGrid();

private:
	void ClearGrid();
	void GenerateGrid();
	FString GetOSMTextureURL(int32 x, int32 y, int32 zoom);
	void DownloadAndApplyTexture(UStaticMeshComponent* TileMesh, FString URL);

	UFUNCTION(BlueprintCallable)
	static int32 LongitudeToTileX(float Lon, int32 Zoom);
	
	UFUNCTION(BlueprintCallable)
	static int32 LatitudeToTileY(float Lat, int32 Zoom);
	
	UFUNCTION(BlueprintCallable)
	void SetPlayerCoords(float Lat, float Lon);
	
	UFUNCTION(BlueprintCallable, CallInEditor)
	void UpdateMap();
	
	// UFUNCTION(BlueprintCallable)
	// void LoadTile(int32 TileX, int32 TileY);
	//
	
	UFUNCTION(BlueprintCallable)
	void UnloadTiles(int32 TileX, int32 TileY);
	
	// UFUNCTION(BlueprintCallable)
	// FPlayerTileLocation GetPlayerTile();
	//
	// UFUNCTION(BlueprintCallable)
	// FVector GetTileWorldPosition(int32 TileX, int32 TileY);
	//
	// void ConvertWorldToGPS(FVector WorldLocation, float& OutLat, float& OutLon);
	//
	// void DownloadTileImage(int32 TileX, int32 TileY);
	//
	// void OnImageDownloaded(int32 TileX, int32 TileY, TArray<uint8> ImageData);

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Zoom = 16;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LoadRadius = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TileSize = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UpdateFrequency = 2.0f;
	
	FTimerHandle UpdateMapTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPlayerCoords PlayerCoords;
	
	UPROPERTY(BlueprintReadWrite)
	FPlayerTileLocation PlayerTileLocation;

	UPROPERTY()
	TMap<FIntPoint, UStaticMeshComponent*> LoadedTiles;

	UPROPERTY(BlueprintReadOnly)
	AActor* PlayerActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterial> BaseMaterial;
};
