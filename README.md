# db-utf8-transcode

### C program to transcode db char-based data to UTF8.

The transcoder runs against one table at a time.  It does the following things:

1. Gets the schema and table to convert, plus other options (report only, force conversion, print debug messages, help) from the command line.
2. Opens read and write connections to the database.
3. Dynamically determines the shortest (in terms of columns) unique index for the table.
4. Dynamically determines the character-based columns for the table.
5. Get the minimum value from the unique index.
6. Gets the row for that minimum value.
7. For each character-based column in the row:
  a. detects the encoding, if it can,
  b. converts the value from the detected encoding to UTF8.  If the encoding cannot be determined for a particular character string the original bytestream is returned.

8. Writes the converted and/or unconverted data back to the same row.
9. Logs the conversion once the update is confirmed.
10. Cleans up memory and database connections, and exits.

Note steps 6, 7 and 8 are not transactional, as they are running on two different connections and the SELECT of the values in step 6 does not use a READ FOR UPDATE lock.  This is purposeful, so the transcoder doesn’t block other activity in the database.  The (very, very) small risk here is a row could be updated between steps 6 and 8, and the update could be lost when the row is overwritten in step 8.

Any variability in rows converted per second for different tables (or even the same table) is due to at least three conditions:

1. number of columns to convert
2. size of data in the columns to convert
3. concurrent load on the database and host machine

### DBA note!

Watch for enforced autovacuums due to wraparound transaction protection when updating large numbers of rows.  Also concurrently reindex any indexed columns that may have been updated after the run completes.

### To run:

Run the transcoder on the database host from the postgres login using a command line like so:

```bash
transcoder --dsn='dbname=<db> user=postgres' --schema=<schema> --table=<table> 1> >(gzip > /tmp/transcoder-runs/<table>.out.gz)  2> >(gzip > /tmp/transcoder-runs/<table>.err.gz)
```

This will compress the stdout and stderr streams, which can get quite large for tables with millions of rows.  To tail the logs use:

```bash
zcat /tmp/transcoder-runs/<table>.err.gz | tail -f
```

### To build:

#### Build and install ICU libraries and header files

```bash
wget http://download.icu-project.org/files/icu4c/54.1/icu4c-54_1-src.tgz
tar -xzf icu4c-54_1-src.tgz
cd icu/source
mkdir -p ./data/out/build/icudt54l
mkdir -p ./data/out/tmp
./runConfigureICU --enable-debug Linux/gcc
make && sudo make install
```

#### Build db-ut8-transcode executable

Install necessary PostgreSQL packages for build

```bash
sudo apt-get install -y -qq postgresql-9.0
sudo apt-get install -y -qq postgresql-9.0-dbg
sudo apt-get install -y -qq postgresql-contrib-9.0
sudo apt-get install -y -qq libpq-dev
sudo apt-get install -y -qq postgresql-server-dev-9.0
```

Update library path

```bash
sudo ldconfig
```

Build and install transcoder executable.  The executable will be in /usr/local/bin

```bash
git clone
cd /opt/git/db-utf8-transcode
make clean && make && sudo make install
```

##### Testing

Build app db

```bash
sudo su - postgres
cd /opt/git/initdb_scripts
./bin/init_db.sh -d app
```


Run tests; all tests should pass

```bash
make requirements test
```

Install the db functions used by transcoder

```bash
sudo su - postgres
cd /opt/git/db-utf8-transcode/sql
psql app -f zip\(anyarray\,anyarray\).sql
psql app -f quoted_elements\(anyarray\).sql
psql app -f get_shortest_unique_key\(name\,name\,name\[\]\,name\[\]\).sql
psql app -f get_min_shortest_unique_key_values.sql
psql app -f get_next_shortest_unique_key_values.sql
```

Install test set

```bash
cd /opt/git/chardet-project/pg_chardetect/test
zcat cust_mail_list_test.dump_p.gz | psql app
```

Convert the data in cust_mail_list_test

```bash
transcoder --dsn='dbname=app' \
          --schema=public \
          --table=cust_mail_list_test \
          2> /tmp/transcoder-test.err \
          1> /tmp/transcoder-test.out
```

Inspect the output in the latest /tmp/run_transcoder.\* for notices, warnings and errors.
There should be no errors, but may be warnings if a conversion is not possible.
