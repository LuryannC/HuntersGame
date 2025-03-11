#include "HuntersGameWorldManager.h"

#include "HttpModule.h"
#include "HuntersGamePlaneActor.h"
#include "ImageUtils.h"
#include "LocationServicesBPLibrary.h"
#include "LocationServicesImpl.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"

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
	}
	
	RegenerateGrid();

	// GetWorld()->GetTimerManager().SetTimer(UpdateMapTimerHandle, [this]
	// {
	// 	UpdateMap();		
	// }, UpdateFrequency, true);
}


void AHuntersGameWorldManager::RegenerateGrid()
{

	OriginLongitude = PlayerCoords.Longitude;
	OriginLatitude = PlayerCoords.Latitude;
	
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

			if (!LoadedTiles.Contains(TileIndex))
			{
				// UStaticMeshComponent* TileMesh = NewObject<UStaticMeshComponent>(this);
				// TileMesh->SetStaticMesh(PlaneMesh);
				// TileMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);

				int32 Index = (y + LoadRadius) * (2 * LoadRadius + 1) + (x + LoadRadius);
				FString TileName = FString::Printf(TEXT("Tile-%i"), Index);
				FActorSpawnParameters ActorSpawnParameters{};
				ActorSpawnParameters.Name = *TileName;
				AActor* PlaneActorInstanced = GetWorld()->SpawnActor<AHuntersGamePlaneActor>(
					PlaneActorClass.Get(),
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					ActorSpawnParameters);

				AHuntersGamePlaneActor* PlaneActor = Cast<AHuntersGamePlaneActor>(PlaneActorInstanced);
				
				if (!PlaneActorInstanced || !PlaneActor) return;
				
				PlaneActor->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
				
				// Convert tile position to Unreal coordinates
				FVector2D TileCoords = TileToLatLon(TileIndex.X, TileIndex.Y);
				FVector TilePosition = ConvertLatLonToUECoords(TileCoords.Y, TileCoords.X, OriginLatitude, OriginLongitude);

				// Load OSM tile texture
				FString TileURL = GetOSMTextureURL(TileIndex.X, TileIndex.Y, Zoom);
				DownloadAndApplyTexture(PlaneActor->GetPlaneMesh(), TileURL);

				LoadedTiles.Add(TileIndex, PlaneActor->GetPlaneMesh());
			}
		}
	}
	
	// UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	//
	// if (!PlaneMesh) return;
	//
	// int PlayerTileX = LongitudeToTileX(PlayerCoords.Longitude, Zoom);
	// int PlayerTileY = LatitudeToTileY(PlayerCoords.Latitude, Zoom);
	//
	// for (int32 x = -LoadRadius; x <= LoadRadius; ++x)
	// {
	// 	for (int32 y = -LoadRadius; y <= LoadRadius; ++y)
	// 	{
	// 		FIntPoint TileIndex{PlayerTileX + x,  PlayerTileY + y};
	// 		
	// 		if (!LoadedTiles.Contains(TileIndex))
	// 		{				
	// 			UStaticMeshComponent* TileMesh = NewObject<UStaticMeshComponent>(this);
	// 			TileMesh->SetStaticMesh(PlaneMesh);
	// 			TileMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	//
	// 			FVector TilePosition = GetActorLocation() + FVector(x * TileSize, y * TileSize, 0.0f);
	//
	// 			const FVector2D TileCoords = TileXYToLatLon(TileIndex.X, TileIndex.Y, Zoom);
	// 			const FVector TileUnrealLocation = GeoCoordsToUnrealWorld(TileCoords);
	// 		
	// 			TileMesh->SetWorldLocation(TilePosition);
	// 		
	// 			//TileMesh->SetWorldScale3D(FVector{25.0f, 25.0f, 1.0f});
	// 			TileMesh->RegisterComponent();
	//
	// 			int NewTileX = PlayerTileX + x;
	// 			int NewTileY = PlayerTileY + y;
	// 		
	// 			FString TileURL = GetOSMTextureURL(NewTileX, NewTileY, Zoom);
	// 			DownloadAndApplyTexture(TileMesh, TileURL);
	//
	// 			LoadedTiles.Add(TileIndex, TileMesh);
	// 		}
	// 	}
	// }
	//
	// const FVector NewLocation = ConvertLatLonToUECoords(PlayerCoords.Latitude, PlayerCoords.Longitude, OriginLongitude, OriginLatitude);
	// SetActorLocation(NewLocation);
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
		UE_LOG(LogTemp, Warning, TEXT("TileMesh is NULL!"));
		return;
	}

	// Check if the mesh has a material, if not create one dynamically
	if (TileMesh->GetNumMaterials() == 0 || !BaseMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("TileMesh has no material slots or BaseMaterial is missing!"));

		if (!BaseMaterial)
		{
			UE_LOG(LogTemp, Error, TEXT("BaseMaterial is NULL! Please assign it in the Editor."));
			return;
		}

		TileMesh->SetMaterial(0, BaseMaterial);
		UE_LOG(LogTemp, Warning, TEXT("BaseMaterial assigned to TileMesh."));
	}

	// Now create a Dynamic Material Instance from the existing material
	UMaterialInstanceDynamic* MaterialInstance = TileMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, BaseMaterial);
	if (!MaterialInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create dynamic material instance."));
		return;
	}
	
	// Generate a filename from the URL (e.g., hash it)
	FString FileName = FMD5::HashAnsiString(*URL) + ".png";  
	FString FilePath = FPaths::ProjectSavedDir() / "CachedTextures" / FileName;

	// Check if file already exists
	if (FPaths::FileExists(FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Loading texture from cache: %s"), *FilePath);
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
				UE_LOG(LogTemp, Warning, TEXT("Texture applied successfully."));

				// Save the downloaded image for future use
				FFileHelper::SaveArrayToFile(ImageData, *FilePath);
				UE_LOG(LogTemp, Warning, TEXT("Texture cached at: %s"), *FilePath);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create texture from downloaded data."));
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
	// if (!PlayerActor)
	// {
	// 	UE_LOG(LogTemp, Log, TEXT("Invalid Player Actor"));
	// 	return;
	// }
	
	int PlayerTileX = LongitudeToTileX(PlayerCoords.Longitude, Zoom);
	int PlayerTileY = LatitudeToTileY(PlayerCoords.Latitude, Zoom);

	FVector TargetLocation = ConvertLatLonToUECoords(PlayerCoords.Latitude, PlayerCoords.Longitude, OriginLongitude, OriginLatitude);
	
	static FVector LastLocation = TargetLocation; // Store last position
	float DistanceMoved = FVector::Dist(LastLocation, TargetLocation);
	
	// Only regenerate grid if moved significantly (e.g., beyond half a tile)
	if (DistanceMoved > TileSize / 2)
	{
		LastLocation = TargetLocation;
		RegenerateGrid(); // Load new tiles
	}
	
	for (const auto& Tile : LoadedTiles)
	{
		if (Tile.Value)
		{
			FVector CurrentPosition = Tile.Value->GetComponentLocation();
			const FVector SmoothedPosition = FMath::Lerp(CurrentPosition, CurrentPosition - TargetLocation, 0.1f); // Adjust smoothing factor as needed
			Tile.Value->SetWorldLocation(SmoothedPosition);
		}
	}

	// UE_LOG(LogTemp, Log, TEXT("Player at Tile (%d, %d)"), PlayerTileX, PlayerTileY);
	//
	// GenerateGrid();
	//
	// TArray<FIntPoint> TilesToRemove;
	// for (const auto& Tile : LoadedTiles)
	// {
	// 	int TileX = Tile.Key.X;
	// 	int TileY = Tile.Key.Y;
	// 	
	// 	if (FMath::Abs(TileX - PlayerTileX) > LoadRadius || FMath::Abs(TileY - PlayerTileY) > LoadRadius)
	// 	{
	// 		TilesToRemove.Add(Tile.Key);
	// 	}
	// }
	
	// FVector UnrealLocation = GeoCoordsToUnrealWorld(FVector2D{PlayerCoords.Longitude, PlayerCoords.Latitude});
	// UE_LOG(LogTemp, Log, TEXT("Player at Unreal Location (%f, %f)"), UnrealLocation.X, UnrealLocation.Y);
	//
		
	// for (FIntPoint TileKey : TilesToRemove)
	// {
	// 	UnloadTiles(TileKey.X, TileKey.Y);
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
	// double EarthRadius = 6378137.0; // Earth's radius in meters
	// double ScaleFactor = 10.0; // Convert real-world meters to Unreal Units (adjust as needed)
	//
	// // Convert latitude and longitude to meters relative to the origin point
	// double DeltaLat = (Latitude - InOriginLatitude) * (PI / 180.0) * EarthRadius;
	// double DeltaLon = (Longitude - InOriginLongitude) * (PI / 180.0) * EarthRadius * cos(InOriginLatitude * PI / 180.0);
	//
	// return FVector(DeltaLon, DeltaLat, 0.0) * ScaleFactor;
	
	const double EarthRadius = 6378137.0;
	
	double LatRad = FMath::DegreesToRadians(Latitude);
	double LonRad = FMath::DegreesToRadians(Longitude);
	double OriginLatRad = FMath::DegreesToRadians(InOriginLatitude);
	double OriginLonRad = FMath::DegreesToRadians(InOriginLongitude);
	
	double X = EarthRadius * (LonRad - OriginLonRad);
	double Y = EarthRadius * (LatRad - OriginLatRad);
	
	return FVector(X, Y, 0.0f);
}

//
// void AHuntersGameWorldManager::DownloadTileImage(int32 TileX, int32 TileY)
// {
// 	FString TileURL = FString::Printf(TEXT("https://tile.openstreetmap.org/%d/%d/%d.png"), Zoom, TileX, TileY);
//
// 	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
// 	HttpRequest->SetURL(TileURL);
// 	HttpRequest->SetVerb("GET");
//
// 	// Capture TileX and TileY using a lambda function
// 	HttpRequest->OnProcessRequestComplete().BindLambda([this, TileX, TileY](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
// 	{
// 		if (bWasSuccessful && Response.IsValid())
// 		{
// 			OnImageDownloaded(TileX, TileY, Response->GetContent());
// 		}
// 	});
// 	
// 	HttpRequest->ProcessRequest();
// 	
// }
//
// void AHuntersGameWorldManager::OnImageDownloaded(int32 TileX, int32 TileY, TArray<uint8> ImageData)
// {
// 	if (!LoadedTiles.Contains(FIntPoint(TileX, TileY)))
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Downloaded image for (%d, %d) but tile does not exist in LoadedTiles!"), TileX, TileY);
// 		return;
// 	}
//
// 	if (ImageData.Num() == 0)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Failed to download image for tile (%d, %d), empty response!"), TileX, TileY);
// 		return;
// 	}
//
// 	UTexture2D* TileTexture = FImageUtils::ImportBufferAsTexture2D(ImageData);
// 	if (!TileTexture)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Failed to create texture from downloaded data for tile (%d, %d)"), TileX, TileY);
// 		return;
// 	}
// 	
// 	if(!LoadedTiles[FIntPoint(TileX, TileY)])
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Invalid tile (%d, %d)"), TileX, TileY);
// 		return;
// 	}
//
//
// 	UMaterialInstanceDynamic* TileMaterial = UMaterialInstanceDynamic::Create(LoadedTiles[FIntPoint(TileX, TileY)]->GetMaterial(0), this);
// 	TileMaterial->SetTextureParameterValue("BaseColorTexture", TileTexture);
// 	
// 	LoadedTiles[FIntPoint(TileX, TileY)]->SetMaterial(0, TileMaterial);
// 	
//
// 	UE_LOG(LogTemp, Log, TEXT("Successfully applied texture to tile (%d, %d)"), TileX, TileY);
// 	
// 	// bool bHasData = ImageData.Num() == 0;
// 	// bool bHasDownload = LoadedTiles.Contains(FIntPoint(TileX, TileY));
// 	//
// 	// if (bHasData || !bHasDownload)
// 	// {
// 	// 	UE_LOG(LogTemp, Warning, TEXT("Failed to download image for tile %d, %d"), TileX, TileY);
// 	// 	return;
// 	// }
// 	//
// 	// // Convert image data to Texture2D
// 	// UTexture2D* TileTexture = FImageUtils::ImportBufferAsTexture2D(ImageData);
// 	// if (TileTexture)
// 	// {
// 	// 	UMaterialInstanceDynamic* TileMaterial = UMaterialInstanceDynamic::Create(LoadedTiles[FIntPoint(TileX, TileY)]->GetMaterial(0), this);
// 	// 	TileMaterial->SetTextureParameterValue("BaseColorTexture", TileTexture);
// 	// 	LoadedTiles[FIntPoint(TileX, TileY)]->SetMaterial(0, TileMaterial);
// 	// }
// }
//
// FVector AHuntersGameWorldManager::GetTileWorldPosition(int32 TileX, int32 TileY)
// {
// 	return FVector(TileX * TileSize, TileY * TileSize, 0.0f);
// }
//
// FPlayerTileLocation AHuntersGameWorldManager::GetPlayerTile()
// {
// 	int32 PlayerTileX = LongitudeToTileX(PlayerCoords.Longitude, Zoom);
// 	int32 PlayerTileY = LatitudeToTileY(PlayerCoords.Latitude, Zoom);
//
// 	return {PlayerTileX, PlayerTileY};
// }
//
// void AHuntersGameWorldManager::ConvertWorldToGPS(FVector WorldLocation, float& OutLat, float& OutLon)
// {
// 	float Scale = 0.0001f; // Adjust scale to match your map size
// 	OutLat = PlayerCoords.Latitude + (WorldLocation.Y * TileSize);
// 	OutLon = PlayerCoords.Longitude + (WorldLocation.X * TileSize);
// }


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

float AHuntersGameWorldManager::GetGPSVelocity()
{
	if (ULocationServicesImpl* LocationServicesImpl = ULocationServices::GetLocationServicesImpl())
	{
		FLocationServicesData CurrentLocation = LocationServicesImpl->GetLastKnownLocation();
		if (bHasPreviousLocation)
		{
			float Distance = CalculateDistance(PreviousLocation.Latitude, PreviousLocation.Longitude,
				CurrentLocation.Latitude, CurrentLocation.Longitude);

			float TimeDelta = (CurrentLocation.Timestamp - PreviousLocation.Timestamp) / 1000.0f; // Convert ms to seconds

			if (TimeDelta > 0)
			{
				float Speed = Distance / TimeDelta; // Speed in meters per second
				PreviousLocation = CurrentLocation;
				return Speed;
			}
		}
		// Store first location
		PreviousLocation = CurrentLocation;
		bHasPreviousLocation = true;
	}
	return 0.0f; // Default if no data
}

FVector2D AHuntersGameWorldManager::TileToLatLon(int32 TileX, int32 TileY)
{
	const double n = FMath::Pow(2.0, Zoom);
	
	double Lon = (TileX + 0.5) / n * 360.0 - 180.0;

	double Y = 1.0 - (2.0 * (TileY + 0.5) /n);
	const double Lat = FMath::RadiansToDegrees(FMath::Atan(FMath::Sinh(PI * Y)));

	return FVector2D(Lon, Lat);
}

FVector AHuntersGameWorldManager::TileToUnrealPosition(int32 TileX, int32 TileY)
{
	const double n = FMath::Pow(2.0, Zoom);

	// Top-left corner latitude and longitude
	double LonTopLeft = (TileX / n) * 360.0 - 180.0;
	double LatTopLeft = FMath::RadiansToDegrees(FMath::Atan(FMath::Sinh(PI * (1 - 2 * TileY / n))));

	// Center of the tile
	double LonCenter = ((TileX + 0.5) / n) * 360.0 - 180.0;
	double LatCenter = FMath::RadiansToDegrees(FMath::Atan(FMath::Sinh(PI * (1 - 2 * (TileY + 0.5) / n))));

	// Convert to Unreal Engine coordinates (assume 1 tile = TileSizeUU units)
	const float XPos = TileX * TileSize;
	const float YPos = -TileY * TileSize;  // Flip Y since OSM has (0,0) at the top-left, but UE has (0,0) at bottom-left

	return FVector(XPos, YPos, 0.0f);  // Z = 0 (assuming flat plane)
}
