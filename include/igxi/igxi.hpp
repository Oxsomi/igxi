#pragma once
#include "deps.hpp"

namespace igx {

	struct IGXI {

		//A file loader
		//Generally uses core2, otherwise link start/stop/readRegion yourself
		struct FileLoader {

			struct Data;		//Implementation dependent data

			const String file;
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

			u32 header;

			Version version;

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
		List<Buffer> data;

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

			u32 startMip{}, mipCount{};
			
			List<GPUFormat> supportedFormats{};

			bool loadMultipleFormats{};

			List<u32> layers{};
			u32 layerStart{}, layerEnd{};

			//Load with all data (not recommended, unless needed)
			InputParams();

			//Only load relevant GPUFormats, layers and mips
			//loadMultipleFormats is false by default; as mostly only one is needed
			//startMin is 0 by default
			//All layers are loaded by default; but you explicitly mention layers it will load
			//If you don't specify an end mip, it will load all mips from startMip
			InputParams(
				const List<GPUFormat> &supportedFormats,
				bool loadMultipleFormats = {},
				u32 startMip = {},
				const List<u32> &layers = {},
				u32 endMip = {}
			);

			//Only load relevant GPUFormats, layers and mips
			//loadMultipleFormats is false by default; as mostly only one is needed
			//startMin is 0 by default
			//startLayer is 0 by default
			//If you don't specify an end layer, it will load all layers
			//If you don't specify an end mip, it will load all mips from startMip
			InputParams(
				const List<GPUFormat> &supportedFormats,
				bool loadMultipleFormats = {},
				u32 startMip = {},
				u32 startLayer = {},
				u32 endLayer = {},
				u32 endMip = {}
			);

		};

		#if defined(__IGXI_CUSTOM_FILE_LOADER__) || defined(__USE_CORE2__)

			//Load from file (see FileLoader)
			static ErrorMessage load(const String &file, IGXI &out, const InputParams &ip);

		#endif

		//Load from binary (always available)
		static ErrorMessage load(const Buffer &buf, IGXI &out, const InputParams &ip);

		//static ErrorMessage save();

	};

}