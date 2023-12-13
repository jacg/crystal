#pragma once

#include <G4ThreeVector.hh>

#include <arrow/api.h>
#include <parquet/arrow/writer.h>

#include <memory>
#include <unordered_map>


#define DBG(stuff) std::cerr << "* * * * * * * * * * " << __FILE__ ":" << __LINE__ << "  " << stuff << std::endl;

struct interaction {
  float x, y, z;
  float edep;
  unsigned short type;
};

class parquet_writer {
public:
  // parquet_writer(const parquet_writer& ) = delete;
  // parquet_writer(      parquet_writer&&) = delete;
  parquet_writer();
  ~parquet_writer();

  arrow::Status append(const G4ThreeVector& pos, const std::vector<interaction>& interactions, std::unordered_map<size_t, size_t> counts);
  arrow::Status write();

private:
  arrow::Result<std::shared_ptr<arrow::Table>> make_table();
  arrow::MemoryPool* pool;

  // Half float doesn't work
  std::shared_ptr<arrow::FloatBuilder>               x_builder;
  std::shared_ptr<arrow::FloatBuilder>               y_builder;
  std::shared_ptr<arrow::FloatBuilder>               z_builder;
  std::shared_ptr<arrow::ListBuilder>                interactions_builder;
  std::vector<std::shared_ptr<arrow::UInt32Builder>> counts_builder;

  std::shared_ptr<arrow::Schema>               schema;
  std::unique_ptr<parquet::arrow::FileWriter>  writer;

  unsigned n_rows = 0;
};


arrow::Result<
  std::vector<
    std::pair<
      G4ThreeVector, std::unordered_map<size_t, size_t>
      >
    >
  > read_entire_file(const std::string& filename);

arrow::Result<
  std::unordered_map<std::string, std::string>
> read_metadata(const std::string& filename);
