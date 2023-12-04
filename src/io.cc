#include "io.hh"

#include <G4ThreeVector.hh>

#include "arrow/io/api.h"
#include "arrow/result.h"
#include "arrow/status.h"
#include "arrow/type_fwd.h"
#include "parquet/exception.h"
#include <unordered_map>

#include <parquet/api/io.h>
#include <parquet/arrow/writer.h>

parquet_writer::parquet_writer(const std::string& filename) :
  pool          {arrow::default_memory_pool()}
, x_builder     {std::make_shared<arrow::FloatBuilder>(pool)}
, y_builder     {std::make_shared<arrow::FloatBuilder>(pool)}
, z_builder     {std::make_shared<arrow::FloatBuilder>(pool)}
, counts_builder{}
, outfile       {arrow::io::FileOutputStream::Open(filename).ValueOrDie()}
, schema        {}
{
  for (auto n=0; n<n_sipms; n++) {
    counts_builder.push_back(std::make_shared<arrow::UInt16Builder>(pool));
  }

  std::vector<std::shared_ptr<arrow::Field>> fields;
  fields.reserve(n_sipms + 3);
  fields.push_back(arrow::field("x", arrow::float32()));
  fields.push_back(arrow::field("y", arrow::float32()));
  fields.push_back(arrow::field("z", arrow::float32()));
  for (auto n=0; n<n_sipms; n++) {
    fields.push_back(arrow::field("sipm_" + std::to_string(n), arrow::uint16()));
  }

  schema = std::make_shared<arrow::Schema>(fields);
}

arrow::Result<std::shared_ptr<arrow::Table>> parquet_writer::make_table() {
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

arrow::Status parquet_writer::append(const G4ThreeVector& pos, std::unordered_map<unsigned, unsigned> counts) {
  ARROW_RETURN_NOT_OK(x_builder -> Append(pos.x()));
  ARROW_RETURN_NOT_OK(y_builder -> Append(pos.y()));
  ARROW_RETURN_NOT_OK(z_builder -> Append(pos.z()));

  unsigned n;
  for (auto i=0; i<n_sipms; i++) {
    n = counts.contains(i) ? counts[i] : 0;
    ARROW_RETURN_NOT_OK(counts_builder[i] -> Append(n));
  }

  return arrow::Status::OK();
}

arrow::Status parquet_writer::write() {
  ARROW_ASSIGN_OR_RAISE(auto data, make_table());
  PARQUET_THROW_NOT_OK(parquet::arrow::WriteTable(*data, pool, outfile, chunk_size));
  return arrow::Status::OK();
}
