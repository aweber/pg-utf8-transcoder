CREATE OR REPLACE FUNCTION public.quoted_elements(anyarray)
RETURNS text[]
LANGUAGE sql
AS
$$
  SELECT array_agg(quoted_elements) AS quoted_elements
    FROM (select quote_literal(unnest($1)) AS quoted_elements) q;
$$;

COMMENT ON FUNCTION public.quoted_elements(anyarray)
IS
'Quote the elements of an array.

INPUT:
    anyarray - any array type

OUTPUT:
    Text array with the elements quoted.
';
