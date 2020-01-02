# IGXI
The IGXI (Ignis Xtended Image) Format is used to store data that can immediately be loaded by the GPU after a format, layer and mip lookup. 
This format allows loading in multiple layers as well as a different base mip if specified (saving GPU memory for mobile; like specifying 4k images for desktop and then loading 256x, 512x or 1k images on mobile).
It also supports multiple GPU formats, to ensure you can use different types of compression for the same image. This means you can also have a depth stencil saved (R32f + R8u formats in 1 texture). These GPU formats have to be unique, as you load based on GPU format (so asking for R32f would load the depth and R8u would load the stencil buffer).

![](https://github.com/Nielsbishere/igxi/workflows/C%2FC++%20CI/badge.svg)

## Dependencies
IGXI headers already include the ignis enums and core2 types that it uses; to avoid dependencies.
Adding [core2](https:://github.com/Nielsbishere/core2) however, will allow you to use the default file loader. Otherwise you have to provide your own.

## API Usage
Without implementing a file loader, you can still use a binary loader. This means the entire file has to be loaded in memory. If you do implement the file loader, it will only use the parts of the binary that are actually used (this means all formats that aren't loaded are cut out; mips and layers that are cut aren't loaded either).

### TODO: Input parameters

### Binary loader

`static ErrorMessage load(const Buffer &buf, IGXI &out, const InputParams &ip)`

Call `igxi::IGXI::load` with your loaded `igxi::Buffer` (Just a `std::vector<uint8_t>`) and the output and input params:

```cpp
using namespace igxi;

Buffer myBuffer = ...;
IGXI out{};
IGXI::InputParams in = ...;
IGXI::ErrorMessage msg = IGXI::load(myBuffer, output, in);

if(msg != IGXI::ErrorMessage::SUCCESS)
    throw std::runtime_error("Couldn't load IGXI");
```

### File loader

Loading via a file loader is required if you want the benefits of skipping regions of IGXI that you don't have to load. If this file is already in memory, you can use the binary loader. I would suggest to use file loading whenever possible, since IGXI files could become very large if they included a wide range of GPU formats, layers and mips that won't be loaded.

Especially for mobile devices, it is important that file loading is used to skip loading mips, layers and formats that aren't used by the GPU.

`static ErrorMessage load(const String &fileName, IGXI &out, const InputParams &ip)`

Call `igxi::IGXI::load` with a String your file loader understands and the output and input params:

```cpp
using namespace igxi;

String myFile = "./test.IGXI";	//Currently doesn't support UTF-16 input files
IGXI out{};
IGXI::InputParams in = ...;
IGXI::ErrorMessage msg = IGXI::load(myBuffer, output, in);

if(msg != IGXI::ErrorMessage::SUCCESS)
    throw std::runtime_error("Couldn't load IGXI");
```

#### Default

The default file loader is from core2 like mentioned in the **dependencies**. This is however not included in the submodules to avoid bloat. If you think core2 is too much to include in your framework or tool, you can implement your own custom file loader (see **custom**).

core2 has two file types; `~/myPath.IGXI` and `./myPath.IGXI`.  The tilde `~` before a path means it is a virtual path (it exists in the exe, apk, etc; it's not really on disk). The dot `./` before a path means it accesses the working directory. core2 can't read out from external paths (`../` or `./../` are invalid paths). You can however use `./` and `../` in a valid path, like: `./MyFolder/./Test/../../` would just evaluate to `./`.

##### Implementation

```cpp
//Locks and unlocks the file system

void IGXI::FileLoader::start() {
	oic::System::files()->begin();
}

void IGXI::FileLoader::stop() {
	oic::System::files()->end();
}

//Reads and checks file region for availability

bool IGXI::FileLoader::readRegion(void *addr, usz &start, usz length) const {

	if (!oic::System::files()->regionExists(file, length, start))
		return true;

	if (!oic::System::files()->read(file, addr, length, start))
		return true;
				
	start += length;
	return false;
}

bool IGXI::FileLoader::checkRegion(usz &start, usz length) const {

	if (!oic::System::files()->regionExists(file, length, start))
		return true;

	start += length;
	return false;
}

usz IGXI::FileLoader::size() const {

	if (!oic::System::files()->exists(file))
		return 0;

	return oic::System::files()->get(file).fileSize;
}
```

#### Custom

If you want a custom file reader, you can implement `igxi::IGXI::FileLoader` for yourself. First you have to set the `__IGXI_CUSTOM_FILE_LOADER__` define before including igxi or define it in cmake before you add igxi.

You can allocate `FileLoader::Data *data` in `FileLoader::start` and deallocate it in `FileLoader::stop` if you need it.  If you don't need this extra data, your start and stop functions can be empty. These stop and start functions have to be implemented.

`bool FileLoader::readRegion(void *addr, usz &start, usz length) const` has to be implemented and should return `true` if out of bounds and `false` if it read the file region correctly. If it read the file region correctly, it should increase the start by length. The file read should just copy directly to the address given, which will have enough space allocated (as long as the file read only reads length bytes).

`bool checkRegion(usz &start, usz length) const` has to be implemented and should return `true` if out of bounds and `false` if it the file region exists. If the file region exists it should increase the start by length.

`usz size() const` has to be implemented and should return a non-zero size for a file that has content. If the file doesn't exist or is a directory, this should return 0.

See the default file read implementation for the basics.

**NOTE: The file's size and contents should remain identical while it is being read.**

## Format
IGXI is the image format of Ignix; it contains all the texture data and how they can be loaded.

```cpp
struct IGXI {
    
    //Flags defined in the header;
    //currently only used to determine if it contains data
    enum Flags : u8 {
        NONE = 0,
      	CONTAINS_DATA = 1 << 0
    };
    

	//Versions
	enum class Version : u32 {
		INVALID,
		V1
	};
    
    //24 bytes header
    struct Header {
        
        char8 header[4];	//IGXI
        
        Version version;
        
        u16 width, height;
        
        u16 length, layers;
        
        Flags flags;				//& CONTAINS_DATA (1)
        ignis::GPUMemoryUsage usage;
        ignis::TextureType type;
        u8 mips;					//How many mips are used (< maxMips(w,h,l))
        
        u8 padding[3];
        u8 formats;					//How many formats are available
        
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
};
```

Like described; this format can store compressed and non-compressed textures side-by-side if not supported by the current GPU. Generally either ASTC or BC is supported and storing both is quite cheap. If you know the resource is only used on Android or Linux it can use ASTC only, but on Windows BC is used instead. Cross-platform resources should include both formats if possible. This means you can also include the non-compressed version if neither are supported; this shouldn't happen in non-debug builds though (since it is a lot of extra data).

IGXI doesn't have to include data, it can also just be used to describe a resource (like a compute target). This means empty and filled Framebuffers could potentially also be outputted to this format.

IGXI can only load a couple of mips, formats or layers if is required.

IGXI can be converted back to individual images or generated through a tool.
