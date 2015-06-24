CREATE OR REPLACE FUNCTION public.zip(in_array_a anyarray, in_array_b anyarray)
  RETURNS SETOF anyarray
LANGUAGE SQL
AS
$$
SELECT ARRAY[a,b] FROM (SELECT unnest($1) AS a, unnest($2) AS b) x;
$$;

COMMENT ON FUNCTION public.zip(anyarray, anyarray)
IS
'Mimics the zip() function in Python.  Will give unexpected results
(cross join) when used with different sized arrays.  Input array
data types must match, otherwise an exception is thrown.

INPUTS:
    in_array_a - First array of any type
    in_array_b - Second array of the same type (not checked)

OUTPUT:
    Array of same type as the two arguments.
';

