#!/bin/bash

#
#  Set up a test database with non-utf8 data and run the transcoder to transcode it
#

echo "drop db"
dropdb test_transcoder
echo "create db"
createdb --encoding=SQL_ASCII --locale=C test_transcoder
echo "load test table schema and data"
zcat test.dump_p.gz | psql test_transcoder
echo "load db functions used by transcoder"
for f in $(ls -1 ../sql/*.sql); do psql test_transcoder -f $f; done
read -p "Data before transcoding. [press enter]"
psql test_transcoder -c "select * from test" | less
echo 'transcoder --dsn="dbname=test_transcoder" --schema=public --table=test 1> /tmp/test_transcoder.stdout'
transcoder --dsn="dbname=test_transcoder" --schema=public --table=test 1> /tmp/test_transcoder.stdout
read -p "Data after transcoding. [press enter]"
psql test_transcoder -c "select * from test" | less
read -p "Transcoder's stdout [press enter]"
less /tmp/test_transcoder.stdout
