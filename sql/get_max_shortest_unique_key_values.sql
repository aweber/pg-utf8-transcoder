CREATE OR REPLACE FUNCTION public.get_max_shortest_unique_key_values(
                in_schemaname NAME, in_tablename NAME,
                in_ukcols TEXT, OUT out_cast_next_uk_values TEXT)
LANGUAGE plpgsql
STRICT
SECURITY DEFINER
SET search_path TO public, pg_temp
AS
$$
DECLARE
    v_sql text;
    v_full_table_name text := quote_ident(in_schemaname) || '.' || quote_ident(in_tablename);
    v_cols text := (in_ukcols);
BEGIN
    v_sql =
'with ukvals as
(
select public.quoted_elements(array_agg(' || v_cols || ')) as vals
from ' || v_full_table_name || ' x
group by ' || v_cols || '
order by ' || v_cols || ' desc
limit 1
)
select array_to_string(
     public.zip(x.vals,
                array_agg(format_type(a.atttypid, a.atttypmod))),
     ''::'') as val_cast
from ukvals x,
   pg_attribute a
where ''' || v_full_table_name || '''::regclass = a.attrelid
and a.attname = any(''{' || v_cols || '}'')
and not a.attisdropped
and a.attnum > 0
group by x.vals, a.attnum
order by x.vals, a.attnum
limit 1';

    RAISE DEBUG 'public.get_max_shortest_unique_key_values query is: %', v_sql;

    EXECUTE v_sql INTO out_cast_next_uk_values;
    RETURN;
END;
$$;

COMMENT ON FUNCTION public.get_max_shortest_unique_key_values(
                in_schemaname NAME, in_tablename NAME,
                in_ukcols TEXT, OUT out_cast_next_uk_values TEXT)
IS
'Returns the maximum shortest unique index value.

    in_schemaname - schema name of table
    in_tablename  - table name
    in_ukcols     - column(s) in shortest unique index

OUTPUT:
    out_cast_next_uk_values - maximum shortest unique index values larger
                              than in_ukvalues
';
