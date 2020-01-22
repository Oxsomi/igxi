#pragma once
#include <stdint.h>
#include <vector>
#include <array>
#include <string>

//Minimal core2 types

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usz = size_t;

using f32 = float;
using f64 = double;

using char8 = char;

template<typename T>
using List = std::vector<T>;

template<typename T, usz N>
using Array = std::array<T, N>;

using String = std::string;

using Buffer = List<u8>;

namespace igxi {

	//ignis enums predefined

	enum class GPUFormat : u16;
	enum class GPUFormatType : u8;
	enum class GPUMemoryUsage : u8;
	enum class TextureType : u8;

	//IGXI format

	struct IGXI {

		//A file loader
		//Generally uses core2, otherwise link start/stop/readRegion yourself
		struct FileLoader {

			struct Data;		//Implementation dependent data

			const String file;	//File to read (application dependent)
			Data *data;

			FileLoader(const String &str) : file(str) {
				start();
			}

			~FileLoader() {
				stop();
			}

			//Read a region of a file into an address; returns true if out of bounds
			//Start has to be incremented with length by the implementation
			bool readRegion(void *addr, usz &start, usz length) const;

			//Checks if a region is unavailable (true if out of bounds)
			//Start has to be incremented with length by the implementation
			bool checkRegion(usz &start, usz length) const;

			//Get the size of a file (0 if non existent or if folder)
			usz size() const;

		private:

			void start();
			void stop();
		};

		//Flags defined in the header;
		//currently only used to determine if it contains data
		enum class Flags : u8 {

			NONE = 0,
			CONTAINS_DATA = 1 << 0,

			ALL = CONTAINS_DATA
		};

		//Versions
		enum class Version : u32 {
			INVALID,
			V1
		};

		//24 bytes header
		struct Header {

			static constexpr u32 magicNumber = 0x49584749;	//IGXI

			u32 header = magicNumber;

			Version version = Version::V1;

			u16 width, height;

			u16 length, layers;

			Flags flags;				//& CONTAINS_DATA (1)
			GPUMemoryUsage usage;
			TextureType type;
			u8 mips;					//How many mips are used (< maxMips(w,h,l))

			u8 signature[3] = { 0x44, 0x55, 0x66 };
			u8 formats;					//How many formats are available (>0)

		} header;

		//Only contains the loaded GPUFormats (header.formats)
		//
		//Generally only one format is used, but in case of compression
		// 		multiple compression formats can be used (please use compression)
		//			IGXI should at least include
		//				ASTC (1-8 bits per pixel)
		//				BC (4-8 bits per pixel)
		//			as fallback, you can always provide a general texture format
		//				RGBA8 or RGBA16f (generally not needed; so please avoid it)
		//				NOTE: This does increase the amount of data
		//					from 5-16 to 37-48 or 69-80 bits per pixel
		//
		//If the user is confident in targetting one platform
		//	they can always use ASTC or BC without fallback
		List<GPUFormat> format;

		//Only if CONTAINS_DATA is turned on
		//Otherwise it will be zero-initialized
		//
		//The data layout per format for a Texture2DArray with size 2x2x1
		//Compressed formats use implementation dependent layouts
		//
		//x0y0z0m0 x1y0z0m0
		//x0y1z0m0 x1y1z0m0
		//
		//x0y0z0m0 x1y0z0m0
		//x0y1z0m0 x1y1z0m0
		//
		//x0y0z0m1
		//
		List<List<Buffer>> data;		//[format][mip]

		//Loading and saving

		//Error messages
		enum class ErrorMessage {
			SUCCESS,
			LOAD_INVALID_HEADER,			//IGXI has invalid header (might not be IGXI)
			LOAD_INVALID_SIZE,				//IGXI has invalid size
			LOAD_INVALID_FILE,
			LOAD_NO_AVAILABLE_FORMATS,		//There were no formats or none that matched
			LOAD_INVALID_RANGE,				//Invalid range was requested
		};

		//Input data

		struct InputParams {

			List<GPUFormat> supportedFormats{};

			//Layers to load (relative to layerStart)
			//Can also include the same layer multiple times if you need it
			List<u16> layers{};

			u16 startLayer{}, layerCount{};	//Layer range to load

			u8 startMip{}, mipCount{};
			
			bool loadMultipleFormats{};
			bool loadData = true;			//If false; won't try to load data (so you can get the header)

			//Load header with or without data
			InputParams(bool loadData = true);

			//Only load relevant GPUFormats, layers and mips into header or memory
			//loadMultipleFormats is false by default; as mostly only one is needed
			//startMip is 0 by default
			//All layers are loaded by default; but you can explicitly mention layers it will load
			//If you don't specify a mip count, it will load all mips from startMip
			InputParams(
				const List<GPUFormat> &supportedFormats,
				bool loadData = true,
				bool loadMultipleFormats = {},
				u8 startMip = {},
				const List<u16> &layers = {},
				u8 mipCount = {}
			);

			//Only load relevant GPUFormats, layers and mips into header or memory
			//loadMultipleFormats is false by default; as mostly only one is needed
			//startMin is 0 by default
			//startLayer is 0 by default
			//If you don't specify a layer count, it will load all layers from startLayer
			//If you don't specify a mip count, it will load all mips from startMip
			InputParams(
				const List<GPUFormat> &supportedFormats,
				bool loadData = true,
				bool loadMultipleFormats = {},
				u8 startMip = {},
				u16 startLayer = {},
				u16 layerCount = {},
				u8 mipCount = {}
			);

		};

		#if defined(__IGXI_CUSTOM_FILE_LOADER__) || defined(__USE_CORE2__)

			//Load from file (see FileLoader)
			static ErrorMessage load(const String &file, IGXI &out, const InputParams &ip);

		#endif

		//Load from binary (always available)
		static ErrorMessage load(const Buffer &buf, IGXI &out, const InputParams &ip);

		//TODO: static ErrorMessage save();

	};

	//Minimal dependencies

	//format.hpp

	//1 nibble (0x0-0xF)
	//& 1			= isSigned
	//& 2			= isUnnormalized
	//& 4			= isFloatingPoint
	enum class GPUFormatType : u8 {

		UNORM,	SNORM,
		UINT,	SINT,

		FLOAT = 7,

		PROPERTY_IS_SIGNED = 1,
		PROPERTY_IS_UNNORMALIZED = 2,
		PROPERTY_IS_FLOATING_POINT = 4
	};

	//(& 3) + 1			= channel count
	//1 << ((>> 2) & 3)	= channel stride
	//(>> 4) & 0xF		= type
	//& 0x10			= isSigned
	//& 0x20			= isUnnormalized
	//& 0x40			= isFloatingPoint
	//& 0x100			= isSRGB
	//& 0x200			= flip channels (RGBA -> BGRA, RGB = BGR)
	//& 0x400			= isNone
	enum class GPUFormat : u16 {

		R8 = 0x00,		RG8,	RGB8,	RGBA8,
		R16,			RG16,	RGB16,	RGBA16,

		R8s = 0x10,		RG8s,	RGB8s,	RGBA8s,
		R16s,			RG16s,	RGB16s,	RGBA16s,

		R8u = 0x20,		RG8u,	RGB8u,	RGBA8u,
		R16u,			RG16u,	RGB16u, RGBA16u,
		R32u,			RG32u,	RGB32u, RGBA32u,
		R64u,			RG64u,	RGB64u, RGBA64u,

		R8i = 0x30,		RG8i,	RGB8i,	RGBA8i,
		R16i,			RG16i,	RGB16i, RGBA16i,
		R32i,			RG32i,	RGB32i, RGBA32i,
		R64i,			RG64i,	RGB64i, RGBA64i,

		R16f = 0x74,	RG16f,	RGB16f,	RGBA16f,
		R32f,			RG32f,	RGB32f,	RGBA32f,
		R64f,			RG64f,	RGB64f, RGBA64f,

		sRGB8 = 0x102,	sRGBA8,
		BGR8 = 0x202,	BGRA8,
		BGR8s = 0x212,	BGRA8s,
		BGR8u = 0x222,	BGRA8u,
		BGR8i = 0x232,	BGRA8i,
		sBGR8 = 0x302,	sBGRA8,

		NONE = 0x400
	};

	//This is a usage hint to how the GPU memory should behave:
	//Flags: 
	//& 1 = isShared		; is CPU accessible (!isShared = device local)
	//& 2 = isPreferred		; requires heap to be the same (!isPreferred = use specified heap)
	//& 4 = isGPUWritable	; can the resource be written to from the GPU?
	//& 8 = isCPUWritable	; can the CPU update this or is it just an initialization
	enum class GPUMemoryUsage : u8 {

		LOCAL				= 0x0,
		SHARED				= 0x1,

		REQUIRE				= 0x0,
		PREFER				= 0x2,

		GPU_WRITE			= 0x4,
		CPU_WRITE			= 0x8,

		ALL = 0xF
	};

	struct FormatHelper {

		static constexpr bool isFloatingPoint(GPUFormat gf);
		static constexpr bool isFloatingPoint(GPUFormatType gft);

		static constexpr bool isSigned(GPUFormat gf);
		static constexpr bool isSigned(GPUFormatType gft);

		static constexpr bool isUnnormalized(GPUFormat gf);
		static constexpr bool isUnnormalized(GPUFormatType gft);

		static constexpr bool isNone(GPUFormat gf);
		static constexpr GPUFormatType getType(GPUFormat gf);

		static constexpr bool isSRGB(GPUFormat gf);
		static constexpr bool flipRGB(GPUFormat gf);

		static constexpr bool isInt(GPUFormat gf);
		static constexpr bool isInt(GPUFormatType gft);

		static constexpr usz getStrideBits(GPUFormat gf);
		static constexpr usz getStrideBytes(GPUFormat gf);
		static constexpr usz getChannelCount(GPUFormat gf);
		static constexpr usz getSizeBits(GPUFormat gf);
		static constexpr usz getSizeBytes(GPUFormat gf);
	};

	constexpr bool FormatHelper::isFloatingPoint(GPUFormat gf) { return u8(gf) & 0x40; }
	constexpr bool FormatHelper::isFloatingPoint(GPUFormatType gft) { return u8(gft) & 0x4; }

	constexpr bool FormatHelper::isSigned(GPUFormat gf) { return u8(gf) & 0x10; }
	constexpr bool FormatHelper::isSigned(GPUFormatType gft) { return u8(gft) & 0x1; }

	constexpr bool FormatHelper::isUnnormalized(GPUFormat gf) { return u8(gf) & 0x20; }
	constexpr bool FormatHelper::isUnnormalized(GPUFormatType gft) { return u8(gft) & 0x2; }

	constexpr bool FormatHelper::isNone(GPUFormat gf) { return u16(gf) & 0x400; }
	constexpr GPUFormatType FormatHelper::getType(GPUFormat gf) { return GPUFormatType(u8(gf) >> 4); }

	constexpr bool FormatHelper::isSRGB(GPUFormat gf) { return u16(gf) & 0x100; }
	constexpr bool FormatHelper::flipRGB(GPUFormat gf) { return u16(gf) & 0x200; }

	constexpr bool FormatHelper::isInt(GPUFormat gf) { return isUnnormalized(gf) && !isFloatingPoint(gf); }
	constexpr bool FormatHelper::isInt(GPUFormatType gft) { return isUnnormalized(gft) && !isFloatingPoint(gft); }

	constexpr usz FormatHelper::getStrideBits(GPUFormat gf) { return usz(8) << ((u8(gf) >> 2) & 3); }
	constexpr usz FormatHelper::getStrideBytes(GPUFormat gf) { return usz(1) << ((u8(gf) >> 2) & 3); }
	constexpr usz FormatHelper::getChannelCount(GPUFormat gf) { return usz(1) + (u8(gf) & 3); }
	constexpr usz FormatHelper::getSizeBits(GPUFormat gf) { return getStrideBits(gf) * getChannelCount(gf); }
	constexpr usz FormatHelper::getSizeBytes(GPUFormat gf) { return getStrideBytes(gf) * getChannelCount(gf); }

	//enums.hpp

	//& 0x03 = dimension (CUBE, 1D, 2D, 3D)
	//& 0x04 = isMultisampled
	//& 0x08 = isArray
	enum class TextureType : u8 {

		TEXTURE_CUBE			= 0x0,
		TEXTURE_1D, 
		TEXTURE_2D,
		TEXTURE_3D,
		TEXTURE_MS				= 0x6,

		TEXTURE_CUBE_ARRAY		= 0x8, 
		TEXTURE_1D_ARRAY, 
		TEXTURE_2D_ARRAY,
		TEXTURE_MS_ARRAY		= 0xC,

		PROPERTY_DIMENSION		= 0x3,
		PROPERTY_IS_MS			= 0x4,
		PROPERTY_IS_ARRAY		= 0x8,	//1 << PROPERTY_IS_ARRAY_BIT
		PROPERTY_IS_ARRAY_BIT	= 0x3
	};
}