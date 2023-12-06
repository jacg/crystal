#include "config.hh"
#include "io.hh"

#include <arrow/io/api.h>

#include <boost/algorithm/string/split.hpp>          // boost::split
#include <boost/algorithm/string/classification.hpp> // boost::is_any_of

#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

std::vector<std::shared_ptr<arrow::Field>> fields() {
  std::vector<std::shared_ptr<arrow::Field>> fields;
  auto n_sipms = my.n_sipms();
  fields.reserve(n_sipms + 3);
  fields.push_back(arrow::field("x", arrow::float32()));
  fields.push_back(arrow::field("y", arrow::float32()));
  fields.push_back(arrow::field("z", arrow::float32()));
  for (auto n=0; n<n_sipms; n++) {
    fields.push_back(arrow::field("sipm_" + std::to_string(n), arrow::uint16()));
  }
  return fields;
}

std::vector<std::shared_ptr<arrow::UInt16Builder>> counts(arrow::MemoryPool* pool) {
  std::vector<std::shared_ptr<arrow::UInt16Builder>> counts;
  counts.reserve(my.n_sipms());
  for (auto n=0; n<my.n_sipms(); n++) {
    counts.push_back(std::make_shared<arrow::UInt16Builder>(pool));
  }
  return counts;
}


#define EXIT(stuff) std::cerr << "\n\n    " << stuff << "\n\n\n"; std::exit(EXIT_FAILURE);
std::tuple<arrow::Compression::type, unsigned short> parse_compression_spec(std::string spec) {
  // Split spec into algorithm and (maybe) level
  std::vector<std::string> result;
  boost::split(result, spec, boost::is_any_of("-"));

  // Detect and report errors relating to number of split parts
  if (result.empty())    { EXIT("Not enough information in compression spec: '" << spec << "'."); }
  if (result.size() > 2) { EXIT("Too many '-'s in compression spec: '" << spec <<"'.")  }

  auto& type_spec = result[0];
  arrow::Compression::type type;
  int level = -1;

  for (auto& c: type_spec) { c = std::tolower(c); }
  if      (type_spec == "brotli" ) { type = arrow::Compression::BROTLI; level = 11; }
  else if (type_spec == "bz2"    ) { type = arrow::Compression::BZ2   ; level =  9; }
  else if (type_spec == "gzip"   ) { type = arrow::Compression::GZIP  ; level =  6; }
  else if (type_spec == "lz4"    ) { type = arrow::Compression::LZ4   ; level =  9; }
  else if (type_spec == "snappy" ) { type = arrow::Compression::SNAPPY;             }
  else if (type_spec == "zstd"   ) { type = arrow::Compression::ZSTD  ; level =  9; }
  else if (type_spec == "none"   ) { type = arrow::Compression::UNCOMPRESSED; }
  else if (type_spec == "lzo"    ) { type = arrow::Compression::LZO; EXIT("LZO not available in C++") }
  else { EXIT("Unrecognized compression algorithm '" << type_spec << "'") }

  if (result.size() == 2) {
    const auto& level_spec = result[1];
    try {
      level = std::stoi(level_spec);
    } catch (const std::exception& e) {
      EXIT("Could not parse compression level '" << level_spec << "'")
    }
  }

  std::cout
    << "RESULT OF PARSING COMPRESSION SPEC   RESULT OF PARSING COMPRESSION SPEC   RESULT OF PARSING COMPRESSION SPEC\n"
    << "type: " << type << "   level: " << level  << std::endl;

  return {type, level};
}
#undef EXIT

std::unique_ptr<parquet::arrow::FileWriter> make_writer(
  std::shared_ptr<arrow::Schema> schema,
  arrow::MemoryPool* pool)
{
  // Choose compression and opt to store Arrow schema for easier reads
  // back into Arrow

  // auto  file_props = parquet::     WriterProperties::Builder();
  // file_props.compression(compression);
  // file_props.compression_level(8);
  // //if (level >= 0) { file_props.compression_level(level); }

  auto [compression, level] = parse_compression_spec(my.compression);
  auto  file_props = parquet::     WriterProperties::Builder()
    .compression(compression)
    //-> compression_level(level)
    -> build();
  auto arrow_props = parquet::ArrowWriterProperties::Builder().store_schema() -> build();
  auto outfile = arrow::io::FileOutputStream::Open(my.outfile).ValueOrDie();
  return parquet::arrow::FileWriter::Open( *schema, pool, outfile,
                                           file_props, arrow_props).ValueOrDie();
}

std::shared_ptr<const arrow::KeyValueMetadata> metadata() {
  auto config_map = my.as_map();
  auto N = config_map.size();
  std::vector<std::string> keys  ; keys  .reserve(N);
  std::vector<std::string> values; values.reserve(N);
  for (const auto& [k, v]: config_map) {
    keys  .push_back(k);
    values.push_back(v);
  }
  return std::make_shared<const arrow::KeyValueMetadata>(keys, values);
}

parquet_writer::parquet_writer() :
  pool          {arrow::default_memory_pool()}
, x_builder     {std::make_shared<arrow::FloatBuilder>(pool)}
, y_builder     {std::make_shared<arrow::FloatBuilder>(pool)}
, z_builder     {std::make_shared<arrow::FloatBuilder>(pool)}
, counts_builder{counts(pool)}
, schema        {std::make_shared<arrow::Schema>(fields(), metadata())}
, writer        {make_writer(schema, pool)}
{}

arrow::Result<std::shared_ptr<arrow::Table>> parquet_writer::make_table() {
  auto n_sipms = my.n_sipms();
  std::vector<std::shared_ptr<arrow::Array>> arrays;
  arrays.reserve(n_sipms);

  ARROW_ASSIGN_OR_RAISE(auto x_array, x_builder -> Finish()); arrays.push_back(x_array);
  ARROW_ASSIGN_OR_RAISE(auto y_array, y_builder -> Finish()); arrays.push_back(y_array);
  ARROW_ASSIGN_OR_RAISE(auto z_array, z_builder -> Finish()); arrays.push_back(z_array);

  for (auto n=0; n<n_sipms; n++) {
    ARROW_ASSIGN_OR_RAISE(auto c_array, counts_builder[n] -> Finish());
    arrays.push_back(c_array);
  }

  return arrow::Table::Make(schema, arrays);
};

arrow::Status parquet_writer::append(const G4ThreeVector& pos, std::unordered_map<size_t, size_t> counts) {
  ARROW_RETURN_NOT_OK(x_builder -> Append(pos.x()));
  ARROW_RETURN_NOT_OK(y_builder -> Append(pos.y()));
  ARROW_RETURN_NOT_OK(z_builder -> Append(pos.z()));

  unsigned n;
  for (auto i=0; i<my.n_sipms(); i++) {
    n = counts.contains(i) ? counts[i] : 0;
    ARROW_RETURN_NOT_OK(counts_builder[i] -> Append(n));
  }
  n_rows++;
  return n_rows == my.chunk_size ? write() : arrow::Status::OK();
}

arrow::Status parquet_writer::write() {
  ARROW_ASSIGN_OR_RAISE(auto data, make_table());
  ARROW_RETURN_NOT_OK(writer -> WriteTable( *data.get(), n_rows));
  n_rows = 0;
  return arrow::Status::OK();
}
