CREATE OR REPLACE FUNCTION public.get_shortest_unique_key_value(in_schemaname name, in_tablename name, in_ukcols text, in_ukvalues text, OUT out_cast_next_uk_values text)
 RETURNS text
 LANGUAGE plpgsql
 STRICT SECURITY DEFINER
 SET search_path TO public, pg_temp
AS $$
DECLARE
    v_sql text;
    v_full_table_name text := quote_ident(in_schemaname) || '.' || quote_ident(in_tablename);
    v_cols text := (in_ukcols);
    v_vals text := (in_ukvalues);
BEGIN

/*
with ukvals
as
(
  select public.quoted_elements(array_agg(campaign_id)) as vals
    from public.cust_mail_list
   where (campaign_id) = ('18'::bigint)
   group by (campaign_id)
   order by (campaign_id)
   limit 1
)
select array_to_string(
         public.zip(x.vals,
                    array_agg(format_type(a.atttypid, a.atttypmod))),
         '::') as val_cast
  from ukvals x,
       pg_attribute a
 where 'public.cust_mail_list'::regclass = a.attrelid
   and a.attname = any('{campaign_id}')
   and not a.attisdropped
   and a.attnum > 0
 group by x.vals, a.attnum
 order by x.vals, a.attnum
 limit 1 ;
   val_cast
--------------
 '18'::bigint
(1 row)
*/

    v_sql =
'with ukvals as
(
  select public.quoted_elements(array_agg(' || v_cols || ')) as vals
    from ' || v_full_table_name || ' x
   where (' || v_cols || ') = (' || v_vals || ')
   group by ' || v_cols || '
   order by ' || v_cols || '
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

    RAISE DEBUG 'public.get_shortest_unique_key_values query is: %', v_sql;

    EXECUTE v_sql INTO out_cast_next_uk_values;
    RETURN;
END;
$$;

COMMENT ON FUNCTION public.get_shortest_unique_key_value(
                in_schemaname NAME, in_tablename NAME,
                in_ukcols TEXT, in_ukvalues TEXT,
                OUT out_cast_next_uk_values TEXT)
IS
'Returns the shortest unique index values equal to in_ukvalues
for a given table.

INPUT:
    in_schemaname - schema name of table
    in_tablename  - table name
    in_ukcols     - column(s) in shortest unique index
    in_ukvalues   - value(s) of previous shortest unique index

OUTPUT:
    out_cast_next_uk_values - minimum shortest unique index values larger
                              than in_ukvalues
';
