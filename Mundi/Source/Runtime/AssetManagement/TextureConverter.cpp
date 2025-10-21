/**
 * @file TextureConverter.cpp
 * @brief Implementation of texture conversion utilities using DirectXTex
 */

#include "pch.h"
#include "TextureConverter.h"
#include <DirectXTex.h>
#include <algorithm>

// Helper function: Convert UTF-8 string to wide string
static std::wstring UTF8ToWide(const std::string& str)
{
	if (str.empty()) return std::wstring();

	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
	                                       static_cast<int>(str.size()), NULL, 0);
	if (size_needed <= 0) return std::wstring();

	std::wstring result(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()),
	                    &result[0], size_needed);
	return result;
}

// Helper function: Convert wide string to UTF-8 string
static std::string WideToUTF8(const std::wstring& wstr)
{
	if (wstr.empty()) return std::string();

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
	                                       static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
	if (size_needed <= 0) return std::string();

	std::string result(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
	                    &result[0], size_needed, NULL, NULL);
	return result;
}

bool FTextureConverter::ConvertToDDS(
	const FString& SourcePath,
	const FString& OutputPath,
	DXGI_FORMAT Format)
{
	using namespace DirectX;

	// 1. Load source image
	std::wstring WSourcePath = UTF8ToWide(SourcePath);
	std::filesystem::path SourceFile(WSourcePath);

	if (!std::filesystem::exists(SourceFile))
	{
		UE_LOG("[TextureConverter] Source file not found: %s", SourcePath.c_str());
		return false;
	}

	TexMetadata metadata;
	ScratchImage image;

	// Load based on file extension
	std::wstring ext = SourceFile.extension().wstring();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

	HRESULT hr = E_FAIL;

	if (ext == L".dds")
	{
		// Already DDS format, no conversion needed
		return true;
	}
	else if (ext == L".tga")
	{
		hr = LoadFromTGAFile(WSourcePath.c_str(), &metadata, image);
	}
	else if (ext == L".hdr")
	{
		hr = LoadFromHDRFile(WSourcePath.c_str(), &metadata, image);
	}
	else
	{
		// Use WIC for common formats (PNG, JPG, BMP, etc.)
		hr = LoadFromWICFile(WSourcePath.c_str(), WIC_FLAGS_NONE, &metadata, image);
	}

	if (FAILED(hr))
	{
		UE_LOG("[TextureConverter] Failed to load source image: %s (HRESULT: 0x%08X)",
		       SourcePath.c_str(), hr);
		return false;
	}

	// 2. Resize to 4-pixel alignment if using block compression
	if (IsCompressed(Format))
	{
		size_t width = metadata.width;
		size_t height = metadata.height;

		// Round up to next multiple of 4
		size_t alignedWidth = (width + 3) & ~3;
		size_t alignedHeight = (height + 3) & ~3;

		if (width != alignedWidth || height != alignedHeight)
		{
			UE_LOG("[TextureConverter] Resizing %s from %dx%d to %dx%d for block compression",
			       SourcePath.c_str(), (int)width, (int)height,
			       (int)alignedWidth, (int)alignedHeight);

			ScratchImage resized;
			hr = Resize(image.GetImages(), image.GetImageCount(), metadata,
			            alignedWidth, alignedHeight, TEX_FILTER_DEFAULT, resized);

			if (SUCCEEDED(hr))
			{
				image = std::move(resized);
				metadata = image.GetMetadata();
			}
			else
			{
				UE_LOG("[TextureConverter] Warning: Resize failed, continuing with original size");
			}
		}
	}

	// 3. Generate mipmaps if needed
	ScratchImage mipChain;
	if (bShouldGenerateMipmaps && metadata.mipLevels == 1)
	{
		hr = GenerateMipMaps(image.GetImages(), image.GetImageCount(), metadata,
		                     TEX_FILTER_DEFAULT, 0, mipChain);
		if (SUCCEEDED(hr))
		{
			image = std::move(mipChain);
			metadata = image.GetMetadata();
		}
	}

	// 4. Compress to target format
	ScratchImage compressed;
	if (IsCompressed(Format))
	{
		// Fast multi-threaded compression
		hr = Compress(image.GetImages(), image.GetImageCount(), metadata,
		              Format, TEX_COMPRESS_PARALLEL | TEX_COMPRESS_DITHER,
		              TEX_THRESHOLD_DEFAULT, compressed);

		if (FAILED(hr))
		{
			UE_LOG("[TextureConverter] Compression failed: %s (HRESULT: 0x%08X)",
			       SourcePath.c_str(), hr);
			return false;
		}
	}
	else
	{
		// Uncompressed format, just convert
		compressed = std::move(image);
	}

	// 4. Determine output path
	FString FinalOutputPath = OutputPath.empty() ? GetDDSCachePath(SourcePath) : OutputPath;
	EnsureCacheDirectoryExists(FinalOutputPath);

	// 5. Save to DDS
	std::wstring WOutputPath = UTF8ToWide(FinalOutputPath);
	hr = SaveToDDSFile(compressed.GetImages(), compressed.GetImageCount(),
	                   compressed.GetMetadata(), DDS_FLAGS_NONE, WOutputPath.c_str());

	if (FAILED(hr))
	{
		UE_LOG("[TextureConverter] Failed to save DDS file: %s (HRESULT: 0x%08X)",
		       FinalOutputPath.c_str(), hr);
		return false;
	}

	UE_LOG("[TextureConverter] Successfully converted: %s -> %s",
	       SourcePath.c_str(), FinalOutputPath.c_str());
	return true;
}

bool FTextureConverter::ShouldRegenerateDDS(
	const FString& SourcePath,
	const FString& DDSPath)
{
	namespace fs = std::filesystem;

	// Check if DDS cache exists
	fs::path SourceFile(UTF8ToWide(SourcePath));
	fs::path DDSFile(UTF8ToWide(DDSPath));

	if (!fs::exists(DDSFile))
	{
		return true; // Cache doesn't exist
	}

	if (!fs::exists(SourceFile))
	{
		return false; // Source missing, use existing cache
	}

	// Compare timestamps
	auto SourceTime = fs::last_write_time(SourceFile);
	auto DDSTime = fs::last_write_time(DDSFile);

	// Regenerate if source is newer than cache
	return SourceTime > DDSTime;
}

FString FTextureConverter::GetDDSCachePath(const FString& SourcePath)
{
	namespace fs = std::filesystem;

	fs::path SourceFile(UTF8ToWide(SourcePath));
	fs::path RelativePath;

	// Try to make path relative to Data/ directory
	fs::path DataDir = fs::current_path() / "Data";

	if (SourceFile.is_absolute())
	{
		// Try to extract relative path from Data directory
		std::error_code ec;
		RelativePath = fs::relative(SourceFile, DataDir, ec);

		if (ec || RelativePath.empty() || RelativePath.string().find("..") == 0)
		{
			// File is outside Data directory, use filename only
			RelativePath = SourceFile.filename();
		}
	}
	else
	{
		// Already relative path
		RelativePath = SourceFile;
	}

	// Build cache path: Data/TextureCache/<relative_path>/<filename>.dds
	fs::path CachePath = DataDir / "TextureCache" / RelativePath;
	CachePath.replace_extension(".dds");

	return WideToUTF8(CachePath.wstring());
}

bool FTextureConverter::IsSupportedFormat(const FString& Extension)
{
	std::string ext = Extension;
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	static const std::unordered_set<std::string> SupportedFormats = {
		".png", ".jpg", ".jpeg", ".bmp", ".tga", ".hdr", ".dds", ".tif", ".tiff"
	};

	return SupportedFormats.find(ext) != SupportedFormats.end();
}

void FTextureConverter::SetGenerateMipmaps(bool bGenerateMips)
{
	bShouldGenerateMipmaps = bGenerateMips;
}

DXGI_FORMAT FTextureConverter::GetRecommendedFormat(bool bHasAlpha)
{
	// BC3 (DXT5) for textures with alpha - fast compression, good quality
	// BC1 (DXT1) for opaque textures - fastest compression, smaller size
	// BC7 available but very slow (10-20x slower than BC3)

	return bHasAlpha ? DXGI_FORMAT_BC3_UNORM : DXGI_FORMAT_BC1_UNORM;
}

void FTextureConverter::EnsureCacheDirectoryExists(const FString& CachePath)
{
	namespace fs = std::filesystem;

	fs::path Path(UTF8ToWide(CachePath));
	fs::path Directory = Path.parent_path();

	if (!Directory.empty() && !fs::exists(Directory))
	{
		std::error_code ec;
		fs::create_directories(Directory, ec);

		if (ec)
		{
			UE_LOG("[TextureConverter] Failed to create cache directory: %s",
			       WideToUTF8(Directory.wstring()).c_str());
		}
	}
}
