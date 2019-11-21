#pragma once

//Minimal core2 types

#include <stdint.h>
#include <vector>
#include <string>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using usz = size_t;

using char8 = char;

template<typename T>
using List = std::vector<T>;

using String = std::string;

using Buffer = List<u8>;

//Minimal ignis enums

namespace ignis {

	//format.hpp

	//& 1			= isSigned
	//& 2			= isUnnormalized
	//& 4			= isFloatingPoint
	enum class GPUFormatType : u8 {

		UNORM,	SNORM,
		UINT,	SINT,

		FLOAT = 7
	};

	//(& 3) + 1			= channel count
	//1 << ((>> 2) & 3)	= channel stride
	//(>> 4) & 0x7		= type
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
		CPU_WRITE			= 0x8
	};

	//enums.hpp

	//& 0x03 = dimension (CUBE, 1D, 2D, 3D)
	//& 0x04 = isMultisampled
	//& 0x08 = isArray
	enum class TextureType : u8 {

		TEXTURE_CUBE			= 0x0,
		TEXTURE_1D, 
		TEXTURE_2D,
		TEXTURE_3D,
		TEXTURE_MS				= 0x6,		//TODO: Figure out how this works with data and IXGI

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

//Minimal format helper (only for GPUFormat)

namespace igx {

	using namespace ignis;

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


}