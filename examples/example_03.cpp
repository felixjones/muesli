#include <muesli/codecs>
#include <muesli/format/binary_format>
#include <muesli/runtime/adapter>
#include <muesli/runtime/compile>
#include <muesli/schema/binary_schema_format>
#include <muesli/schema/lowering>

#include <cassert>
#include <iostream>
#include <sstream>

namespace mu = muesli;

struct book {
    std::string title;
    std::string author;
    int32_t year;
    float rating;

    static constexpr auto codec = mu::tuple_codec(
        mu::string_codec.member<&book::title>(),
        mu::string_codec.member<&book::author>(),
        mu::int32_codec.member<&book::year>(),
        mu::float_codec.member<&book::rating>()
    ).apply<book>();
};

static const auto book_schema = mu::schema::build_schema(book::codec);
static constexpr auto book_adapter = mu::runtime::adapter(book::codec);

int main() {
    const book myBook{
        "Chewing Muesli for Beginners",
        "Captain Kellogg",
        1864,
        1.2f
    };

    // Write data
    std::stringstream bookBytes(std::ios::in | std::ios::out | std::ios::binary);
    mu::make_binary_format<char>(book::codec).serialize(myBook, bookBytes);
    std::vector<std::byte> bookSchemaBytes = mu::schema::serialize_schema(book_schema);

    // Read data
    std::optional<muesli::runtime::codec> anonymousBookCodec = mu::runtime::compile(bookSchemaBytes);
    std::istringstream inputStream(bookBytes.str(), std::ios::binary);
    std::optional<mu::runtime::value_node> anonymousBook = anonymousBookCodec->deserialize(inputStream);

    // Adapt into typed data
    std::optional<book> adaptedBook = book_adapter.adapt(*anonymousBook);

    assert(adaptedBook->title == myBook.title);
    assert(adaptedBook->author == myBook.author);
    assert(adaptedBook->year == myBook.year);
    assert(adaptedBook->rating == myBook.rating);

    std::cout << "Book round-trip successful:\n";
    std::cout << "  Title:  " << adaptedBook->title << "\n";
    std::cout << "  Author: " << adaptedBook->author << "\n";
    std::cout << "  Year:   " << adaptedBook->year << "\n";
    std::cout << "  Rating: " << adaptedBook->rating << "\n";
}
