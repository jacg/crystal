#pragma once

#include <G4ThreeVector.hh>

#include <arrow/api.h>
#include <parquet/arrow/writer.h>

#include <memory>
#include <unordered_map>


class parquet_writer {
public:
  parquet_writer();
  ~parquet_writer() {
    std::cerr << "CALLING CLOSE" << std::endl;
    arrow::Status status;
    status = write();
    if (! status.ok()) {
      std::cerr << "Could not write to file" << std::endl;
    }

    status = writer -> Close();
    if (! status.ok()) {
      std::cerr << "Could not close the file properly" << std::endl;
    }
  }

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
  std::unique_ptr<parquet::arrow::FileWriter>  writer;
  std::shared_ptr<arrow::Schema>               schema;

  unsigned n_rows     = 0;
  unsigned n_sipms    = 3;
  unsigned chunk_size = 2;
};
