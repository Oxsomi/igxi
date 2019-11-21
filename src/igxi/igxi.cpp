#include "igxi/igxi.hpp"
#include <unordered_map>

template<typename K, typename V>
using HashMap = std::unordered_map<K, V>;

namespace igx {

	//TODO: Core2 impl
	//TODO: Input params impl

	//void start();
	//void stop();

	////Read a region of a file into an address; returns true if out of bounds
	//bool readRegion(Buffer &buf, usz start, usz length) const;

	//A binary loader
	struct BinaryLoader {

		const Buffer file;

		BinaryLoader(const Buffer &buf) : file(buf) { }

		bool readRegion(void *addr, usz &start, usz length) const {

			if (start + length > file.size())
				return true;

			memcpy(addr, file.data() + start, length);
			start += length;
			return false;
		}

		bool checkRegion(usz &start, usz length) const {

			if (start + length > file.size())
				return true;

			start += length;
			return true;
		}

	};

	template<typename Loader>
	inline IGXI::ErrorMessage loadData(const Loader &loader, IGXI &out, const IGXI::InputParams &input) {

		//Read header

		using Header = IGXI::Header;
		using Version = IGXI::Version;
		using Flags = IGXI::Flags;

		usz start{};

		if (loader.readRegion(&out.header, start, sizeof(Header)))
			return IGXI::ErrorMessage::LOAD_INVALID_SIZE;

		//Copy of header of the actual file; out.header is the output header (the data the output file includes)
		auto head = out.header;

		u8 sign[3]{ 0x44, 0x55, 0x66 };

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
			head.mips == 0
			//TODO: Check usage, type, mips (< max)
		)
			return IGXI::ErrorMessage::LOAD_INVALID_HEADER;

		//Find formats

		bool noData = !(u8(head.flags) & u8(Flags::CONTAINS_DATA));

		List<GPUFormat> availableFormats(head.formats);

		if(loader.readRegion(availableFormats.data(), start, sizeof(GPUFormat) * head.formats))
			return IGXI::ErrorMessage::LOAD_INVALID_SIZE;

		HashMap<usz, GPUFormat> formats;

		for (GPUFormat format : availableFormats) {

			usz loc = start;

			if(loader.checkRegion(start, getFormatBytes(format, head)))
				return IGXI::ErrorMessage::LOAD_INVALID_SIZE;

			formats[loc] = format;
		}

		//TODO: Change number of mips and resolution depending on start mip and end mip in out.head
		//TODO: Change number of layers depending on available layers in input struct in out.head

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

			return loadFormats(loader, head, out, input, { *fmt });
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

				return loadFormats(loader, head, out, input, fmts);
			}

			//Load all formats
			else {

				for (auto format : formats)
					out.format.push_back(format.second);

				if(noData)
					return IGXI::ErrorMessage::SUCCESS;

				return loadFormats(loader, head, out, input, formats);
			}

		}
	}

	#if defined(__IGXI_CUSTOM_FILE_LOADER__) || defined(__USE_CORE2__)

		IGXI::ErrorMessage IGXI::load(const String &file, IGXI &out, const InputParams &ip){
			const IGXI::FileLoader loader(file);
			return loadData(loader, out, ip);
		}

	#endif

		IGXI::ErrorMessage IGXI::load(const Buffer &buf, IGXI &out, const InputParams &ip) {
		const BinaryLoader loader(buf);
		return loadData(loader, out, ip);
	}
}