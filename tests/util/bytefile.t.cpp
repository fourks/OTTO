#include "../testing.t.hpp"

#include <iterator>

#include "util/bytefile.hpp"
#include "util/algorithm.hpp"

namespace top1 {

  using Chunk = ByteFile::Chunk;
  using Path = ByteFile::Path;
  using Error = ByteFile::Error;

  TEST_CASE("Bytefile", "[util]") {

    Path somePath = "testdata/test1.bytes";
    test::truncateFile(somePath);

    ByteFile f;

    SECTION("Open / close") {

      REQUIRE(!f.is_open());
      REQUIRE_NOTHROW(f.open(somePath));

      REQUIRE_NOTHROW(f.close());

      REQUIRE(!f.is_open());
      // REQUIRE(filesystem::exists(somePath));
      // REQUIRE(filesystem::is_regular_file(somePath));
    }

    SECTION("seek") {

      ByteFile::Position somePos = 10;

      REQUIRE(!f.is_open());
      REQUIRE_THROWS_AS(f.position(), ByteFile::Error);

      REQUIRE_NOTHROW(f.open(somePath));
      REQUIRE_NOTHROW(f.position() == 0);
      REQUIRE(f.size() == 0);

      SECTION ("Seeking past the end") {
        REQUIRE_NOTHROW(f.seek(somePos) == somePos);
        REQUIRE(f.position() == somePos);
      }
    }

    SECTION("Write / Read bytes") {

      constexpr std::size_t someSize = 2048;

      REQUIRE_NOTHROW(f.open(somePath));

      std::array<std::byte, someSize> bytes;

      REQUIRE_NOTHROW(f.write_bytes(bytes.begin(), bytes.end()));
      REQUIRE_NOTHROW(f.position() == someSize);
      REQUIRE_NOTHROW(f.size() == someSize);
      REQUIRE_NOTHROW(f.close());

      std::array<std::byte, someSize> readBytes;
      REQUIRE_NOTHROW(f.open(somePath));
      REQUIRE_NOTHROW(f.read_bytes(readBytes.begin(), readBytes.end()));

      REQUIRE_NOTHROW(f.position() == someSize);
      REQUIRE_NOTHROW(f.size() == someSize);

      // Compare arrays
      REQUIRE(std::equal(readBytes.begin(), readBytes.end(), bytes.begin()));

      REQUIRE_THROWS_AS(f.read_bytes(readBytes.begin(), 10), Error);
    }

    SECTION("Chunks") {

      Path somePath2 = "testdata/test2.bytes";
      int someLength = 4096;

      struct SomeChunk : Chunk {
        bytes<4> field1;
        bytes<12> field2;

        SomeChunk() : Chunk("Chu1") {}
        SomeChunk(Chunk& c) : Chunk(c) {}

        void write_fields(ByteFile& file) override {
          file.write_bytes(field1);
          file.write_bytes(field2);
        }

        void read_fields(ByteFile& file) override {
          file.read_bytes(field1);
          file.read_bytes(field2);
        }
      };

      struct SomeOtherChunk : Chunk {
        bytes<4> field1;
        std::vector<std::byte> dynField;

        SomeOtherChunk() : Chunk("Chu2") {}
        SomeOtherChunk(Chunk& c) : Chunk(c) {}

        void write_fields(ByteFile& file) override {
          file.write_bytes(field1);
          file.write_bytes(dynField.begin(), dynField.end());
        }

        void read_fields(ByteFile& file) override {
          file.read_bytes(field1);
          file.read_bytes(std::back_inserter(dynField), size.as_u() - 4);
        }
      };

      f.open(somePath2);
      REQUIRE(f.is_open());

      SomeChunk sc;
      SomeOtherChunk soc;

      sc.field1.as_u() = 0x52F93DAA;
      sc.field2 = {0xFF, 0x25, 0x54, 0x20, 0x99, 0xD3,
                   0xFF, 0x25, 0x54, 0x20, 0x99, 0xD3,};

      soc.field1.as_u() = 0x852B2C3;
      std::generate_n(std::back_inserter(soc.dynField), someLength,
        [&] () {
          return std::byte(std::rand());
        });

      sc.write(f);
      soc.write(f);

      f.close();
      REQUIRE(!f.is_open());

      f.open(somePath2);
      REQUIRE(f.is_open());

      SECTION ("for_chunks_in_range") {

        REQUIRE_NOTHROW(f.for_chunks_in_range(0, f.size(),
            [&] (auto&& c) {
              if (c.id == "Chu1") {
                SomeChunk rsc(c);
                rsc.read(f);
                if (rsc.field1 != sc.field1) throw "Chunk mismatch";
                if (rsc.field2 != sc.field2) throw "Chunk mismatch";
              } else if (c.id == "Chu2") {
                SomeOtherChunk rsoc(c);
                rsoc.read(f);
                if (rsoc.field1 != soc.field1) throw "Chunk mismatch";
                if (!std::equal(soc.dynField.begin(), soc.dynField.end(),
                    rsoc.dynField.begin())) throw "Chunk mismatch";
              }
            }));

        // out of bounds

        REQUIRE_NOTHROW(f.for_chunks_in_range(0, f.size() + 4,
            [&] (auto&& c) {}));

      }
    }
  }

}
