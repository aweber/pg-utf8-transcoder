CREATE OR REPLACE FUNCTION public.get_next_shortest_unique_key_values
    (in_schemaname name, in_tablename name,
     in_ukvalues text,
     OUT out_cast_next_uk_values text)
 RETURNS text
 LANGUAGE plpgsql
 STRICT SECURITY DEFINER
 SET search_path TO public, pg_temp
AS $$
DECLARE
    v_cols text;
    v_vals text := (in_ukvalues);
    v_sql text;
    v_full_table_name text := quote_ident(in_schemaname) || '.' || quote_ident(in_tablename);
    v_unique_key_cols_array name[];
    v_unique_key_cols TEXT;
    v_unique_key_data_types_array name[];
    v_unique_key_data_types TEXT;
BEGIN
  select out_unique_key_col,
         out_unique_key_data_type
    into v_unique_key_cols_array, v_unique_key_data_types_array
    from public.get_shortest_unique_key(in_schemaname, in_tablename);

  if not found
    THEN raise exception 'Cannot get shortest unique key column(s) and data type(s) for %s.%s.  Aborting.',
            in_schemaname, in_tablename;
  end if;

  v_unique_key_cols       = array_to_string(v_unique_key_cols_array, ', ');
  v_unique_key_data_types = array_to_string(v_unique_key_data_types_array, ', ');

  /*
    v_sql =
'with ukvals as
(
  select public.quoted_elements(array_agg(' || v_unique_key_cols || ')) as vals
    from ' || v_full_table_name || ' x
   where (' || v_unique_key_cols || ') > (' || in_ukvalues || ')
   group by ' || v_unique_key_cols || '
   order by ' || v_unique_key_cols || '
   limit 1
)
select array_to_string(
         public.zip(x.vals,
                    array_agg(format_type(a.atttypid, a.atttypmod))),
         ''::'') as val_cast
  from ukvals x,
       pg_attribute a
  where ''' || v_full_table_name || '''::regclass = a.attrelid
    and a.attname = any(''{' || v_unique_key_cols || '}'')
    and not a.attisdropped
    and a.attnum > 0
  group by x.vals, a.attnum
  order by x.vals, a.attnum
  limit 1';
*/

v_sql =
'with ukvals as
(
select public.quoted_elements(array[' || v_unique_key_cols || ']::text[]) as vals
  from ' || v_full_table_name || '
 where (' || v_unique_key_cols || ') > (' || in_ukvalues || ')
 group by ' || v_unique_key_cols || '
 order by ' || v_unique_key_cols || '
limit 1
)
select array_to_string(array_agg(cast_vals), '', '')::text
  from
(
select array_to_string(public.zip(x.vals::text[], $1::text[]), ''::'') as cast_vals
  from ukvals x
) foo'
;

    RAISE DEBUG 'public.get_next_shortest_unique_key_values query is: %', v_sql;

    EXECUTE v_sql USING v_unique_key_data_types_array INTO out_cast_next_uk_values;

--    EXECUTE v_sql INTO out_cast_next_uk_values;
    RETURN;
END;
$$;

COMMENT ON FUNCTION public.get_next_shortest_unique_key_values(
                in_schemaname NAME, in_tablename NAME,
                in_ukvalues TEXT,
                OUT out_cast_next_uk_values TEXT)
IS
'Returns the minimum shortest unique index values larger than in_ukvalues
for a given table.

INPUT:
    in_schemaname - schema name of table
    in_tablename  - table name
    in_ukvalues   - value(s) of previous shortest unique index

OUTPUT:
    out_cast_next_uk_values - minimum shortest unique index values larger
                              than in_ukvalues
';
