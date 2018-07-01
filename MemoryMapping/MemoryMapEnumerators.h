#pragma once

namespace mm
{
	enum class CacheHint
	{
		Normal,         ///< good overall performance
		SequentialScan, ///< read file only once with few seeks
		RandomAccess    ///< jump around
	};
}
