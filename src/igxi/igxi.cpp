#include "igxi/igxi.hpp"
#include <unordered_map>
#include <cstring>
#include <cmath>
#include <algorithm>

#ifdef __USE_CORE2__
	#include "system/system.hpp"
	#include "system/local_file_system.hpp"
#endif

namespace igxi {

	#ifdef __USE_CORE2__

		void IGXI::File::start(bool) {
			data = (IGXI::File::Data*) oic::System::files()->open(file);
		}

		void IGXI::File::stop() {
			oic::System::files()->close((oic::File*)data);
		}

		bool IGXI::File::readRegion(void *addr, usz &start, usz length) const {

			if (!((oic::File*)data)->read(addr, length, start))
				return true;
				
			start += length;
			return false;
		}

		bool IGXI::File::writeRegion(const void *addr, usz &start, usz length) {

			if (!((oic::File*)data)->write(addr, length, start))
				return true;
				
			start += length;
			return false;
		}

		bool IGXI::File::resize(usz length) {
			return !((oic::File*)data)->resize(length);
		}

		bool IGXI::File::checkRegion(usz &start, usz length) const {

			if (!((oic::File*)data)->hasRegion(length, start))
				return true;

			start += length;
			return false;
		}

		usz IGXI::File::size() const {
			return ((oic::File*)data)->size();
		}

	#endif

	template<typename K, typename V>
	using HashMap = std::unordered_map<K, V>;

	//Input params constructor

	IGXI::InputParams::InputParams(bool loadData) : loadData(loadData) {}

	//Only load relevant GPUFormats, layers and mips into header or memory
	//loadMultipleFormats is false by default; as mostly only one is needed
	//startMip is 0 by default
	//All layers are loaded by default; but you explicitly mention layers it will load
	//If you don't specify a mip count, it will load all mips from startMip
	IGXI::InputParams::InputParams(
		const List<GPUFormat> &supportedFormats,
		bool loadData,
		bool loadMultipleFormats,
		u8 startMip,
		const List<u16> &layers,
		u8 mipCount
	) :
		supportedFormats(supportedFormats), layers(layers),
		startMip(startMip), mipCount(mipCount),
		loadMultipleFormats(loadMultipleFormats), loadData(loadData) {}

	//Only load relevant GPUFormats, layers and mips into header or memory
	//loadMultipleFormats is false by default; as mostly only one is needed
	//startMin is 0 by default
	//startLayer is 0 by default
	//If you don't specify a layer count, it will load all layers from startLayer
	//If you don't specify a mip count, it will load all mips from startMip
	IGXI::InputParams::InputParams(
		const List<GPUFormat> &supportedFormats,
		bool loadData,
		bool loadMultipleFormats,
		u8 startMip,
		u16 startLayer,
		u16 layerCount,
		u8 mipCount
	) :
		supportedFormats(supportedFormats), startLayer(startLayer), layerCount(layerCount),
		startMip(startMip), mipCount(mipCount),
		loadMultipleFormats(loadMultipleFormats), loadData(loadData) {}

	//A binary loader
	struct BinaryFile {

		Buffer file;

		inline BinaryFile(Buffer &buf) : file(buf) { }

		inline bool resize(usz length) {
			file.resize(length);
			return false;
		}

		inline bool readRegion(void *addr, usz &start, usz length) const {

			if (start + length > file.size())
				return true;

			std::memcpy(addr, file.data() + start, length);
			start += length;
			return false;
		}

		inline bool checkRegion(usz &start, usz length) const {

			if (start + length > file.size())
				return true;

			start += length;
			return true;
		}

		inline bool writeRegion(const void *addr, usz &start, usz length) {

			if (start + length > file.size())
				return true;

			std::memcpy(file.data() + start, addr, length);

			start += length;
			return false;
		}

	};

	inline List<usz> getFormatSizes(GPUFormat format, IGXI::Header head) {

		List<usz> sizes(head.mips);

		for (usz i = 0; i < head.mips; ++i) {

			sizes[i] = FormatHelper::getSizeBytes(format) * head.width * head.height * head.length * head.layers;

			head.width = (u16)std::ceil(head.width / 2.f);
			head.height = (u16)std::ceil(head.height / 2.f);
			head.length = (u16)std::ceil(head.length / 2.f);
		}

		return sizes;
	}

	inline usz getFormatBytes(GPUFormat format, IGXI::Header head) {

		usz size{};

		for (usz i = 0; i < head.mips; ++i) {

			size += FormatHelper::getSizeBytes(format) * head.width * head.height * head.length * head.layers;

			head.width = (u16)std::ceil(head.width / 2.f);
			head.height = (u16)std::ceil(head.height / 2.f);
			head.length = (u16)std::ceil(head.length / 2.f);
		}

		return size;
	}

	template<typename File>
	inline IGXI::ErrorMessage loadFormats(
		const File &file, IGXI::Header head, IGXI &out, const IGXI::InputParams &input, const HashMap<usz, GPUFormat> &formats
	) {

		u8 endMip = input.startMip + input.mipCount;

		out.data.resize(formats.size());

		usz j{};

		for (auto &fmt : formats) {

			usz src = fmt.first;
			out.data[j].resize(out.header.mips);

			for (usz i = 0; i < head.mips; ++i) {

				usz perLayer = FormatHelper::getSizeBytes(fmt.second) * head.width * head.height * head.length;

				if (i >= input.startMip && i < endMip) {

					auto mip = out.data[j][i];
					mip.resize(perLayer * out.header.layers);

					//No layers masked out; so load all

					if (!input.layers.size())
						file.readRegion(mip.data(), src, mip.size());

					//Layers masked out; so fragmented
					//Foreach layer do a copy

					else {

						usz dst{};

						for (usz k = 0, l = input.layers.size(); k < l; ++k) {
							usz src0 = src + perLayer * (usz(input.layers[k]) + input.startLayer);
							file.readRegion(mip.data() + dst, src0, perLayer);
							dst += perLayer;
						}

					}

				}
				
				//Skip a mip
				else
					src += perLayer * head.layers;

				//Resize to next mip

				head.width = (u16)std::ceil(head.width / 2.f);
				head.height = (u16)std::ceil(head.height / 2.f);
				head.length = (u16)std::ceil(head.length / 2.f);
			}

			++j;
		}

		return IGXI::ErrorMessage::SUCCESS;
	}

	inline u8 getMaxMips(u16 width, u16 height, u16 length) {
		return u8(std::ceil(std::log2(std::fmax(width, std::fmax(height, length)))));
	}

	template<typename File>
	inline IGXI::ErrorMessage loadData(const File &file, IGXI &out, const IGXI::InputParams &input) {

		//Read header

		using Header = IGXI::Header;
		using Version = IGXI::Version;
		using Flags = IGXI::Flags;

		usz start{};

		if (file.readRegion(&out.header, start, sizeof(Header)))
			return IGXI::ErrorMessage::LOAD_INVALID_SIZE;

		//Copy of header of the actual file; out.header is the output header (the data the output file includes)
		const auto head = out.header;

		//Validate header

		static constexpr u8 sign[3]{ 0x44, 0x55, 0x66 };

		if(
			head.header != Header::magicNumber ||
			head.version != Version::V1 ||
			head.flags > Flags::ALL ||
			std::memcmp(head.signature, sign, sizeof(sign)) != 0 ||
			head.formats == 0 ||
			head.width == 0 ||
			head.height == 0 ||
			head.length == 0 ||
			head.layers == 0 ||
			head.mips == 0 ||
			head.usage > GPUMemoryUsage::ALL ||
			head.mips > getMaxMips(head.width, head.height, head.length)
		)
			return IGXI::ErrorMessage::INVALID_HEADER;

		TextureType val = TextureType(u8(head.type) & ~u8(TextureType::PROPERTY_IS_ARRAY));

		if(val > TextureType::TEXTURE_MS || (val > TextureType::TEXTURE_3D && val != TextureType::TEXTURE_MS))
			return IGXI::ErrorMessage::INVALID_HEADER;

		//Find formats

		bool noData = !(u8(head.flags) & u8(Flags::CONTAINS_DATA)) || !input.loadData;

		List<GPUFormat> availableFormats(head.formats);

		if(file.readRegion(availableFormats.data(), start, sizeof(GPUFormat) * head.formats))
			return IGXI::ErrorMessage::LOAD_INVALID_SIZE;

		HashMap<usz, GPUFormat> formats;

		for (GPUFormat format : availableFormats) {

			usz loc = start;

			if(file.checkRegion(start, getFormatBytes(format, head)))
				return IGXI::ErrorMessage::LOAD_INVALID_SIZE;

			formats[loc] = format;
		}

		//Get real mip and layer count from input

		if(input.startMip >= head.mips || input.startLayer >= head.layers)
			return IGXI::ErrorMessage::LOAD_INVALID_RANGE;

		auto mips = out.header.mips = input.mipCount ? input.mipCount : head.mips - input.startMip;
		auto layers = out.header.layers = input.layerCount ? input.layerCount : head.layers - input.startLayer;

		if(input.startMip + mips > head.mips || input.startLayer + layers > head.layers)
			return IGXI::ErrorMessage::LOAD_INVALID_RANGE;

		if (input.layers.size()) {

			if(input.layers.size() > 0xFFFF)
				return IGXI::ErrorMessage::LOAD_INVALID_RANGE;

			for(auto layer : input.layers)
				if(layer >= out.header.layers)
					return IGXI::ErrorMessage::LOAD_INVALID_RANGE;

			out.header.layers = u16(input.layers.size());
		}

		for (usz i = 0; i < input.startMip; ++i) {
			out.header.width = (u16)std::ceil(out.header.width / 2.f);
			out.header.height = (u16)std::ceil(out.header.height / 2.f);
			out.header.length = (u16)std::ceil(out.header.length / 2.f);
		}

		//Load first format
		if (!input.loadMultipleFormats) {

			auto fmt = formats.end();

			for (auto format : formats) {

				auto it = std::find(input.supportedFormats.begin(), input.supportedFormats.end(), format.second);

				if (it != input.supportedFormats.end()) {
					fmt = formats.find(format.first);
					break;
				}
			}

			if(fmt == formats.end())
				return IGXI::ErrorMessage::LOAD_NO_AVAILABLE_FORMATS;

			out.format = { fmt->second };

			if(noData)
				return IGXI::ErrorMessage::SUCCESS;

			return loadFormats(file, head, out, input, { *fmt });
		}

		//Load multiple formats
		else {

			//Load a few formats
			if (input.supportedFormats.size()) {

				auto fmts = decltype(formats){};

				for (auto it = formats.begin(); it != formats.end(); ++it) {

					if (std::find(input.supportedFormats.begin(), input.supportedFormats.end(), it->second) != input.supportedFormats.end()) {
						out.format.push_back(it->second);
						fmts[it->first] = it->second;
					}

				}

				if(fmts.size() == 0)
					return IGXI::ErrorMessage::LOAD_NO_AVAILABLE_FORMATS;

				if(noData)
					return IGXI::ErrorMessage::SUCCESS;

				return loadFormats(file, head, out, input, fmts);
			}

			//Load all formats
			else {

				for (auto format : formats)
					out.format.push_back(format.second);

				if(noData)
					return IGXI::ErrorMessage::SUCCESS;

				return loadFormats(file, head, out, input, formats);
			}
		}
	}


	template<typename File>
	inline IGXI::ErrorMessage saveData(const File &file, const IGXI &in) {

		//Validate header

		using Header = IGXI::Header;
		using Version = IGXI::Version;
		using Flags = IGXI::Flags;

		const Header &head = in.header;
		static constexpr u8 sign[3]{ 0x44, 0x55, 0x66 };

		if(
			head.header != Header::magicNumber ||
			head.version != Version::V1 ||
			head.flags > Flags::ALL ||
			memcmp(head.signature, sign, sizeof(sign)) != 0 ||
			head.formats == 0 ||
			head.width == 0 ||
			head.height == 0 ||
			head.length == 0 ||
			head.layers == 0 ||
			head.mips == 0 ||
			head.usage > GPUMemoryUsage::ALL ||
			head.mips > getMaxMips(head.width, head.height, head.length) ||
			head.formats != in.format.size()
		)
			return IGXI::ErrorMessage::INVALID_HEADER;

		TextureType val = TextureType(u8(head.type) & ~u8(TextureType::PROPERTY_IS_ARRAY));

		if(val > TextureType::TEXTURE_MS || (val > TextureType::TEXTURE_3D && val != TextureType::TEXTURE_MS))
			return IGXI::ErrorMessage::INVALID_HEADER;

		//Validate data

		const bool containsData = u8(head.flags) & u8(Flags::CONTAINS_DATA);

		if (containsData) {

			if (in.data.size() != in.format.size())
				return IGXI::ErrorMessage::SAVE_INVALID_FORMATS;

			usz i {};
			for (GPUFormat format : in.format) {

				auto &dat = in.data[i];

				if (dat.size() != head.mips)
					return IGXI::ErrorMessage::SAVE_INVALID_MIPS;

				auto formatSizes = getFormatSizes(format, head);

				for (usz j = 0; j < head.mips; ++j)
					if (dat[j].size() != formatSizes[j])
						return IGXI::ErrorMessage::SAVE_INVALID_DATA_SIZE;

				++i;
			}
		}

		//Reserve size for file

		usz bufferSize = sizeof(Header) + sizeof(GPUFormat) * head.formats;

		if (containsData) {
			for (GPUFormat format : in.format)
				bufferSize += getFormatBytes(format, head);
		}

		if (file.resize(bufferSize))
			return IGXI::ErrorMessage::SAVE_NO_SPACE;

		//Write header

		usz start{};

		if (file.writeRegion(&head, start, sizeof(Header)))
			return IGXI::ErrorMessage::SAVE_FILE_ACCESS;

		//Find formats

		if (file.writeRegion(in.format.data(), start, sizeof(GPUFormat) * head.formats))
			return IGXI::ErrorMessage::SAVE_FILE_ACCESS;

		//Check in.data

		if (containsData) {

			usz i {};
			for (GPUFormat format : in.format) {

				auto &dat = in.data[i];

				for (usz j = 0; j < head.mips; ++j)
					if (!file.writeRegion(dat[j].data(), start, dat[j].size()))
						return IGXI::ErrorMessage::SAVE_FILE_ACCESS;

				++i;
			}
		}

		return IGXI::ErrorMessage::SUCCESS;
	}

	#if defined(__IGXI_CUSTOM_FILE_LOADER__) || defined(__USE_CORE2__)

		IGXI::ErrorMessage IGXI::load(const String &file, IGXI &out, const InputParams &ip){

			try {

				const IGXI::File f(file, false);
				return loadData(f, out, ip);

			} catch (std::runtime_error&) {
				return IGXI::ErrorMessage::LOAD_INVALID_FILE;
			}
		}

		IGXI::ErrorMessage IGXI::save(const IGXI &out, const String &file){

			try {

				const IGXI::File f(file, true);
				return saveData(f, out);

			} catch (std::runtime_error) {
				return IGXI::ErrorMessage::SAVE_INVALID_FILE;
			}
		}

	#endif

	IGXI::ErrorMessage IGXI::load(const Buffer &buf, IGXI &out, const InputParams &ip) {
		const BinaryFile file((Buffer&)buf);
		return loadData(file, out, ip);
	}


	IGXI::ErrorMessage IGXI::save(const IGXI &in, Buffer &buf) {
		const BinaryFile file(buf);
		return saveData(file, in);
	}
}