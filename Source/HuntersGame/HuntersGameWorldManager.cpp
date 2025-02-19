#include "HuntersGameWorldManager.h"

#include "HttpModule.h"
#include "ImageUtils.h"
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
	if (PC)
	{
		PlayerActor = PC->GetPawn();
	}

	// GetWorld()->GetTimerManager().SetTimer(UpdateMapTimerHandle, [this]
	// {
	// 	UpdateMap();		
	// }, UpdateFrequency, true);
}


void AHuntersGameWorldManager::RegenerateGrid()
{
	ClearGrid();
	GenerateGrid();
}

void AHuntersGameWorldManager::ClearGrid()
{
	
	for (UStaticMeshComponent* Tile : TileMeshes)
	{
		if (Tile)
		{
			Tile->DestroyComponent();
		}
	}
}

void AHuntersGameWorldManager::GenerateGrid()
{
	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));

	if (!PlaneMesh) return;

	int PlayerTileX = LongitudeToTileX(PlayerCoords.Longitude, Zoom);
	int PlayerTileY = LatitudeToTileY(PlayerCoords.Latitude, Zoom);
	
	for (int32 x = -LoadRadius; x <= LoadRadius; ++x)
	{
		for (int32 y = -LoadRadius; y <= LoadRadius; ++y)
		{
			UStaticMeshComponent* TileMesh = NewObject<UStaticMeshComponent>(this);
			TileMesh->SetStaticMesh(PlaneMesh);
			TileMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);

			FVector TilePosition = GetActorLocation() + FVector(x * TileSize, y * TileSize, 0.0f);
			
			TileMesh->SetWorldLocation(TilePosition);
			
			TileMesh->SetWorldScale3D(FVector{25.0f, 25.0f, 1.0f});
			TileMesh->RegisterComponent();

			int NewTileX = PlayerTileX + x;
			int NewTileY = PlayerTileY + y;
			
			FString TileURL = GetOSMTextureURL(NewTileX, NewTileY, Zoom);
			DownloadAndApplyTexture(TileMesh, TileURL);

			TileMeshes.Add(TileMesh);
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
	
	// Download and apply texture
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindLambda([TileMesh, MaterialInstance](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (bWasSuccessful && Response.IsValid())
		{
			UTexture2D* DownloadedTexture = FImageUtils::ImportBufferAsTexture2D(Response->GetContent());
			if (DownloadedTexture)
			{
				MaterialInstance->SetTextureParameterValue("BaseTexture", DownloadedTexture);
				UE_LOG(LogTemp, Warning, TEXT("Texture applied successfully."));
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

int32 AHuntersGameWorldManager::LatitudeToTileY(float Lat, int32 Zoom)
{
	float LatRad = Lat * PI / 180.0;
	return (1.0 - log(tan(LatRad) + 1.0 / cos(LatRad)) / PI) / 2.0 * pow(2.0, Zoom);
}

//
//
// void AHuntersGameWorldManager::SetPlayerCoords(float Lat, float Lon)
// {
// 	PlayerCoords = {Lat, Lon};
// }
//
// void AHuntersGameWorldManager::UpdateMap()
// {
// 	if (!PlayerActor)
// 	{
// 		UE_LOG(LogTemp, Log, TEXT("Invalid Player Actor"));
// 		return;
// 	}
// 	
// 	FVector PlayerLocation = PlayerActor->GetActorLocation();  // Get player's position in world space
// 	float PlayerLat, PlayerLon;
// 	ConvertWorldToGPS(PlayerLocation, PlayerLat, PlayerLon);
// 	
// 	int PlayerTileX = LongitudeToTileX(PlayerCoords.Longitude, Zoom);
// 	int PlayerTileY = LatitudeToTileY(PlayerCoords.Latitude, Zoom);
//
// 	UE_LOG(LogTemp, Log, TEXT("Player at Tile (%d, %d)"), PlayerTileX, PlayerTileY);
//
// 	for (int dx = -LoadRadius; dx <= LoadRadius; dx++)
// 	{
// 		for (int dy = -LoadRadius; dy <= LoadRadius; dy++)
// 		{
// 			int NewTileX = PlayerTileX + dx;
// 			int NewTileY = PlayerTileY + dy;
// 			FIntPoint NewTilePos(NewTileX, NewTileY);
//
// 			if (!LoadedTiles.Contains(NewTilePos))
// 			{
// 				LoadTile(NewTileX, NewTileY);
// 			}
// 		}
// 	}
//
// 	TArray<FIntPoint> KeysToRemove;
// 	for (auto& Tile : LoadedTiles)
// 	{
// 		if (!LoadedTiles.Contains(Tile.Key))
// 		{
// 			KeysToRemove.Add(Tile.Key);
// 		}
// 	}
//
// 	for (FIntPoint TileToRemove : KeysToRemove)
// 	{
// 		UnloadTiles(TileToRemove.X, TileToRemove.Y);
// 	}
// 	
// 	// for (int dx = -LoadRadius; dx <= LoadRadius; dx++)
// 	// {
// 	// 	for (int dy = -LoadRadius; dy <= LoadRadius; dy++)
// 	// 	{
// 	// 		int NewTileX = PlayerTileX + dx;
// 	// 		int NewTileY = PlayerTileY + dy;
// 	//
// 	// 		if (!LoadedTiles.Contains(FIntPoint(NewTileX, NewTileY)))
// 	// 		{
// 	// 			UE_LOG(LogTemp, Warning, TEXT("Tile (%d, %d) not found in LoadedTiles. Loading..."), NewTileX, NewTileY);
// 	// 			LoadTile(NewTileX, NewTileY);
// 	// 		}
// 	// 	}
// 	// }
// 	
// 	// for (int32 DirectionX = -LoadRadius; DirectionX <= LoadRadius; DirectionX++)
// 	// {
// 	// 	for (int32 DirectionY = -LoadRadius; DirectionY <= LoadRadius; DirectionY++)
// 	// 	{
// 	// 		int32 NewTileX = GetPlayerTile().TileX + DirectionX;
// 	// 		int32 NewTileY = GetPlayerTile().TileY + DirectionY;
// 	//
// 	// 		if (!LoadedTiles.Contains(FIntPoint(NewTileX, NewTileY)))
// 	// 		{
// 	// 			LoadTile(NewTileX, NewTileY);
// 	// 		}
// 	// 	}
// 	// }
// 	//
// 	// TArray<FIntPoint> TilesToRemove;
// 	// for (const auto& Tile : LoadedTiles)
// 	// {
// 	// 	int TileX = Tile.Key.X;
// 	// 	int TileY = Tile.Key.Y;
// 	//
// 	// 	if (FMath::Abs(TileX - PlayerTileLocation.TileX) > LoadRadius || FMath::Abs(TileY - PlayerTileLocation.TileY) > LoadRadius)
// 	// 	{
// 	// 		TilesToRemove.Add(Tile.Key);
// 	// 	}
// 	// }
// 	//
// 	// for (FIntPoint TileKey : TilesToRemove)
// 	// {
// 	// 	UnloadTiles(TileKey.X, TileKey.Y);
// 	// }
// }
//
// void AHuntersGameWorldManager::LoadTile(int32 TileX, int32 TileY)
// {
// 	FIntPoint Index = FIntPoint(TileX, TileY);
// 	
// 	if (LoadedTiles.Contains(Index))
// 	{
// 		return; // Tile already exists
// 	}
//
// 	UStaticMeshComponent* TileMesh = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass());
// 	if (!TileMesh)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Failed to create tile mesh for (%d, %d)"), TileX, TileY);
// 		return;
// 	}
//
// 	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
// 	if (PlaneMesh)
// 	{
// 		TileMesh->SetStaticMesh(PlaneMesh);
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Failed to load Plane mesh for tile (%d, %d)"), TileX, TileY);
// 	}
// 	
// 	TileMesh->SetWorldLocation(GetActorLocation());
// 	TileMesh->SetWorldScale3D(FVector{2.0f, 2.0f, 1.0f});
//
// 	TileMesh->SetFlags(RF_Transactional);
// 	TileMesh->SetMobility(EComponentMobility::Movable);
// 	TileMesh->SetHiddenInGame(false);
// 	
// 	TileMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
// 	TileMesh->RegisterComponent(); 
//
// 	LoadedTiles.Add(Index, TileMesh);
//
// 	UE_LOG(LogTemp, Log, TEXT("Tile (%d, %d) successfully added"), TileX, TileY);
//
// 	DownloadTileImage(TileX, TileY);
// }
//
// void AHuntersGameWorldManager::UnloadTiles(int32 TileX, int32 TileY)
// {
// 	FIntPoint TileKey{TileX, TileY};
//
// 	if (LoadedTiles.Contains(TileKey))
// 	{
// 		UStaticMeshComponent* TileMesh = LoadedTiles[TileKey];
// 		if (TileMesh)
// 		{
// 			TileMesh->DestroyComponent();  // Removes from scene
// 		}
//
// 		LoadedTiles.Remove(TileKey);
// 		UE_LOG(LogTemp, Log, TEXT("Unloaded tile (%d, %d)"), TileKey.X, TileKey.Y);
// 	}
// 	
// 	// if (UStaticMeshComponent** MeshComponent = LoadedTiles.Find(FIntPoint(TileX, TileY)))
// 	// {
// 	// 	(*MeshComponent)->DestroyComponent();
// 	// 	LoadedTiles.Remove(FIntPoint(TileX, TileY));
// 	// }
// }
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