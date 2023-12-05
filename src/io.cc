#include "config.hh"
#include "io.hh"

#include <arrow/io/api.h>

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

parquet_writer::parquet_writer() :
  pool          {arrow::default_memory_pool()}
, x_builder     {std::make_shared<arrow::FloatBuilder>(pool)}
, y_builder     {std::make_shared<arrow::FloatBuilder>(pool)}
, z_builder     {std::make_shared<arrow::FloatBuilder>(pool)}
, counts_builder{}
, writer        {}
, schema        {std::make_shared<arrow::Schema>(fields())}
{
  for (auto n=0; n<my.n_sipms(); n++) {
    counts_builder.push_back(std::make_shared<arrow::UInt16Builder>(pool));
  }

  // Choose compression and opt to store Arrow schema for easier reads
  // back into Arrow
  auto  file_props = parquet::     WriterProperties::Builder().compression(arrow::Compression::SNAPPY) -> build();
  auto arrow_props = parquet::ArrowWriterProperties::Builder().store_schema() -> build();
  auto outfile = arrow::io::FileOutputStream::Open(my.outfile).ValueOrDie();
  writer = parquet::arrow::FileWriter::Open( *schema, pool, outfile,
                                             file_props, arrow_props).ValueOrDie();

}

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

arrow::Status parquet_writer::append(const G4ThreeVector& pos, std::unordered_map<unsigned, unsigned> counts) {
  ARROW_RETURN_NOT_OK(x_builder -> Append(pos.x()));
  ARROW_RETURN_NOT_OK(y_builder -> Append(pos.y()));
  ARROW_RETURN_NOT_OK(z_builder -> Append(pos.z()));

  unsigned n;
  for (auto i=0; i<my.n_sipms(); i++) {
    n = counts.contains(i) ? counts[i] : 0;
    ARROW_RETURN_NOT_OK(counts_builder[i] -> Append(n));
  }
  n_rows++;
  return n_rows == chunk_size ? write() : arrow::Status::OK();
}

arrow::Status parquet_writer::write() {
  std::cerr << "CALLING WRITE" << std::endl;
  ARROW_ASSIGN_OR_RAISE(auto data, make_table());
  ARROW_RETURN_NOT_OK(writer -> WriteTable( *data.get(), n_rows));
  n_rows = 0;
  return arrow::Status::OK();
}
