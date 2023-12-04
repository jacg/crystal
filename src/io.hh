#pragma once

#include <G4ThreeVector.hh>
#include "arrow/memory_pool.h"
#include <arrow/api.h>

#include <memory>
#include <unordered_map>


class parquet_writer {
public:
  parquet_writer(const std::string& filename);
  arrow::Status append(const G4ThreeVector& pos, std::unordered_map<unsigned, unsigned> counts);

  arrow::Status write();
private:
  arrow::Result<std::shared_ptr<arrow::Table>> make_table();

  arrow::MemoryPool* pool;

  // Half float doesn't work
  std::shared_ptr<arrow::FloatBuilder>           x_builder;
  std::shared_ptr<arrow::FloatBuilder>           y_builder;
  std::shared_ptr<arrow::FloatBuilder>           z_builder;
  std::vector<
  std::shared_ptr<arrow::UInt16Builder   >> counts_builder;

  std::shared_ptr<arrow::io::FileOutputStream> outfile;
  std::shared_ptr<arrow::Schema>               schema;

  unsigned n_sipms    = 3;
  unsigned chunk_size = 1024;
};
