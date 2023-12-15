#include "config.hh"
#include "io.hh"

#include <n4-sequences.hh>

#include <arrow/io/api.h>

#include <parquet/arrow/reader.h>

#include <boost/algorithm/string/split.hpp>          // boost::split
#include <boost/algorithm/string/classification.hpp> // boost::is_any_of

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

const bool NOT_NULLABLE = false;

auto interaction_type = arrow::struct_({
  arrow::field("x"   , arrow::float32(), NOT_NULLABLE),
  arrow::field("y"   , arrow::float32(), NOT_NULLABLE),
  arrow::field("z"   , arrow::float32(), NOT_NULLABLE),
  arrow::field("edep", arrow::float32(), NOT_NULLABLE),
  arrow::field("type", arrow:: uint32(), NOT_NULLABLE),
});

std::vector<std::shared_ptr<arrow::Field>> fields() {
  return {
    arrow::field("x", arrow::float32()),
    arrow::field("y", arrow::float32()),
    arrow::field("z", arrow::float32()),
    arrow::field("interactions"
                , arrow::list(arrow::field( "interaction"
                                          , interaction_type
                                          , NOT_NULLABLE))
                , NOT_NULLABLE),
    arrow::field("photon_counts", arrow::fixed_size_list(arrow::uint32(), my.n_sipms()))
  };
}

std::shared_ptr<arrow::FixedSizeListBuilder> counts(arrow::MemoryPool* pool) {
  return std::make_shared<arrow::FixedSizeListBuilder>(pool, std::make_shared<arrow::UInt32Builder>(), my.n_sipms());
}

#define EXIT(stuff) std::cerr << "\n\n    " << stuff << "\n\n\n"; std::exit(EXIT_FAILURE);
std::tuple<arrow::Compression::type, std::optional<int>> parse_compression_spec(std::string spec) {
  // Split spec into algorithm and (maybe) level
  std::vector<std::string> result;
  boost::split(result, spec, boost::is_any_of("-"));

  // Detect and report errors relating to number of split parts
  if (result.empty())    { EXIT("Not enough information in compression spec: '" << spec << "'."); }
  if (result.size() > 2) { EXIT("Too many '-'s in compression spec: '" << spec <<"'.")  }

  auto& type_spec = result[0];
  arrow::Compression::type type;
  std::optional<int> level{};

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
    << "Chosen compression type: " << type
    << "   level: " << (level.has_value() ? std::to_string(level.value()) : "NONE")
    << std::endl;

  return {type, level};
}
#undef EXIT

std::unique_ptr<parquet::arrow::FileWriter> make_writer(
  std::shared_ptr<arrow::Schema> schema,
  arrow::MemoryPool* pool)
{
  // Choose compression and opt to store Arrow schema for easier reads
  // back into Arrow
  auto [compression, level] = parse_compression_spec(my.compression);

  auto  file_props_builder = parquet::     WriterProperties::Builder();
  file_props_builder.compression(compression);
  if (level.has_value()) { file_props_builder.compression_level(level.value()); }
  auto file_props = file_props_builder.build();

  auto arrow_props = parquet::ArrowWriterProperties::Builder().store_schema() -> build();
  auto outfile = arrow::io::FileOutputStream::Open(my.outfile).ValueOrDie();
  return parquet::arrow::FileWriter::Open(*schema, pool, outfile, file_props, arrow_props).ValueOrDie();
}

std::string run_shell_cmd(const std::string& cmd, size_t buffer_size = 1024) {
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) { std::cerr << "popen() failed!" << std::endl; exit(EXIT_FAILURE); }

  char buffer[buffer_size];
  std::string result;
  while (fgets(buffer, buffer_size, pipe.get()) != nullptr) { result += buffer; }
  return result;
}

std::unordered_map<std::string, std::string> git_metadata() {
  std::unordered_map<std::string, std::string> out;

  auto commit_hash = run_shell_cmd("git log -1 --pretty=format:\"%H\"");
  auto commit_date = run_shell_cmd("git log -1 --pretty=format:\"%ad\" --date=iso8601");
  auto commit_msg  = run_shell_cmd("git log -1 --pretty=format:\"%s\"");

  out["commit-hash"] = commit_hash;
  out["commit-date"] = commit_date;
  out["commit-msg" ] = commit_msg ;

  return out;
}

std::shared_ptr<const arrow::KeyValueMetadata> metadata() {
  auto config_map = my.as_map();
  auto   git_meta = git_metadata();
  auto   cli_args = my.cli_args();

  auto N = config_map.size()
         +   git_meta.size()
         +   cli_args.size();
  std::vector<std::string> keys  ; keys  .reserve(N);
  std::vector<std::string> values; values.reserve(N);

  auto insert = [&] (const auto& map) { for (const auto& [k, v]: map) { keys.push_back(k); values.push_back(v);} };
  insert(config_map);
  insert(  git_meta);
  insert(  cli_args);

  return std::make_shared<const arrow::KeyValueMetadata>(keys, values);
}

std::shared_ptr<arrow::Schema> make_schema() {
  return std::make_shared<arrow::Schema>(fields(), metadata());
}

auto make_interaction_builder() {
  auto pool = arrow::default_memory_pool();
  std::vector<std::shared_ptr<arrow::ArrayBuilder>> vec_of_builders {
    std::make_shared<arrow:: FloatBuilder>(pool),
    std::make_shared<arrow:: FloatBuilder>(pool),
    std::make_shared<arrow:: FloatBuilder>(pool),
    std::make_shared<arrow:: FloatBuilder>(pool),
    std::make_shared<arrow::UInt32Builder>(pool)
  };
  return std::make_shared<arrow::StructBuilder>(interaction_type, pool, vec_of_builders);
}

parquet_writer::parquet_writer() :
  pool                {arrow::default_memory_pool()}
, x_builder           {std::make_shared<arrow::FloatBuilder>(pool)}
, y_builder           {std::make_shared<arrow::FloatBuilder>(pool)}
, z_builder           {std::make_shared<arrow::FloatBuilder>(pool)}
, interactions_builder{std::make_shared<arrow:: ListBuilder>(pool, make_interaction_builder(), interaction_type)}
, counts_builder      {counts(pool)}
, schema              {std::make_shared<arrow::Schema>(fields(), metadata())}
, writer              {make_writer(schema, pool)}
{}

parquet_writer::~parquet_writer() {
  arrow::Status status;
  status = write();           if (! status.ok()) { std::cerr << "\nCould not write to file "           << status.ToString() << std::endl; }
  status = writer -> Close(); if (! status.ok()) { std::cerr << "\nCould not close the file properly " << status.ToString()  << std::endl; }
}

arrow::Result<std::shared_ptr<arrow::Table>> parquet_writer::make_table() {
  std::vector<std::shared_ptr<arrow::Array>> arrays;
  arrays.reserve(5);

  ARROW_ASSIGN_OR_RAISE(auto x_array      , x_builder      -> Finish()); arrays.push_back(x_array);
  ARROW_ASSIGN_OR_RAISE(auto y_array      , y_builder      -> Finish()); arrays.push_back(y_array);
  ARROW_ASSIGN_OR_RAISE(auto z_array      , z_builder      -> Finish()); arrays.push_back(z_array);
  ARROW_ASSIGN_OR_RAISE(auto i_array, interactions_builder -> Finish()); arrays.push_back(i_array);
  ARROW_ASSIGN_OR_RAISE(auto photon_counts, counts_builder -> Finish()); arrays.push_back(photon_counts);

  return arrow::Table::Make(schema, arrays);
};

arrow::Status parquet_writer::append(const G4ThreeVector& pos, const std::vector<interaction>& interactions, std::unordered_map<size_t, size_t> counts) {
  ARROW_RETURN_NOT_OK(x_builder            -> Append(pos.x()));
  ARROW_RETURN_NOT_OK(y_builder            -> Append(pos.y()));
  ARROW_RETURN_NOT_OK(z_builder            -> Append(pos.z()));
  ARROW_RETURN_NOT_OK(interactions_builder -> Append());
  ARROW_RETURN_NOT_OK(counts_builder       -> Append());

  // ----- Interactions --------------------------------------------------------------------------------------
  auto interaction_builder = static_cast<arrow::StructBuilder*>(interactions_builder -> value_builder());

  auto ix_builder = static_cast<arrow:: FloatBuilder*>(interaction_builder -> field_builder(0));
  auto iy_builder = static_cast<arrow:: FloatBuilder*>(interaction_builder -> field_builder(1));
  auto iz_builder = static_cast<arrow:: FloatBuilder*>(interaction_builder -> field_builder(2));
  auto ie_builder = static_cast<arrow:: FloatBuilder*>(interaction_builder -> field_builder(3));
  auto it_builder = static_cast<arrow::UInt32Builder*>(interaction_builder -> field_builder(4));

  for (const auto& i: interactions) {
    ARROW_RETURN_NOT_OK(interaction_builder -> Append());
    ARROW_RETURN_NOT_OK(ix_builder->Append(i.x));
    ARROW_RETURN_NOT_OK(iy_builder->Append(i.y));
    ARROW_RETURN_NOT_OK(iz_builder->Append(i.z));
    ARROW_RETURN_NOT_OK(ie_builder->Append(i.edep));
    ARROW_RETURN_NOT_OK(it_builder->Append(i.type));
  }

  // ----- SiPM photon counts --------------------------------------------------------------------------------
  auto sipm_count_builder = static_cast<arrow::UInt32Builder*>(counts_builder -> value_builder());

  unsigned n;
  for (size_t i=0; i<my.n_sipms(); i++) {
    n = counts.contains(i) ? counts[i] : 0;
    ARROW_RETURN_NOT_OK(sipm_count_builder -> Append(n));
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

MAYBE_EVENTS read_entire_file(const std::string& filename) {
  arrow::MemoryPool* pool = arrow::default_memory_pool();
  std::shared_ptr<arrow::io::RandomAccessFile> input;
  std::unique_ptr<parquet::arrow::FileReader> reader;
  std::shared_ptr<arrow::Table>                table;

  ARROW_ASSIGN_OR_RAISE(input, arrow::io::ReadableFile::Open(filename));
  ARROW_RETURN_NOT_OK  (parquet::arrow::OpenFile(input, pool, &reader));
  ARROW_RETURN_NOT_OK  (reader -> ReadTable(&table));

  if (! make_schema() -> Equals(*table->schema())) { return arrow::Status::Invalid("Schemas do not match"); }

  auto batch   = table -> CombineChunksToBatch().ValueOrDie();
  auto columns = batch -> columns();
  const auto* x = columns[0] -> data() -> GetValues<float>(1); // I do not understand the meaning of 1
  const auto* y = columns[1] -> data() -> GetValues<float>(1); // I do not understand the meaning of 1
  const auto* z = columns[2] -> data() -> GetValues<float>(1); // I do not understand the meaning of 1
  //const auto* interactions = columns[3] -> data() -> GetValues<float>(1); // I do not understand the meaning of 1
  const auto counts_list  = static_pointer_cast<arrow::FixedSizeListArray>(columns[4]);
  const auto counts_start = static_pointer_cast<arrow::UInt32Array>(counts_list -> values()) -> raw_values();

  std::vector<EVENT> rows;
  for (auto row=0; row< table -> num_rows(); row++) {
    auto counts_vec = std::vector<uint32_t> ( counts_start + counts_list -> value_offset(row    )
                                            , counts_start + counts_list -> value_offset(row + 1));
    std::unordered_map<size_t, size_t> counts_map;
    for (auto [sipm_id, count] : n4::enumerate(counts_vec)) { counts_map.insert({sipm_id, count}); }
    rows.push_back({ {x[row], y[row], z[row]}, counts_map });
  }
  return rows;
}

arrow::Result< std::unordered_map<std::string, std::string>>
read_metadata(const std::string& filename) {
  arrow::MemoryPool* pool = arrow::default_memory_pool();
  std::shared_ptr<arrow::io::RandomAccessFile> input;
  std::unique_ptr<parquet::arrow::FileReader> reader;

  ARROW_ASSIGN_OR_RAISE(input, arrow::io::ReadableFile::Open(filename));
  ARROW_RETURN_NOT_OK  (parquet::arrow::OpenFile(input, pool, &reader));

  auto kv_meta = reader -> parquet_reader() -> metadata() -> key_value_metadata();
  std::unordered_map<std::string, std::string> meta;
  kv_meta -> ToUnorderedMap(&meta);
  return meta;
}
