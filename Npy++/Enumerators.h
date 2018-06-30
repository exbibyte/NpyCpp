#pragma once

enum class FileOpenMode
{
	Null,
	Read,
	Write,
	Append,
};

static const char* ToString(const FileOpenMode mode)
{
	switch (mode)
	{
		case FileOpenMode::Read:
			return "r";
		case FileOpenMode::Write:
			return "w";
		case FileOpenMode::Append:
			return "a";
		default:
			return "?";
	}
}
