#pragma once
#include "LocationServicesBPLibrary.h"


#include "HuntersGameWorldManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(HuntersGameWorldManagerLog, Log, All)

class AHuntersGamePlaneActor;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasPreviousLocation = false;

	UFUNCTION(BlueprintPure)
	FVector2D TileToLatLon(int32 TileX, int32 TileY);

	// UFUNCTION(BlueprintPure)
	// FVector TileToUnrealPosition(int32 TileX, int32 TileY);

	UFUNCTION(BlueprintPure)
	FVector TileToUnrealPosition(int32 TileX, int32 TileY, int32 PlayerTileX, int32 PlayerTileY);
	
private:
	UFUNCTION(CallInEditor, Category = "Grid")
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
	
	UFUNCTION(CallInEditor, Category = "Grid")
	void UpdateMap();
	
	UFUNCTION(BlueprintCallable)
	void UnloadTiles(int32 TileX, int32 TileY);
	
	FVector ConvertLatLonToUECoords(double Latitude, double Longitude, double InOriginLatitude, double InOriginLongitude);
	
	UFUNCTION(BlueprintPure)
	FVector GeoCoordsToUnrealWorld(FVector2D Coords) const;
	
	FVector TileXYToUnreal(int32 TileX, int32 TileY);

	UFUNCTION(BlueprintCallable)
	float CalculateDistance(float InitialLat, float InitialLon, float FinalLat, float FinalLon);

	// UFUNCTION(BlueprintCallable)
	// float GetGPSVelocity();
protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Zoom = 18;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LoadRadius = 0;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<AHuntersGamePlaneActor> PlaneActorClass;

	UPROPERTY(BlueprintReadWrite)
	FLocationServicesData PreviousLocation;
	
	double OriginLatitude;
	double OriginLongitude;
};
