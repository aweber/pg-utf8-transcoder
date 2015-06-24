CREATE OR REPLACE FUNCTION public.get_shortest_unique_key(in_schemaname name, in_tablename name,
   OUT out_unique_key_col name[], OUT out_unique_key_data_type name[])
 LANGUAGE sql
 STRICT
AS $$
with shortest_uindex as
(
     SELECT i.indexrelid, i.indrelid, i.indkey
       FROM pg_index i
      WHERE i.indrelid = ($1 || '.' || $2)::regclass
        AND i.indisunique
        AND i.indisvalid
        AND i.indisready
        AND i.indpred IS NULL
        AND 0 != ANY(i.indkey)
      ORDER BY i.indnatts
      LIMIT 1
), exploded_index_cols as  -- preserve index column order
(
    SELECT indexrelid, indrelid, unnest(indkey) AS indcolnum
      FROM shortest_uindex
)
    SELECT array_agg(a.attname)::name[] as unique_key_col,
           array_agg(format_type(a.atttypid, a.atttypmod))::name[] as unique_key_data_type
      FROM pg_attribute a,
           exploded_index_cols eic
     WHERE a.attrelid = eic.indrelid
       AND a.attnum = eic.indcolnum;
$$;

COMMENT ON FUNCTION public.get_shortest_unique_key(in_schemaname name, in_tablename name,
   OUT out_unique_key_col name[], OUT out_unique_key_data_type name[])
IS
'Returns shortest unique index columns and data types for a given table.

INPUT:
    in_schemaname - schema name of table
    in_tablename  - table name

OUTPUT:
    out_unique_key_col - name array of unique index column(s)
    out_unique_key_data_type - name array of unique index column data type(s)
';
