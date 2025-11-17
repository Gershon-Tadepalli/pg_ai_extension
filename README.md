# pg_ai_extension

 Use the ClickHouse AI SDK (C++) to build a Postgres extension that exposes a function like pg_gen_query(‘create a table to store orders made by customers’)

# Prerequisite

  Get the Clickhouse ai-sdk-cpp code and install it.

  1. git clone https://github.com/ClickHouse/ai-sdk-cpp
  2. git submodule update --init --recursive
  3. cmake -S . -B build -DCMAKE_CXX_STANDARD=20 -DCMAKE_INSTALL_PREFIX=/opt/ai-sdk
  4. sudo cmake --install build

# Build

  mkdir build && cd build
  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
  make -j8 && make install

# Run

    Start Postgres

    ./pg_ctl -D test start
    ./psql -d postgres

    Run the prompts to get the SQL query output to be copied & executed.

    1) create extension pg_ai_ext;
    2) select pg_gen_query('create a table to store orders made by customers');
    3) select pg_gen_query('insert 10 rows into orders table in the last 2 months');
    4) select pg_gen_query('show all users who made purchases in the last 30 days');
    Above prompts are used as examples