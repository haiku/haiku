/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_BUFFER_H_
#define _B_HTTP_BUFFER_H_

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

class BDataIO;
class BString;


namespace BPrivate {

namespace Network {

using HttpTransferFunction = std::function<size_t(const std::byte*, size_t)>;


class HttpBuffer
{
public:
								HttpBuffer(size_t capacity = 8 * 1024);

			ssize_t				ReadFrom(BDataIO* source,
									std::optional<size_t> maxSize = std::nullopt);
			size_t				WriteTo(HttpTransferFunction func,
									std::optional<size_t> maxSize = std::nullopt);
			void				WriteExactlyTo(HttpTransferFunction func,
									std::optional<size_t> maxSize = std::nullopt);
			std::optional<BString> GetNextLine();

			size_t				RemainingBytes() const noexcept;

			void				Flush() noexcept;
			void				Clear() noexcept;

			std::string_view	Data() const noexcept;

	// load data into the buffer
			HttpBuffer&			operator<<(const std::string_view& data);

private:
			std::vector<std::byte> fBuffer;
			size_t				fCurrentOffset = 0;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_BUFFER_H_
