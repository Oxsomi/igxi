#pragma once
#include "graphics/enums.hpp"

namespace igxi {

	//IGXI format

	struct IGXI {

		//A file loader
		//Generally uses core2, otherwise remove it and add start/stop/readRegion yourself
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

			void start();
			void stop();

			//Read a region of a file into an address; returns true if out of bounds
			//Start has to be incremented with length by the implementation
			bool readRegion(void *addr, usz &start, usz length) const;

			//Checks if a region is unavailable (true if out of bounds)
			//Start has to be incremented with length by the implementation
			bool checkRegion(usz &start, usz length) const;

			//Get the size of a file (0 if non existent or if folder)
			usz size() const;

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
			ignis::GPUMemoryUsage usage;
			ignis::TextureType type;
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
		List<ignis::GPUFormat> format;

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
			LOAD_NO_AVAILABLE_FORMATS,		//There were no formats or none that matched
			LOAD_INVALID_RANGE,				//Invalid range was requested
		};

		//Input data

		struct InputParams {

			List<ignis::GPUFormat> supportedFormats{};

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
				const List<ignis::GPUFormat> &supportedFormats,
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
				const List<ignis::GPUFormat> &supportedFormats,
				bool loadData = true,
				bool loadMultipleFormats = {},
				u8 startMip = {},
				u16 startLayer = {},
				u16 layerCount = {},
				u8 mipCount = {}
			);

		};

		//Load from file (see FileLoader)
		static ErrorMessage load(const String &file, IGXI &out, const InputParams &ip);

		//Load from binary (always available)
		static ErrorMessage load(const Buffer &buf, IGXI &out, const InputParams &ip);

		//TODO: static ErrorMessage save();

	};

}