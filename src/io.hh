#pragma once

#include <G4ThreeVector.hh>

#include <arrow/api.h>
#include <parquet/arrow/writer.h>

#include <memory>
#include <unordered_map>

struct interaction {
  float x, y, z;
  float edep;
  unsigned short type;

  interaction(float x, float y,  float z, float edep, unsigned short type)
    : x{x}, y{y}, z{z}, edep{edep}, type{type}
  {}
};


class parquet_writer {
public:
  parquet_writer();
  ~parquet_writer();

  arrow::Status append(const G4ThreeVector& pos, const std::vector<interaction>& interactions, std::unordered_map<size_t, size_t> counts);
  arrow::Status write();

private:
  arrow::Result<std::shared_ptr<arrow::Table>> make_table();
  arrow::MemoryPool* pool;

  // Half float doesn't work
  std::shared_ptr<arrow::FloatBuilder>         x_builder;
  std::shared_ptr<arrow::FloatBuilder>         y_builder;
  std::shared_ptr<arrow::FloatBuilder>         z_builder;
  std::shared_ptr<arrow::ListBuilder>          interactions_builder;
  std::shared_ptr<arrow::FixedSizeListBuilder> counts_builder;

  std::shared_ptr<arrow::Schema>               schema;
  std::unique_ptr<parquet::arrow::FileWriter>  writer;

  unsigned n_rows = 0;
};


using EVENT = std::tuple<G4ThreeVector, std::vector<interaction>, std::unordered_map<size_t, size_t>>;
using MAYBE_EVENTS = arrow::Result<std::vector<EVENT>>;

MAYBE_EVENTS read_entire_file(const std::string& filename);

arrow::Result<
  std::unordered_map<std::string, std::string>
> read_metadata(const std::string& filename);
