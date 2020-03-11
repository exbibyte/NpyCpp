#include <Npy++.h>

#include <iostream>

namespace npypp
{
	namespace detail
	{
		char IsBigEndian()
		{
			union
			{
				uint32_t i;
				char c[4];
			} u{ 0x01020304 };

			return u.c[0] == 1 ? '>' : '<';
		}

        #pragma region Npy Utilities

		std::string GetNpyHeaderFortranOrder()
		{
			return "'fortran_order': False";
		}

		std::string GetNpyHeaderShape(const std::vector<size_t>& shape)
		{
			std::string ret;

			ret += "'shape': (";
			for (const auto i : shape)
			{
				ret += std::to_string(i);
				ret += ", ";
			}
			ret += ")";

			return ret;
		}

		std::string SetNpyHeaderPadding(std::string& properties)
		{
			// pad with spaces so that preamble + dict is modulo 16 bytes. preamble is 10 bytes. 
			// properties needs to end with \n

			auto remainder = 16 - (10 + properties.size()) % 16;
			properties.insert(properties.end(), remainder, ' ');
			properties.back() = '\n';

			return properties;
		}

		bool ParseFortranOrder(const std::string& npyHeader)
		{
			auto position = npyHeader.find("fortran_order");
			assert(position != std::string::npos);

			return npyHeader.substr(position + 16, 4) == "True";
		}

		void ParseShape(std::vector<size_t>& shape, const std::string& npyHeader)
		{
			auto loc1 = npyHeader.find("(");
			auto loc2 = npyHeader.find(")");
			assert(loc1 != std::string::npos && loc2 != std::string::npos);

			auto tokens = strutils::Tokenize<std::vector<std::string>>(npyHeader.substr(loc1 + 1, loc2 - loc1 - 1), ',');
			// avoid that last element is null
			int decrementer = 0;
			if (tokens[tokens.size() - 1].find_first_not_of(' ') == std::string::npos)
				decrementer = 1;

			shape.resize(tokens.size() - static_cast<size_t>(decrementer));
			for (size_t i = 0; i < shape.size(); ++i)
				shape[i] = static_cast<size_t>(std::atoi(tokens[i].c_str()));
		}

		/**
		* Returns the word size
		*/
		size_t ParseDescription(const std::string& npyHeader)
		{
			auto position = npyHeader.find("descr");
			assert(position != std::string::npos);

			position += 9;
			//byte order code | stands for not applicable. 
			//not sure when this applies except for byte array
			const bool UNUSED littleEndian = npyHeader[position] == '<' || npyHeader[position] == '|';
			assert(littleEndian);

			std::string wordSizeString = npyHeader.substr(position + 2);
			position = wordSizeString.find("'");
			assert(position != std::string::npos);
			return static_cast<size_t>(std::atoi(wordSizeString.substr(0, position).c_str()));
		}

		void ParseNpyHeader(FILE* fp, size_t& wordSize, std::vector<size_t>& shape, bool& fortranOrder)
		{
			constexpr size_t expectedCharToRead{ 11 };
			char buffer[256];
			const size_t UNUSED charactersRead = fread(buffer, sizeof(char), expectedCharToRead, fp);
			assert(charactersRead == expectedCharToRead);

			std::string header = fgets(buffer, 256, fp);
			assert(header[header.size() - 1] == '\n');

			fortranOrder = ParseFortranOrder(header);
			ParseShape(shape, header);
			wordSize = ParseDescription(header);
		}

        #pragma endregion

        #pragma region Npz Utilities

		void ParseNpzFooter(FILE* fp, uint16_t& nRecords, size_t& globalHeaderSize, size_t& globalHeaderOffset)
		{
			constexpr int footerLength{ 22 };
			// footer is 22 chars long, seek the file pointer to 22 places from the end
			fseek(fp, -footerLength, SEEK_END);

			std::array<char, footerLength> footer {};
			size_t UNUSED elementsRead = fread(&footer[0], sizeof(char), footerLength, fp);
			assert(elementsRead == footerLength);

			std::memcpy(&nRecords, &footer[10], sizeof(nRecords));
            std::memcpy(&globalHeaderSize, &footer[12], sizeof(globalHeaderSize));
            std::memcpy(&globalHeaderOffset, &footer[16], sizeof(globalHeaderOffset));

            #ifndef NDEBUG
                uint16_t tmp;
                std::memcpy(&tmp, &footer[4], sizeof(tmp)); assert(tmp == 0);
                std::memcpy(&tmp, &footer[6], sizeof(tmp)); assert(tmp == 0);
                std::memcpy(&tmp, &footer[8], sizeof(tmp)); assert(tmp == nRecords);
                std::memcpy(&tmp, &footer[20], sizeof(tmp)); assert(tmp == 0);
            #endif
		}

		std::string GetLocalHeader(const uint32_t crc, const uint32_t nBytes, const std::string& localNpyFileName)
		{
			std::string ret("PK"); // signature magic
			appendBytes<uint16_t>(ret, 0x0403); // signature magic

			appendBytes<uint16_t>(ret, 20); // min version to extract
			appendBytes<uint16_t>(ret, 0); // general purpose bit flag
			appendBytes<uint16_t>(ret, 0); // compression method
			appendBytes<uint16_t>(ret, 0); // file last mod time
			appendBytes<uint16_t>(ret, 0); // file last mod date

			appendBytes<uint32_t>(ret, crc);
			appendBytes<uint32_t>(ret, nBytes); // compressed size
			appendBytes<uint32_t>(ret, nBytes); // uncompressed size

			appendBytes<uint16_t>(ret, static_cast<uint16_t>(localNpyFileName.size())); // npy file size
			appendBytes<uint16_t>(ret, 0); // extra field length

			ret += localNpyFileName;

			return ret;
		}

		void AppendGlobalHeader(std::string& out, const std::string& localHeader, const uint32_t globalHeaderOffset, const std::string& localNpyFileName)
		{
			out += "PK"; // signature magic
			appendBytes<uint16_t>(out, 0x0201); // signature magic

			appendBytes<uint16_t>(out, 20); // version made by

			out.insert(out.end(), localHeader.begin() + 4, localHeader.begin() + 30);  // insert local header

			appendBytes<uint16_t>(out, 0); // file comment length
			appendBytes<uint16_t>(out, 0); // disk number where file starts
			appendBytes<uint16_t>(out, 0); // internal file attributes

			appendBytes<uint32_t>(out, 0); // external file attributes
			appendBytes<uint32_t>(out, globalHeaderOffset); // relative offset of local file header, since it begins where the global header used to begin
			
			out += localNpyFileName;
		}

		std::string GetNpzFooter(const std::string& globalHeader, const uint32_t globalHeaderOffset, const uint32_t localHeaderSize, const uint32_t nElementsInBytes, const uint16_t nNewRecords)
		{
			std::string ret("PK"); // signature magic
			appendBytes<uint16_t>(ret, 0x0605); // signature magic

			appendBytes<uint16_t>(ret, 0); // number of this disk
			appendBytes<uint16_t>(ret, 0); // disk where footer starts

			appendBytes<uint16_t>(ret, nNewRecords); // number of records on this disk
			appendBytes<uint16_t>(ret, nNewRecords); // total number of records

			appendBytes<uint32_t>(ret, static_cast<uint32_t>(globalHeader.size())); // nbytes of global headers
			appendBytes<uint32_t>(ret, globalHeaderOffset + nElementsInBytes + localHeaderSize); // offset of start of global headers, since global header now starts after newly written array

			appendBytes<uint16_t>(ret, 0); // zip file comment length

			return ret;
		}

        #pragma endregion
	}
}
