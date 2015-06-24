CREATE OR REPLACE FUNCTION public.get_min_shortest_unique_key_values(
                in_schemaname NAME, in_tablename NAME,
                OUT out_cast_min_uk_values TEXT)
LANGUAGE plpgsql
STRICT
SECURITY DEFINER
SET search_path TO public, pg_temp
AS
$$
DECLARE
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

v_sql =
'with ukvals as
(
select public.quoted_elements(array[' || v_unique_key_cols || ']::text[]) as vals
  from ' || v_full_table_name || '
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

    RAISE DEBUG 'public.get_min_shortest_unique_key_values query is: %', v_sql;

    EXECUTE v_sql USING v_unique_key_data_types_array INTO out_cast_min_uk_values;
    RETURN;
END;
$$;

COMMENT ON FUNCTION public.get_min_shortest_unique_key_values(
                in_schemaname NAME, in_tablename NAME,
                OUT out_cast_min_uk_values TEXT)
IS
'Returns the minimum shortest unique index value.

    in_schemaname - schema name of table
    in_tablename  - table name

OUTPUT:
    out_cast_min_uk_values - minimum shortest unique index values
';
