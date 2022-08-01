/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_BUFFER_H_
#define _B_HTTP_BUFFER_H_

#include <functional>
#include <optional>
#include <vector>

class BDataIO;
class BString;


namespace BPrivate {

namespace Network {

using HttpTransferFunction = std::function<size_t (const std::byte*, size_t)>;


class HttpBuffer {
public:
							HttpBuffer(size_t capacity = 8*1024);

	ssize_t					ReadFrom(BDataIO* source);
	void					WriteExactlyTo(BDataIO* target);
	void					WriteTo(HttpTransferFunction func,
								std::optional<size_t> maxSize = std::nullopt);
	std::optional<BString>	GetNextLine();

	size_t					RemainingBytes() noexcept;

	void					Flush() noexcept;
	void					Clear() noexcept;

private:
	std::vector<std::byte>	fBuffer;
	size_t					fCurrentOffset = 0;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_BUFFER_H_
