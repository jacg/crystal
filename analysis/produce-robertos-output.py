import glob
import os
import sys

import pyarrow.parquet as pq

import numpy  as np
import polars as pl


def metadata(df, evt_start):
    return (df.select("x y z".split())
              .insert_at_idx(0, pl.Series("event_id", np.arange(len(df)) + evt_start))
              .rename(dict( x="initial_x"
                          , y="initial_y"
                          , z="initial_z"))
            )

def images(df):
    return (df.select(pl.col("photon_counts").flatten())
              .to_numpy()
              .flatten()
              .reshape(len(df), 8, 8)
              .astype(np.float32)
            )

def write_files(df, start_evt, filename_csv, filename_npy):
    metadata(df, start_evt).write_csv(filename_csv)
    np.save(filename_npy, images(df))

def chunks(df):
    while len(df):
        yield df.slice(    0, 10000) # start, length
        df =  df.slice(10000)


def process_files(filenames, outputdir):
    start_evt = 0
    file_no   = 0
    for filename in filenames:
        for df in chunks(pl.read_parquet(filename, columns="x y z photon_counts".split())):
            filename_csv = os.path.join(outputdir, f"metadata_{file_no:>02}.csv")
            filename_npy = os.path.join(outputdir, f"images_{file_no:>02}.npy")
            write_files(df, start_evt, filename_csv, filename_npy)
            start_evt += len(df)
            file_no   += 1

def metadata_to_str(meta):
    return "\n".join(f"{k.decode()}  => {v.decode()}"
                     for k,v in sorted(meta.items())
                     if k.decode() != "ARROW:schema"
                     ) + "\n"


if __name__ == "__main__":
    _, inputdir, outputdir = sys.argv
    if not os.path.exists(outputdir):
        os.mkdir(outputdir)

    filenames = sorted(glob.glob(os.path.join(inputdir, "*.parquet")))
    process_files(filenames, outputdir)

    meta_filename = os.path.join(outputdir, "simulation_parameters.txt")
    meta = pq.read_metadata(filenames[0]).metadata
    open(meta_filename, "w").write(metadata_to_str(meta))


# filename    = "crystal-out.parquet"
# fileout_csv = "metadata_0.csv"
# fileout_npy =   "images_0.npy"
# data        = pl.read_parquet(filename)

# acc_evt = 0
# csv = (data.select("x y z".split())
#            .insert_at_idx(0, pl.Series("event_id", np.arange(len(data)) + acc_evt))
#            .rename(dict( x="initial_x"
#                        , y="initial_y"
#                        , z="initial_z"))
#        )
# csv.write_csv(fileout_csv)

# npy = (data.select(pl.col("photon_counts")
#                      .flatten())
#            .to_numpy()
#            .flatten()
#            .reshape(len(data), 8, 8)
#            .astype(np.float32)
#        )
# np.save(fileout_npy, npy)

# csv
# event_id,initial_x,initial_y,initial_z

# npy
# (10 000, 8, 8)
