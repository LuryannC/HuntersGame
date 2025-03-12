#include "HuntersGameWorldManager.h"

#include "HttpModule.h"
#include "HuntersGamePlaneActor.h"
#include "ImageUtils.h"
#include "LocationServicesBPLibrary.h"
#include "LocationServicesImpl.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(HuntersGameWorldManagerLog)

AHuntersGameWorldManager::AHuntersGameWorldManager()
{
	PlayerCoords = {-27.152f,-48.918f};
}

void AHuntersGameWorldManager::BeginPlay()
{
	Super::BeginPlay();
	
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->GetPawn())
	{
		PlayerActor = PC->GetPawn();
		PlayerActor->SetActorLocation(FVector::Zero());
		UE_LOG(HuntersGameWorldManagerLog, Log, TEXT("BeginPlay() - Player found and assigned"));
	}
	
	GetWorld()->GetTimerManager().SetTimer(UpdateMapTimerHandle, [this]
	{
		UpdateMap();		
	}, UpdateFrequency, true);
	
	RegenerateGrid();
}


void AHuntersGameWorldManager::RegenerateGrid()
{

	OriginLongitude = PlayerCoords.Longitude;
	OriginLatitude = PlayerCoords.Latitude;
	UE_LOG(HuntersGameWorldManagerLog, Log, TEXT("RegenerateGrid() - Regenerate grid with origin lon: %f | Lat: %f "), OriginLongitude, OriginLatitude);
	
	ClearGrid();
	GenerateGrid();
}

void AHuntersGameWorldManager::ClearGrid()
{
	for (auto& Tile : LoadedTiles)
	{
		if (Tile.Value)
		{			
			Tile.Value->DestroyComponent();
		}
	}
	
	LoadedTiles.Empty();
}

void AHuntersGameWorldManager::GenerateGrid()
{

	int PlayerTileX = LongitudeToTileX(OriginLongitude, Zoom);
	int PlayerTileY = LatitudeToTileY(OriginLatitude, Zoom);

	for (int32 x = -LoadRadius; x <= LoadRadius; ++x)
	{
		for (int32 y = -LoadRadius; y <= LoadRadius; ++y)
		{
			FIntPoint TileIndex{ PlayerTileX + x, PlayerTileY + y };
			UE_LOG(HuntersGameWorldManagerLog, Log, TEXT("GenerateGrid() - Tile Index: %s"), *TileIndex.ToString());
			UE_LOG(HuntersGameWorldManagerLog, Log, TEXT("GenerateGrid() - Player tile - X: %i | Y: %i"), PlayerTileX, PlayerTileY);

			if (!LoadedTiles.Contains(TileIndex))
			{
				int32 Index = (y + LoadRadius) * (2 * LoadRadius + 1) + (x + LoadRadius);
				FString TileName = FString::Printf(TEXT("Tile-%i"), Index);
				UE_LOG(HuntersGameWorldManagerLog, Log, TEXT("GenerateGrid() - Tile Name: %s"), *TileName);
				
				FActorSpawnParameters ActorSpawnParameters{};
				// ActorSpawnParameters.Name = *TileName;
				AActor* PlaneActorInstanced = GetWorld()->SpawnActor<AHuntersGamePlaneActor>(
					PlaneActorClass.Get(),
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					ActorSpawnParameters);

				AHuntersGamePlaneActor* PlaneActor = Cast<AHuntersGamePlaneActor>(PlaneActorInstanced);
				PlaneActor->SetActorEnableCollision(false);
				
				if (!PlaneActorInstanced || !PlaneActor) return;
				
				PlaneActor->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
				
				// Convert tile position to Unreal coordinates
				FVector2D TileCoords = TileToLatLon(TileIndex.X, TileIndex.Y);
				UE_LOG(HuntersGameWorldManagerLog, Log, TEXT("GenerateGrid() - Tile Coords: X:%f | Y:%f "), TileCoords.X, TileCoords.Y);
				
				FVector TilePosition = TileToUnrealPosition(TileIndex.X, TileIndex.Y, PlayerTileX, PlayerTileY);
				UE_LOG(HuntersGameWorldManagerLog, Log, TEXT("GenerateGrid() - Tile Position: X:%f | Y:%f "), TilePosition.X, TilePosition.Y);
				PlaneActor->SetActorLocation(TilePosition);

				// Load OSM tile texture
				FString TileURL = GetOSMTextureURL(TileIndex.X, TileIndex.Y, Zoom);
				UE_LOG(HuntersGameWorldManagerLog, Log, TEXT("GenerateGrid() - Tile URL: %s "), *TileURL);
				DownloadAndApplyTexture(PlaneActor->GetPlaneMesh(), TileURL);

				LoadedTiles.Add(TileIndex, PlaneActor->GetPlaneMesh());
			}
		}
	}
}

FString AHuntersGameWorldManager::GetOSMTextureURL(int32 x, int32 y, int32 zoom)
{
	// return FString::Printf(TEXT("https://tile.openstreetmap.org/%d/%d/%d.png"), zoom, x, y);
	return FString::Printf(TEXT("https://api.mapbox.com/styles/v1/kayorx1/cm7bfvxlp008a01s08bef17qu/tiles/%d/%d/%d@2x?access_token=pk.eyJ1Ijoia2F5b3J4MSIsImEiOiJjbTZ6dXpqZWQwOHNrMm1wcDBrNDhwM2VwIn0.g_VZaf3bWgpQbsgnUuF2Yw"), zoom, x, y);
}

void AHuntersGameWorldManager::DownloadAndApplyTexture(UStaticMeshComponent* TileMesh, FString URL)
{
	if (!TileMesh)
	{
		UE_LOG(HuntersGameWorldManagerLog, Warning, TEXT("TileMesh is NULL!"));
		return;
	}

	// Check if the mesh has a material, if not create one dynamically
	if (TileMesh->GetNumMaterials() == 0 || !BaseMaterial)
	{
		UE_LOG(HuntersGameWorldManagerLog, Warning, TEXT("TileMesh has no material slots or BaseMaterial is missing!"));

		if (!BaseMaterial)
		{
			UE_LOG(HuntersGameWorldManagerLog, Error, TEXT("BaseMaterial is NULL! Please assign it in the Editor."));
			return;
		}

		TileMesh->SetMaterial(0, BaseMaterial);
		UE_LOG(HuntersGameWorldManagerLog, Warning, TEXT("BaseMaterial assigned to TileMesh."));
	}

	// Now create a Dynamic Material Instance from the existing material
	UMaterialInstanceDynamic* MaterialInstance = TileMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, BaseMaterial);
	if (!MaterialInstance)
	{
		UE_LOG(HuntersGameWorldManagerLog, Error, TEXT("Failed to create dynamic material instance."));
		return;
	}
	
	// Generate a filename from the URL (e.g., hash it)
	FString FileName = FMD5::HashAnsiString(*URL) + ".png";  
	FString FilePath = FPaths::ProjectSavedDir() / "CachedTextures" / FileName;

	// Check if file already exists
	if (FPaths::FileExists(FilePath))
	{
		UE_LOG(HuntersGameWorldManagerLog, Warning, TEXT("Loading texture from cache: %s"), *FilePath);
		UTexture2D* CachedTexture = FImageUtils::ImportFileAsTexture2D(FilePath);
		if (CachedTexture)
		{
			MaterialInstance->SetTextureParameterValue("BaseTexture", CachedTexture);
			return;  // Exit early, no need to download
		}
	}	
		
	// If not cached, download and save it
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindLambda([TileMesh, MaterialInstance, FilePath](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (bWasSuccessful && Response.IsValid())
		{
			TArray<uint8> ImageData = Response->GetContent();
			UTexture2D* DownloadedTexture = FImageUtils::ImportBufferAsTexture2D(ImageData);
			if (DownloadedTexture)
			{
				MaterialInstance->SetTextureParameterValue("BaseTexture", DownloadedTexture);
				UE_LOG(HuntersGameWorldManagerLog, Warning, TEXT("Texture applied successfully."));

				// Save the downloaded image for future use
				FFileHelper::SaveArrayToFile(ImageData, *FilePath);
				UE_LOG(HuntersGameWorldManagerLog, Warning, TEXT("Texture cached at: %s"), *FilePath);
			}
			else
			{
				UE_LOG(HuntersGameWorldManagerLog, Error, TEXT("Failed to create texture from downloaded data."));
			}
		}
	});

	Request->SetURL(URL);
	Request->SetVerb("GET");
	Request->ProcessRequest();
}

int32 AHuntersGameWorldManager::LongitudeToTileX(float Lon, int32 Zoom)
{
	return (Lon + 180.0) / 360.0 * pow(2.0, Zoom);
}

int32 AHuntersGameWorldManager::LatitudeToTileY(float Lat, int32 ZoomValue)
{
	float LatRad = Lat * PI / 180.0;
	return (1.0 - log(tan(LatRad) + 1.0 / cos(LatRad)) / PI) / 2.0 * pow(2.0, ZoomValue);
}

FVector AHuntersGameWorldManager::TileXYToUnreal(int32 TileX, int32 TileY)
{
	// Convert TileX, TileY to lat/lon
	FVector2D TileLatLon = TileToLatLon(TileX, TileY);

	// Convert the lat/lon to Unreal Engine world coordinates relative to origin
	return ConvertLatLonToUECoords(TileLatLon.Y, TileLatLon.X, OriginLatitude, OriginLongitude);
}

void AHuntersGameWorldManager::SetPlayerCoords(float Lat, float Lon)
{
	PlayerCoords = {Lat, Lon};
}

FVector AHuntersGameWorldManager::GeoCoordsToUnrealWorld(FVector2D Coords) const
{
	const FVector OriginUnrealLocation{0,0,0};

	const FVector2D OriginGeoCoords{0.0, 0.0};

	int32 TargetTileX = LongitudeToTileX(Coords.X, Zoom);
	int32 TargetTileY = LatitudeToTileY(Coords.Y, Zoom);

	int32 OriginTileX = LongitudeToTileX(OriginGeoCoords.X, Zoom);
	int32 OriginTileY = LatitudeToTileY(OriginGeoCoords.Y, Zoom);

	// Convert tile difference to Unreal world offset
	FVector UnrealOffset = FVector(
		(TargetTileX - OriginTileX) * TileSize,
		(TargetTileY - OriginTileY) * TileSize,
		0.0f // Assuming a flat world
	);

	return OriginUnrealLocation + UnrealOffset;
}

void AHuntersGameWorldManager::UpdateMap()
{
	
	int NewPlayerTileX = LongitudeToTileX(PlayerCoords.Longitude, Zoom);
	int NewPlayerTileY = LatitudeToTileY(PlayerCoords.Latitude, Zoom);

	static int LastPlayerTileX = NewPlayerTileX;
	static int LastPlayerTileY = NewPlayerTileY;
	
	if (NewPlayerTileX != LastPlayerTileX || NewPlayerTileY != LastPlayerTileY)
	{
		LastPlayerTileX = NewPlayerTileX;
		LastPlayerTileY = NewPlayerTileY;
        
		RegenerateGrid(); // Reload tiles around the new player position
	}
	
	// Move tiles instead of the player
	for (const auto& Tile : LoadedTiles)
	{
		if (Tile.Value)
		{
			int32 TileX = Tile.Key.X;
			int32 TileY = Tile.Key.Y;
			FVector NewTilePosition = TileToUnrealPosition(TileX, TileY, NewPlayerTileX, NewPlayerTileY);
			Tile.Value->SetWorldLocation(NewTilePosition);
			
		}
	}

	// FVector TargetLocation = ConvertLatLonToUECoords(PlayerCoords.Latitude, PlayerCoords.Longitude, OriginLongitude, OriginLatitude);
	//
	// static FVector LastLocation = TargetLocation; // Store last position
	// float DistanceMoved = FVector::Dist(LastLocation, TargetLocation);
	//
	// // Only regenerate grid if moved significantly (e.g., beyond half a tile)
	// if (DistanceMoved > TileSize / 2)
	// {
	// 	LastLocation = TargetLocation;
	// 	RegenerateGrid(); // Load new tiles
	// }
	//
	// for (const auto& Tile : LoadedTiles)
	// {
	// 	if (Tile.Value)
	// 	{
	// 		FVector CurrentPosition = Tile.Value->GetComponentLocation();
	// 		const FVector SmoothedPosition = FMath::Lerp(CurrentPosition, CurrentPosition - TargetLocation, 0.1f); // Adjust smoothing factor as needed
	// 		Tile.Value->SetWorldLocation(SmoothedPosition);
	// 	}
	// }
}

void AHuntersGameWorldManager::UnloadTiles(int32 TileX, int32 TileY)
{
	FIntPoint TileKey{TileX, TileY};

	if (LoadedTiles.Contains(TileKey))
	{
		UStaticMeshComponent* TileMesh = LoadedTiles[TileKey];
		if (TileMesh)
		{
			TileMesh->DestroyComponent();  // Removes from scene
		}

		LoadedTiles.Remove(TileKey);
		UE_LOG(LogTemp, Log, TEXT("Unloaded tile (%d, %d)"), TileKey.X, TileKey.Y);
	}
}

FVector AHuntersGameWorldManager::ConvertLatLonToUECoords(double Latitude, double Longitude, double InOriginLatitude,
	double InOriginLongitude)
{	
	const double EarthRadius = 6378137.0; // Meters

	// Convert degrees to radians
	double LatRad = FMath::DegreesToRadians(Latitude);
	double LonRad = FMath::DegreesToRadians(Longitude);
	double OriginLatRad = FMath::DegreesToRadians(OriginLatitude);
	double OriginLonRad = FMath::DegreesToRadians(OriginLongitude);

	// Calculate the x/y offsets in meters
	double X = EarthRadius * (LonRad - OriginLonRad) * FMath::Cos(OriginLatRad);
	double Y = EarthRadius * (LatRad - OriginLatRad);

	// Apply scale factor to convert meters -> Unreal units
	float ScaleFactor = 0.01f; // Adjust as needed for world scale
	return FVector(X * ScaleFactor, -Y * ScaleFactor, 0.0f);
}


float AHuntersGameWorldManager::CalculateDistance(float InitialLat, float InitialLon, float FinalLat, float FinalLon)
{
	// Convert degrees to radians
	float RadLat1 = FMath::DegreesToRadians(InitialLat);
	float RadLon1 = FMath::DegreesToRadians(InitialLon);
	float RadLat2 = FMath::DegreesToRadians(FinalLat);
	float RadLon2 = FMath::DegreesToRadians(FinalLon);

	// Radius of Earth in meters
	const float EarthRadius = 6371000.0f;
	
	// Haversine formula
	float DeltaLat = RadLat2 - RadLat1;
	float DeltaLon = RadLon2 - RadLon1;

	float A = FMath::Sin(DeltaLat / 2) * FMath::Sin(DeltaLat / 2) +
		  FMath::Cos(RadLat1) * FMath::Cos(RadLat2) *
		  FMath::Sin(DeltaLon / 2) * FMath::Sin(DeltaLon / 2);

	float C = 2 * FMath::Atan2(FMath::Sqrt(A), FMath::Sqrt(1 - A));

	return EarthRadius * C; // Distance in meters
}

// float AHuntersGameWorldManager::GetGPSVelocity()
// {
// 	if (ULocationServicesImpl* LocationServicesImpl = ULocationServices::GetLocationServicesImpl())
// 	{
// 		FLocationServicesData CurrentLocation = LocationServicesImpl->GetLastKnownLocation();
// 		if (bHasPreviousLocation)
// 		{
// 			float Distance = CalculateDistance(PreviousLocation.Latitude, PreviousLocation.Longitude,
// 				CurrentLocation.Latitude, CurrentLocation.Longitude);
//
// 			float TimeDelta = (CurrentLocation.Timestamp - PreviousLocation.Timestamp) / 1000.0f; // Convert ms to seconds
//
// 			if (TimeDelta > 0)
// 			{
// 				float Speed = Distance / TimeDelta; // Speed in meters per second
// 				PreviousLocation = CurrentLocation;
// 				return Speed;
// 			}
// 		}
// 		// Store first location
// 		PreviousLocation = CurrentLocation;
// 		bHasPreviousLocation = true;
// 	}
// 	return 0.0f; // Default if no data
// }

FVector2D AHuntersGameWorldManager::TileToLatLon(int32 TileX, int32 TileY)
{
	const double n = FMath::Pow(2.0, Zoom);
	
	double Lon = (TileX + 0.5) / n * 360.0 - 180.0;

	double Y = 1.0 - (2.0 * (TileY + 0.5) /n);
	const double Lat = FMath::RadiansToDegrees(FMath::Atan(FMath::Sinh(PI * Y)));

	return FVector2D(Lon, Lat);
}

// FVector AHuntersGameWorldManager::TileToUnrealPosition(int32 TileX, int32 TileY)
// {
// 	const double n = FMath::Pow(2.0, Zoom);
//
// 	// Top-left corner latitude and longitude
// 	double LonTopLeft = (TileX / n) * 360.0 - 180.0;
// 	double LatTopLeft = FMath::RadiansToDegrees(FMath::Atan(FMath::Sinh(PI * (1 - 2 * TileY / n))));
//
// 	// Center of the tile
// 	double LonCenter = ((TileX + 0.5) / n) * 360.0 - 180.0;
// 	double LatCenter = FMath::RadiansToDegrees(FMath::Atan(FMath::Sinh(PI * (1 - 2 * (TileY + 0.5) / n))));
//
// 	// Convert to Unreal Engine coordinates (assume 1 tile = TileSizeUU units)
// 	const float XPos = TileX * TileSize;
// 	const float YPos = -TileY * TileSize;  // Flip Y since OSM has (0,0) at the top-left, but UE has (0,0) at bottom-left
//
// 	return FVector(XPos, YPos, 0.0f);  // Z = 0 (assuming flat plane)
// }

FVector AHuntersGameWorldManager::TileToUnrealPosition(int32 TileX, int32 TileY, int32 PlayerTileX, int32 PlayerTileY)
{
	// Offset each tile so that the player's tile is at (0,0,0)
	int32 RelativeTileX = TileX - PlayerTileX;
	int32 RelativeTileY = TileY - PlayerTileY;

	float XPos = (RelativeTileX + 0.5f) * TileSize;
	float YPos = -(RelativeTileY + 0.5f) * TileSize; // Flip Y for UE coordinate system

	return FVector(XPos, YPos, 0.0f);  // Flat plane, so Z = 0
}
