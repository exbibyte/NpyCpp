#include <Npy++.h>

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

		std::string GetNpyHeaderFortranOrder()
		{
			return "'fortran_order': False";
		}

		std::string GetNpyHeaderShape(const std::vector<size_t>& shape)
		{
			std::string ret;

			ret += "'shape': (";
			for (size_t i = 0; i < shape.size(); ++i)
			{
				ret += std::to_string(shape[i]);
				ret += ", ";
			}
			ret += ")";

			return ret;
		}

		std::string SetNpyHeaderPadding(std::string& properties)
		{
			// pad with spaces so that preamble + dict is modulo 16 bytes. preamble is 10 bytes. 
			// properties needs to end with \n

			int remainder = 16 - (10 + properties.size()) % 16;
			properties.insert(properties.end(), remainder, ' ');
			properties.back() = '\n';

			return properties;
		}

		std::string GetMagic()
		{
			std::string magic = "";
			magic += static_cast<char>(0x93);
			magic += "NUMPY";
			magic += static_cast<char>(0x01); //major version of numpy format
			magic += static_cast<char>(0x00); //minor version of numpy format

			return magic;
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

			shape.resize(tokens.size() - decrementer);
			for (size_t i = 0; i < shape.size(); ++i)
				shape[i] = std::atoi(tokens[i].c_str());
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
			const bool littleEndian = npyHeader[position] == '<' || npyHeader[position] == '|';
			assert(littleEndian);

			std::string wordSizeString = npyHeader.substr(position + 2);
			position = wordSizeString.find("'");
			assert(position != std::string::npos);
			return std::atoi(wordSizeString.substr(0, position).c_str());
		}

		void ParseNpyHeader(FILE* fp, size_t& wordSize, std::vector<size_t>& shape, bool& fortranOrder)
		{
			constexpr size_t expectedCharToRead{ 11 };
			char buffer[256];
			const size_t charactersRead = fread(buffer, sizeof(char), expectedCharToRead, fp);
			assert(charactersRead == expectedCharToRead);

			std::string header = fgets(buffer, 256, fp);
			assert(header[header.size() - 1] == '\n');

			fortranOrder = ParseFortranOrder(header);
			ParseShape(shape, header);
			wordSize = ParseDescription(header);
		}
	}
}