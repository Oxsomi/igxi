# IGXI
The IGXI (Ignis Xtended Image) Format is used to store data that can immediately be loaded by the GPU after a format lookup (the formats supported by the GPU). 
This format also allows loading in multiple layers as well as a different base mip if specified.
## Dependencies
IGXI headers already include the ignis enums and core2 types that it uses; to avoid dependencies.
Adding [https:://github.com/Nielsbishere/core2](core2) however, will allow you to use the default file loader. Otherwise you have to provide your own.
## API Usage
Without implementing a file loader, you can still use a binary loader. This means the entire file has to be loaded in memory. If you do implement the file loader, it will only use the parts of the binary that are actually used (this means all formats that aren't loaded are cut out; mips and layers that are cut aren't loaded either).
### Binary loader
### File loader
#### Default
#### Custom
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
    
    //24 bytes header
    struct Header {
        
        char8 header[4];	//IGXI
        
        uint32 version;		//1 or higher
        
        u16 width, height;
        
        u16 length, layers;
        
        TextureFlags flags;				//& CONTAINS_DATA (1)
        ignis::GPUMemoryUsage usage;
        ignis::TextureType type;
        u8 mips;					//How many mips are used (< maxMips(w,h,l))
        
        u8 padding[3];
        u8 formats;					//How many formats are available
        
    } header;
    
    //[header.formats]; Picks the first available format for the current device
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
    
    List<Buffer> data;
    
    static IGXI load()
};
```

Like described; this format can store compressed and non-compressed textures side-by-side if not supported by the current GPU. Generally either ASTC or BC is supported and storing both is quite cheap. If you know the resource is only used on Android or Linux it can use ASTC only, but on Windows BC is used instead. Cross-platform resources should include both formats if possible. This means you can also include the non-compressed version if neither are supported; this shouldn't happen in non-debug builds though.

IGXI doesn't have to include data, it can also just be used to describe a resource (like a compute target). This means Framebuffers could potentially also be outputted to this format.

IGXI can also be loaded from a different mip if it has data.

IGXI can be converted back to individual images or generated through a tool.
