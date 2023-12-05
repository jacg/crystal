#include "config.hh"
#include "io.hh"

#include <arrow/io/api.h>

#include <memory>
#include <string>
#include <unordered_map>

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


std::tuple<arrow::Compression::type, unsigned short> parse_compression_spec(std::string spec) {
  char* s = spec.data();
  char* c_type  = std::strtok(s      , "-"); std::string s_type {c_type};
  char* c_level = std::strtok(nullptr, "-"); std::string s_level{c_level};

  std::cout << "s_type: " << s_type << "  s_level: " << s_level << std::endl;

  arrow::Compression::type type;
  int level;

  for (auto& c: s_type) { c = std::tolower(c); }
  if (s_type == "snappy") { type = arrow::Compression::SNAPPY; level = -1; }
  if (s_type == "gzip"  ) { type = arrow::Compression::GZIP  ; level =  6; }
  if (s_type == "zstd  ") { type = arrow::Compression::ZSTD  ; level =  9; }

  if (! s_level.empty()) { level = std::stoi(s_level); }

  std::cout
    << "RESULT OF PARSING COMPRESSION SPEC   RESULT OF PARSING COMPRESSION SPEC   RESULT OF PARSING COMPRESSION SPEC\n"
    << "type: " << type << "   level: " << level  << std::endl;

  return {type, level};
}

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
